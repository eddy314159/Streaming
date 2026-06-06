#ifndef STREAM_CLIENt_H
#define STREAM_CLIENT_H

#include "ClientConn.h"
#include "DirectXScreenshot.h"
#include "H264Encoder.h"

class StreamClient
{
public:
	StreamClient() = default;

	bool run(const std::string& ip, const std::string& port, const std::string& token, int timeSleep, int bitrate, int width, int height);

private:
	void callback(TcpConnexionCallbackInfo info);

	H264Encoder encoder;
	DirectXScreenShot screenshot;
	int timeSleep = 0;

	asio::io_context io_context;
};

#endif
