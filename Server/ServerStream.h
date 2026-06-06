#ifndef SERVERSTREAM_H
#define SERVERSTREAM_H

#include "ClientConn.h"

#include <string>
#include <memory>

class ClientConn;

struct ServerStreamClient
{
	bool deleted = false;
	std::string token;
	std::shared_ptr<ClientConn> conn;
	std::mutex mutexAssociedClients;
	std::vector<std::shared_ptr<ServerStreamClient>> associatedClients;
};

class ServerStream
{
public:
	ServerStream();
	ServerStream(const ServerStream& other) = delete;
	ServerStream(ServerStream&& other) = default;
	ServerStream& operator=(const ServerStream& other) = delete;
	ServerStream& operator=(ServerStream&& other) = default;

	~ServerStream();

	bool start(unsigned short port, std::size_t nb_thread);
	void stop();

	unsigned short getPort() const;

	bool addClient(std::string token);
	bool delClient(const std::string& token);
	bool addAssociatedClient(const std::string& clientToken, const std::string& associedClientToken);
	bool delAssociatedClient(const std::string& clientToken, const std::string& associedClientToken);
	bool getClientSatus(const std::string& clientToken, uint32_t& status);

private:
	asio::io_context io_context;

	unsigned short port = 0;

	std::vector<std::thread> threads;  

	std::vector<std::shared_ptr<ServerStreamClient>> clients;
	std::mutex mutexClients;

	std::vector<std::shared_ptr<ClientConn>> pendingConn;
	std::mutex mutexPendingConn;

	asio::steady_timer timerReleaseDisconnectedAssociedClients;

	std::atomic<bool> started = false;

	void listen();
	void callbackClientConn(TcpConnexionCallbackInfo info);
	void releaseDisconnectedAssociedClients();
};

#endif