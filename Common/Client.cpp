#include "Client.h"

bool Client::connect(const std::string& host, const std::string& port, const std::string& token)
{
	if (status != CLIENT_STATUS::DISCONNECTED)
		return false;

	asio::error_code er;

	tcp::socket sock(io_context);
	tcp::resolver resolver(io_context);

	tcp::resolver::results_type endpoints = resolver.resolve(host, port, er);

	if (er)
		return false;

	asio::connect(sock, endpoints, er);

	if (er)
		return false;

	conn = std::make_unique<ClientConn>(io_context, token);

	conn->run(std::move(sock), [this](TcpConnexionCallbackInfo info)
		{
			callback(std::move(info));
		});

	workGuard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(io_context.get_executor());

	asio::io_context* _io_context = &io_context;

	threadCallback = std::thread([_io_context]()
		{
			_io_context->restart();
			_io_context->run();
		});

	status = CLIENT_STATUS::CONNECTING;

	return true;
}

void Client::disconnect()
{
	if (status == CLIENT_STATUS::DISCONNECTED)
		return;

	if (workGuard)
	{
		workGuard->reset();
		workGuard.reset();
	}

	if (conn)
		conn->close();

	if (threadCallback.joinable())
		threadCallback.join();

	if (conn)
		conn.reset();

	io_context.stop();

	status = CLIENT_STATUS::DISCONNECTED;
}

bool Client::send(std::span<const char> data)
{
	if (conn)
		return conn->send(data);

	return false;
}

CLIENT_STATUS Client::getStatus() const
{
	return status;
}

void Client::callback(TcpConnexionCallbackInfo info)
{
	switch (info.eventType)
	{
	case TCPCONNEXION_CALLBACK_EVENT::INITIALISED:
	{
		status = CLIENT_STATUS::CONNECTED;

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::ERROR_INITIALIZE:
	{
		status = CLIENT_STATUS::ERROR_BAD_TOKEN;

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::ERROR_RECV:
	{
		status = CLIENT_STATUS::ERROR_RECV;

		callbackData(std::move(info));

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::ERROR_SEND:
	{
		status = CLIENT_STATUS::ERROR_SEND;

		callbackData(std::move(info));

		break;
	}

	case TCPCONNEXION_CALLBACK_EVENT::DATA_AVAILABLE:
	case TCPCONNEXION_CALLBACK_EVENT::DATA_SENT:
	{
		callbackData(std::move(info));

		break;
	}

	default:
		break;
	}
}