#ifndef STREAMER_H
#define STREAMER_H

#include "Client.h"
#include "Protocole.h"
#include "h264Encoder.h"
#include "DirectXScreenshot.h"

class Streamer : public Client
{
public:
	Streamer() = default;
	Streamer(const Streamer& other) = delete;
	Streamer(Streamer&& other) = default;
	Streamer& operator=(const Streamer& other) = delete;
	Streamer& operator=(Streamer&& other) = default;

	~Streamer();

	bool start(int framerate, int bitrate);
	void stop();
	bool isStreaming() const;
	StreamInfo getStreamInfo() const;

private:
	H264Encoder encoder;
	DirectXScreenShot screenshot;
	StreamInfo streamInfo = { 0 };
	std::mutex mutex;

	std::atomic<bool> streaming = false;

	virtual void callbackData(TcpConnexionCallbackInfo info) override;
};

#endif
