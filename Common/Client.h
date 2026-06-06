#ifndef STREAMCLIENT_H
#define STREAMCLIENT_H

#include "ClientConn.h"
#include "DirectXScreenshot.h"
#include "H264Encoder.h"

#include <thread>

enum class CLIENT_STATUS
{
	CONNECTED,
	CONNECTING,
	DISCONNECTED,
	ERROR_BAD_TOKEN,
	ERROR_SEND,
	ERROR_RECV
};

class Client
{
public:
	Client() = default;
	Client(const Client& other) = delete;
	Client(Client&& other) = default;
	Client& operator=(const Client& other) = delete;
	Client& operator=(Client&& other) = default;

	bool connect(const std::string& host, const std::string& port, const std::string& token);
	void disconnect();

	CLIENT_STATUS getStatus() const;

protected:
	bool send(std::span<const char> data);

	virtual void callbackData(TcpConnexionCallbackInfo info) = 0;

private:
	asio::io_context io_context;
	std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> workGuard;

	std::atomic<CLIENT_STATUS> status = CLIENT_STATUS::DISCONNECTED;

	std::thread threadCallback;
	std::unique_ptr<ClientConn> conn;

	void callback(TcpConnexionCallbackInfo info);
};

#endif