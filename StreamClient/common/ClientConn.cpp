#include "ClientConn.h"

ClientConn::ClientConn(asio::io_context& io_context, std::string token)
	: TcpConnexion(io_context), token(std::move(token))
{
	
}

bool ClientConn::initialize()
{
	asio::error_code er;

	asio::write(sock, asio::buffer(token), er);

	if (er)
		return false;

	return true;
}

std::string ClientConn::getToken() const
{
	return token;
}

bool ClientConn::send(std::span<const char> data)
{
	return TcpConnexion::send(data);
}