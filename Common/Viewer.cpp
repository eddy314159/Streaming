#include "Viewer.h"

Viewer::Viewer(std::function<void(unsigned int, unsigned int, std::vector<char>)> callBack) :
	Client(), callbackFrame(std::move(callBack))
{

}

Viewer::~Viewer()
{
	decoder.stop();
}

void Viewer::callbackData(TcpConnexionCallbackInfo info)
{
	if (getStatus() != CLIENT_STATUS::CONNECTED)
		return;

	if (info.eventType == TCPCONNEXION_CALLBACK_EVENT::DATA_AVAILABLE)
	{
		if (info.data.size() < sizeof(StreamInfo))
			return;

		StreamInfo newStreamInfo = reinterpret_cast<StreamInfo&>(info.data[0]);
		StreamInfo oldStreamInfo = streamInfo;

		if (newStreamInfo.height != oldStreamInfo.height || newStreamInfo.width != oldStreamInfo.width)
		{
			decoder.stop();
			decoder.start(newStreamInfo.width, newStreamInfo.height);
		}

		if (info.data.size() > sizeof(StreamInfo))
		{
			std::vector<char> frame;

			if (decoder.decode({ info.data.begin() + sizeof(StreamInfo), info.data.end() }, frame))
				callbackFrame(newStreamInfo.width, newStreamInfo.height, std::move(frame));
		}

		streamInfo = newStreamInfo;
	}
}

StreamInfo Viewer::getStreamInfo() const
{
	return streamInfo;
}

