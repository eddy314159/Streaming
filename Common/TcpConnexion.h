#ifndef SOCKET_H
#define SOCKET_H

#include <atomic>
#include <vector>
#include <thread>
#include <span>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <asio.hpp>

using asio::ip::tcp;

class TcpConnexion;

enum TCPCONNEXION_STATUS
{
	FINISHED = 1,
	FINISHING = 2,
	INITIALIZING = 4,
	OPEN = 8,
	CLOSED = 16,
	SENDING = 32,
	RECEVING = 64
};

enum class TCPCONNEXION_CALLBACK_EVENT
{
	INITIALISED,
	ERROR_INITIALIZE,
	DATA_AVAILABLE,
	DATA_SENT,
	ERROR_RECV,
	ERROR_SEND,
};

enum class TCPCONNEXION_CLOSE_TYPE
{
	ERROR_INITIALIZE,
	ERROR_RECV,
	ERROR_SEND,
	USER
};

struct TcpConnexionCallbackInfo
{
	TcpConnexion* conn = nullptr;
	TCPCONNEXION_CALLBACK_EVENT eventType;
	std::vector<char> data;
};

using TcpConnexionCallback = void(TcpConnexionCallbackInfo);

class TcpConnexion
{
public:
	TcpConnexion() = delete;
	TcpConnexion(const TcpConnexion& other) = delete;
	TcpConnexion(TcpConnexion&& other) = default;

	TcpConnexion& operator=(const TcpConnexion& other) = delete;
	TcpConnexion& operator=(TcpConnexion&& other) = default;

	TcpConnexion(asio::io_context& io_context);
	virtual ~TcpConnexion();

	virtual bool run(tcp::socket _sock, std::function<TcpConnexionCallback> _callback);
	virtual void close();

	virtual bool send(std::span<const char> data);
	std::uint32_t getStatus() const;

private:
	static constexpr std::size_t MAX_DATA_SIZE = 50'000'000;

	std::atomic<std::uint32_t> status = TCPCONNEXION_STATUS::FINISHED;

	std::mutex mutex;
	std::condition_variable endOperation;

	std::thread threadInitialize;

	std::function<TcpConnexionCallback> callback;

	static void ThreadInitialize(TcpConnexion* This);
	void recv();

	void internalClose(TCPCONNEXION_CLOSE_TYPE closeType);

protected:
	asio::io_context& io_context;

	tcp::socket sock;

	virtual bool initialize() = 0;
};

#endif