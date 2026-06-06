#include "ServerStream.h"

#include <iostream>

ServerStream::ServerStream()
	: timerReleaseDisconnectedAssociedClients(io_context)
{
}

ServerStream::~ServerStream()
{
	stop();
}

bool ServerStream::start(unsigned short _port, std::size_t nb_thread)
{
	if (started)
		return false;

	port = _port;

	listen();
	releaseDisconnectedAssociedClients();

	for (std::size_t i = 0; i < nb_thread; i++)
	{
		threads.push_back(std::thread([this]()
			{
				io_context.run();
			}));
	}

	started = true;

	return true;
}

void ServerStream::stop()
{
	if (!started)
		return;

	started = false;

	io_context.stop();

	for (std::thread& thread : threads)
	{
		thread.join();
	}
	threads.clear();

	for (std::shared_ptr<ServerStreamClient>& client : clients)
	{
		if (client && client->conn)
			client->conn->close();
	}
	clients.clear();

	for (std::shared_ptr<ClientConn>& conn : pendingConn)
	{
		if (conn)
			conn->close();
	}
	pendingConn.clear();

	port = 0;
}

unsigned short ServerStream::getPort() const
{
	return port;
}

void ServerStream::listen()
{
	std::shared_ptr<tcp::socket> sock = std::make_shared<tcp::socket>(io_context);

	std::shared_ptr<tcp::acceptor> acceptor = std::make_shared<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));

	acceptor->async_accept(*sock, [this, sock, acceptor](asio::error_code er)
		{
			if (!er)
			{
				mutexPendingConn.lock();

				bool found = false;
				ClientConn* newConn = nullptr;

				for (std::shared_ptr<ClientConn>& conn : pendingConn)
				{
					if (!conn)
					{
						conn = std::make_unique<ClientConn>(io_context);
						newConn = conn.get();

						found = true;
						break;
					}
				}

				if (!found)
				{
					std::unique_ptr<ClientConn> _newConn = std::make_unique<ClientConn>(io_context);;
					newConn = _newConn.get();

					pendingConn.push_back(std::move(_newConn));
				}

				mutexPendingConn.unlock();

				auto callback = [this](TcpConnexionCallbackInfo info)
				{
					callbackClientConn(std::move(info));
				};

				newConn->run(std::move(*sock), std::move(callback));
			}

			if (acceptor->is_open() && started)
			{
				acceptor->close();

				listen();
			}
		});
}

