#include "Streamer.h"
#include "Protocole.h"

Streamer::~Streamer()
{
	screenshot.stop();
	encoder.stop();
}

bool Streamer::start(int _framerate, int _bitrate)
{
	if (getStatus() != CLIENT_STATUS::CONNECTED || streaming)
		return false;

	if (!screenshot.start())
		return false;

	if (!encoder.start(screenshot.getWidth(), screenshot.getHeight(), _framerate, _bitrate))
	{
		screenshot.stop();

		return false;
	}

	StreamInfo _streamInfo;

	_streamInfo.width = screenshot.getWidth();
	_streamInfo.height = screenshot.getHeight();
	_streamInfo.framerate = _framerate;
	_streamInfo.bitrate = _bitrate;

	streamInfo = _streamInfo;

	if (send({ reinterpret_cast<const char*>(&_streamInfo) , sizeof(StreamInfo) }))
	{
		streaming = true;

		return true;
	}

	return false;
}

void Streamer::stop()
{
	std::lock_guard<std::mutex> lock(mutex);

	streaming = false;

	screenshot.stop();
	encoder.stop();
}

bool Streamer::isStreaming() const
{
	return streaming;
}

void Streamer::callbackData(TcpConnexionCallbackInfo info)
{
	if (getStatus() != CLIENT_STATUS::CONNECTED)
	{
		stop();

		return;
	}

	if (info.eventType == TCPCONNEXION_CALLBACK_EVENT::DATA_SENT)
	{
		if (!streaming)
			return;

		std::lock_guard<std::mutex> lock(mutex);

		std::vector<char> screen;

		do
		{
			screenshot.screen(screen);
		} while (screen.empty());

		std::vector<char> dataEncoded;

		if (!encoder.encode(screen, dataEncoded))
		{
			send({ reinterpret_cast<const char*>(&streamInfo) , sizeof(StreamInfo) });

			return;
		}

		std::vector<char> dataSend(sizeof(StreamInfo) + dataEncoded.size());

		std::memcpy(&dataSend[0], &streamInfo, sizeof(StreamInfo));
		std::copy(dataEncoded.begin(), dataEncoded.end(), dataSend.begin() + sizeof(StreamInfo));

		send(dataSend);
	}
}