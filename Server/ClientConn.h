#ifndef VIEWERCONN_H
#define VIEWERCONN_H

#include "TcpConnexion.h"
#include "ServerStream.h"

#include <memory>
#include <queue>

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

	ClientConn(asio::io_context& io_context);

	virtual bool run(tcp::socket _sock, std::function<TcpConnexionCallback> _callback) override;
	virtual void close() override;

	virtual bool send(std::span<const char> data) override;

	std::string getToken() const;

	ServerStreamClient* client = nullptr;

	constexpr static int TOKEN_SIZE = 16;

private:
	static constexpr std::size_t MAX_QUEUE_SIZE = 10;

	std::string token;

	std::mutex mutexQueueSend;
	std::queue<std::vector<char>> queueSend;

	std::function<TcpConnexionCallback> userCallback;

	bool internalSend();
	virtual bool initialize() override;
	static void OverledCallback(TcpConnexionCallbackInfo info);
};
#endif
