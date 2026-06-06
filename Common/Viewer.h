#ifndef VIEWER_H
#define VIEWER_H

#include "Client.h"
#include "H264Decoder.h"
#include "Protocole.h"

class Viewer : public Client
{
public:
	Viewer() = default;
	Viewer(std::function<void(unsigned int, unsigned int, std::vector<char>)> callBack);
	Viewer(const Viewer& other) = delete;
	Viewer(Viewer&& other) = default;
	Viewer& operator=(const Viewer& other) = delete;
	Viewer& operator=(Viewer&& other) = default;
	~Viewer();

	StreamInfo getStreamInfo() const;

private:
	H264Decoder decoder;
	std::atomic<StreamInfo> streamInfo = { {0} };

	std::function<void(unsigned int, unsigned int, std::vector<char>)> callbackFrame;

	virtual void callbackData(TcpConnexionCallbackInfo info) override;
};

#endif