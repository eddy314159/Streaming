#ifndef STREAMER_H
#define STREAMER_H

#include "Client.h"
#include "Protocole.h"
#include "H264Encoder.h"
#include "DirectXScreenshot.h"

#include <thread>
#include <deque>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <vector>

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

	// threading + queues
	std::thread captureThread;
	std::thread encodeThread;

	std::deque<std::vector<char>> captureQueue;    // raw screenshots
	std::deque<std::vector<char>> encodedQueue;    // encoded frames ready to send

	std::mutex captureMutex;
	std::mutex encodedMutex;
	std::condition_variable captureCv;
	std::condition_variable encodeCv;

	std::atomic<bool> streaming = false;
	std::atomic<bool> runThreads = false;
	// indicates an async send is in progress (synchronization flag)
	std::atomic<bool> sendInProgress = false;

	size_t maxCaptureQueueSize = 5;

	// loops
	void captureLoop();
	void encodeLoop();

	virtual void callbackData(TcpConnexionCallbackInfo info) override;
};

#endif
