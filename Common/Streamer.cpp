#include "Streamer.h"
#include "Protocole.h"
#include <cstring>
#include <chrono>

Streamer::~Streamer()
{
	stop();
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
		sendInProgress = true;
		streaming = true;

		runThreads = true;
		captureThread = std::thread(&Streamer::captureLoop, this);
		encodeThread = std::thread(&Streamer::encodeLoop, this);

		return true;
	}

	return false;
}

void Streamer::stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		streaming = false;
		runThreads = false;
	}

	// notify threads to exit
	captureCv.notify_all();
	encodeCv.notify_all();

	// join threads
	if (captureThread.joinable())
		captureThread.join();
	if (encodeThread.joinable())
		encodeThread.join();

	// clear queues
	{
		std::lock_guard<std::mutex> lock1(captureMutex);
		captureQueue.clear();
	}
	{
		std::lock_guard<std::mutex> lock2(encodedMutex);
		encodedQueue.clear();
	}

	screenshot.stop();
	encoder.stop();
}

bool Streamer::isStreaming() const
{
	return streaming;
}

void Streamer::captureLoop()
{
	while (runThreads)
	{
		std::vector<char> screen;
		if (!screenshot.screen(screen) || screen.empty())
			continue;

		{
			std::lock_guard<std::mutex> lock(captureMutex);
			if (captureQueue.size() >= maxCaptureQueueSize)
			{
				// drop oldest to bound memory
				captureQueue.pop_front();
			}
			captureQueue.emplace_back(std::move(screen));
		}

		encodeCv.notify_one();
	}

	// notify encoder in case it's waiting
	encodeCv.notify_one();
}

void Streamer::encodeLoop()
{
	while (runThreads)
	{
		std::vector<char> frame;

		{
			std::unique_lock<std::mutex> lock(captureMutex);
			if (captureQueue.empty())
			{
				// wait for a capture or timeout to re-check runThreads
				captureCv.wait_for(lock, std::chrono::milliseconds(10), [this]() { return !captureQueue.empty() || !runThreads; });
			}
			if (!captureQueue.empty())
			{
				frame = std::move(captureQueue.front());
				captureQueue.pop_front();
			}
		}

		if (frame.empty())
		{
			continue;
		}

		std::vector<char> dataEncoded;
		if (!encoder.encode(frame, dataEncoded))
			continue;

		if (!sendInProgress)
		{
			std::vector<char> dataSend(sizeof(StreamInfo) + dataEncoded.size());
			std::memcpy(&dataSend[0], &streamInfo, sizeof(StreamInfo));
			std::copy(dataEncoded.begin(), dataEncoded.end(), dataSend.begin() + sizeof(StreamInfo));

			sendInProgress = true;
			send(dataSend);
		}
		else
		{
			std::lock_guard<std::mutex> lock(encodedMutex);
			if (encodedQueue.size() >= maxCaptureQueueSize)
				encodedQueue.pop_front();
			encodedQueue.emplace_back(std::move(dataEncoded));
		}
	}
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

		std::vector<char> dataEncoded;

		{
			std::lock_guard<std::mutex> lock(encodedMutex);
			if (encodedQueue.empty())
			{
				sendInProgress = false;

				return;
			}
			dataEncoded = std::move(encodedQueue.front());
			encodedQueue.pop_front();
		}

		std::vector<char> dataSend(sizeof(StreamInfo) + dataEncoded.size());
		std::memcpy(&dataSend[0], &streamInfo, sizeof(StreamInfo));
		std::copy(dataEncoded.begin(), dataEncoded.end(), dataSend.begin() + sizeof(StreamInfo));

		send(dataSend);
	}
}