void ServerStream::callbackClientConn(TcpConnexionCallbackInfo info)
{
	ClientConn* conn = (ClientConn*)(info.conn);

	assert(conn);

	switch (info.eventType)
	{
	case TCPCONNEXION_CALLBACK_EVENT::INITIALISED:
	{
		std::shared_ptr<ClientConn>* connInitialised = nullptr;

		mutexPendingConn.lock();

		for (std::shared_ptr<ClientConn>& pendingConn : pendingConn)
		{
			if (conn == pendingConn.get())
			{
				connInitialised = &pendingConn;

				break;
			}
		}

		mutexPendingConn.unlock();

		assert(connInitialised != nullptr);

		mutexClients.lock();

		for (std::shared_ptr<ServerStreamClient>& client : clients)
		{
			if (client && client->token == (*connInitialised)->getToken())
			{
				if (!client->conn || client->conn->getStatus() & TCPCONNEXION_STATUS::FINISHED)
				{
					client->conn = *connInitialised;
					client->conn->client = client.get();

					connInitialised->reset();
				}

				break;
			}
		}

		mutexClients.unlock();

		if (*connInitialised)
			(*connInitialised)->close();

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::ERROR_INITIALIZE:
	{
		mutexPendingConn.lock();

		for (std::shared_ptr<ClientConn>& pendingConn : pendingConn)
		{
			if (conn == pendingConn.get())
			{
				conn->close();

				break;
			}
		}

		mutexPendingConn.unlock();

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::DATA_AVAILABLE:
	{
		ServerStreamClient* client = conn->client;

		assert(client != nullptr);

		if (!(client->conn->getStatus() & TCPCONNEXION_STATUS::OPEN))
			return;

		client->mutexAssociedClients.lock();

		for (std::shared_ptr<ServerStreamClient> associedClient : client->associatedClients)
		{
			if (associedClient && associedClient->conn && associedClient->conn->getStatus() & TCPCONNEXION_STATUS::OPEN)
				associedClient->conn->send(info.data);
		}

		client->mutexAssociedClients.unlock();

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::ERROR_RECV:
	case TCPCONNEXION_CALLBACK_EVENT::ERROR_SEND:
	{
		conn->close();

		break;
	}
	};
}

bool ServerStream::addClient(std::string token)
{
	if (!started)
		return false;

	assert(token.size() == ClientConn::TOKEN_SIZE);

	std::lock_guard<std::mutex> lock(mutexClients);

	ServerStreamClient* newClient = nullptr;

	for (std::shared_ptr<ServerStreamClient>& client : clients)
	{
		if (!client)
		{
			client = std::make_shared<ServerStreamClient>();
			newClient = client.get();

			break;
		}
	}

	if (!newClient)
	{
		std::shared_ptr<ServerStreamClient> _newClient = std::make_shared<ServerStreamClient>();
		newClient = _newClient.get();

		clients.push_back(std::move(_newClient));
	}

	newClient->token = std::move(token);

	return true;
}

bool ServerStream::addAssociatedClient(const std::string& clientToken, const std::string& associedClientToken)
{
	if (!started)
		return false;

	std::lock_guard<std::mutex> lock(mutexClients);

	ServerStreamClient* client = nullptr;
	std::shared_ptr<ServerStreamClient> newAssociedClient;

	for (const std::shared_ptr<ServerStreamClient>& _client : clients)
	{
		if (!_client)
			continue;

		if (_client->token == clientToken)
			client = _client.get();

		if (_client->token == associedClientToken)
			newAssociedClient = _client;
	}

	if (!client || !newAssociedClient)
		return false;

	client->mutexAssociedClients.lock();

	for (std::shared_ptr<ServerStreamClient>& associedClient : client->associatedClients)
	{
		if (!associedClient)
		{
			associedClient = std::move(newAssociedClient);

			client->mutexAssociedClients.unlock();

			return true;
		}
	}

	client->associatedClients.push_back(std::move(newAssociedClient));

	client->mutexAssociedClients.unlock();

	return true;
}

bool ServerStream::delClient(const std::string& token)
{
	if (!started)
		return false;

	mutexPendingConn.lock();

	for (std::shared_ptr<ClientConn>& pendingConn : pendingConn)
	{
		if (pendingConn && pendingConn->getStatus() & TCPCONNEXION_STATUS::FINISHED
			&& pendingConn->getToken() == token)
			pendingConn.reset();
	}

	mutexPendingConn.unlock();

	std::lock_guard<std::mutex> lock(mutexClients);

	for (std::shared_ptr<ServerStreamClient>& client : clients)
	{
		if (client && client->token == token)
		{
			if (client->conn)
				client->conn->close();

			client->mutexAssociedClients.lock();

			client->associatedClients.clear();

			client->mutexAssociedClients.unlock();

			client->deleted = true;

			client.reset();

			return true;
		}
	}

	return false;
}

bool ServerStream::delAssociatedClient(const std::string& clientToken, const std::string& associedClientToken)
{
	if (!started)
		return false;

	std::lock_guard<std::mutex> lock(mutexClients);

	ServerStreamClient* client = nullptr;
	ServerStreamClient* associedClient = nullptr;

	for (const std::shared_ptr<ServerStreamClient>& _client : clients)
	{
		if (!_client)
			continue;

		if (_client->token == clientToken)
			client = _client.get();

		if (_client->token == associedClientToken)
			associedClient = _client.get();
	}

	if (!client || !associedClient)
		return false;

	client->mutexAssociedClients.lock();

	for (std::shared_ptr<ServerStreamClient>& _associedClient : client->associatedClients)
	{
		if (_associedClient.get() == associedClient)
		{
			_associedClient.reset();

			client->mutexAssociedClients.unlock();

			return true;
		}
	}

	client->mutexAssociedClients.unlock();

	return false;
}

bool ServerStream::getClientSatus(const std::string& clientToken, uint32_t& status)
{
	if (!started)
		return false;

	std::lock_guard<std::mutex> lock(mutexClients);

	for (std::shared_ptr<ServerStreamClient>& client : clients)
	{
		if (client && client->token == clientToken)
		{
			if (client->conn)
				status = client->conn->getStatus();

			else
				status = TCPCONNEXION_STATUS::FINISHED;

			return true;
		}
	}

	return false;
}

void ServerStream::releaseDisconnectedAssociedClients()
{
	std::lock_guard<std::mutex> lock(mutexClients);

	for (std::shared_ptr<ServerStreamClient> client : clients)
	{
		if (client)
		{
			client->mutexAssociedClients.lock();

			for (std::shared_ptr<ServerStreamClient>& associedClient : client->associatedClients)
			{
				if (associedClient && associedClient->deleted)
					associedClient.reset();
			}

			client->mutexAssociedClients.unlock();
		}
	}

	timerReleaseDisconnectedAssociedClients.expires_after(std::chrono::seconds(1));
	timerReleaseDisconnectedAssociedClients.async_wait([this](asio::error_code er)
	{
		if (started)
			releaseDisconnectedAssociedClients();
	}); 
}