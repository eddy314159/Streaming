#ifndef VIEWERCONN_H
#define VIEWERCONN_H

#include "TcpConnexion.h"

#include <memory>

struct ServerStreamClient;

class ClientConn : public TcpConnexion
{
public:
	ClientConn() = delete;
	ClientConn(const ClientConn& other) = delete;
	ClientConn(ClientConn&& other) = default;
	ClientConn& operator=(const ClientConn& other) = delete;
	ClientConn& operator=(ClientConn&& other) = default;

	virtual ~ClientConn() = default;

	ClientConn(asio::io_context& io_context, std::string token);

	virtual bool send(std::span<const char> data) override;

	std::string getToken() const;

	constexpr static int TOKEN_SIZE = 50;

private:
	std::string token;

	virtual bool initialize() override;
};
#endif

