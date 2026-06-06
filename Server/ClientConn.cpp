#include "ClientConn.h"
#include <iostream>

ClientConn::ClientConn(asio::io_context& io_context)
	: TcpConnexion(io_context)
{
	token.resize(TOKEN_SIZE);
}

bool ClientConn::initialize()
{
	asio::error_code er;

	asio::read(sock, asio::buffer(token), er);

	if (er)
		return false;

	return true;
}

std::string ClientConn::getToken() const
{
	return token;
}

bool ClientConn::run(tcp::socket _sock, std::function<TcpConnexionCallback> _callback)
{
	userCallback = std::move(_callback);

	return TcpConnexion::run(std::move(_sock), OverledCallback);
}

void ClientConn::close()
{
	TcpConnexion::close();
	
	std::unique_lock<std::mutex> lock(mutexQueueSend);

	while (!queueSend.empty())
	{
		queueSend.pop();
	}
}

bool ClientConn::send(std::span<const char> data)
{
	if (!(getStatus() & TCPCONNEXION_STATUS::OPEN))
		return false;

	if (!(getStatus() & TCPCONNEXION_STATUS::SENDING))
	{
		return TcpConnexion::send(data);
	}
	else
	{
		std::lock_guard<std::mutex> lock(mutexQueueSend);

		if (queueSend.size() >= MAX_QUEUE_SIZE)
		{
			return false;
		}
		else
		{
			queueSend.push({ data.begin(), data.end() });

			return true;
		}
	}
}

void ClientConn::OverledCallback(TcpConnexionCallbackInfo info)
{
	ClientConn* This = (ClientConn*)info.conn;

	if (info.eventType == TCPCONNEXION_CALLBACK_EVENT::DATA_SENT)
	{
		std::unique_lock<std::mutex> lock(This->mutexQueueSend);

		if (!This->queueSend.empty())
		{
			This->send(This->queueSend.front());

			This->queueSend.pop();
		}
	}
	else
	{
		if (This->userCallback)
			This->userCallback(std::move(info));
	}
	
}