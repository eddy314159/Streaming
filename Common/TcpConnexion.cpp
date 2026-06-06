#include "TcpConnexion.h"
#include <iostream>

TcpConnexion::TcpConnexion(asio::io_context& io_context)
	: io_context(io_context), sock(io_context)
{

}

TcpConnexion::~TcpConnexion()
{
	std::unique_lock<std::mutex> lock(mutex);

	endOperation.wait(lock, [this]()
		{
			return !(status & TCPCONNEXION_STATUS::RECEVING) && !(status & TCPCONNEXION_STATUS::SENDING);
		});
}

bool TcpConnexion::run(tcp::socket _sock, std::function<TcpConnexionCallback> _callback)
{
	mutex.lock();

	if (!(status & TCPCONNEXION_STATUS::FINISHED))
	{
		mutex.unlock();

		return false;
	}

	sock = std::move(_sock);
	callback = std::move(_callback);

	status &= ~TCPCONNEXION_STATUS::FINISHED;
	status &= ~TCPCONNEXION_STATUS::CLOSED;
	status |= TCPCONNEXION_STATUS::INITIALIZING;

	threadInitialize = std::thread(ThreadInitialize, this);

	mutex.unlock();

	return true;
}

void TcpConnexion::close()
{
	internalClose(TCPCONNEXION_CLOSE_TYPE::USER);
}

void TcpConnexion::internalClose(TCPCONNEXION_CLOSE_TYPE closeType)
{
	std::unique_lock<std::mutex> lock(mutex);

	bool alreadyClosed = status & TCPCONNEXION_STATUS::CLOSED;

	if (!alreadyClosed)
	{
		asio::error_code er;
		sock.cancel(er);
		sock.close(er);

		status &= ~TCPCONNEXION_STATUS::OPEN;
		status |= TCPCONNEXION_STATUS::CLOSED;
	}

	if (closeType == TCPCONNEXION_CLOSE_TYPE::USER)
	{
		if (status & TCPCONNEXION_STATUS::FINISHING || status & TCPCONNEXION_STATUS::FINISHED)
		{
			lock.unlock();

			return;
		}

		status |= TCPCONNEXION_STATUS::FINISHING;

		lock.unlock();

		threadInitialize.join();

		status &= ~TCPCONNEXION_STATUS::FINISHING;
		status |= TCPCONNEXION_STATUS::FINISHED;
	}
	else if (!alreadyClosed)
	{
		TcpConnexionCallbackInfo info;
		info.conn = this;

		switch (closeType)
		{
		case TCPCONNEXION_CLOSE_TYPE::ERROR_INITIALIZE:
			info.eventType = TCPCONNEXION_CALLBACK_EVENT::ERROR_INITIALIZE;

			break;

		case TCPCONNEXION_CLOSE_TYPE::ERROR_RECV:
			info.eventType = TCPCONNEXION_CALLBACK_EVENT::ERROR_RECV;

			break;

		case TCPCONNEXION_CLOSE_TYPE::ERROR_SEND:
			info.eventType = TCPCONNEXION_CALLBACK_EVENT::ERROR_SEND;

			break;
		}

		lock.unlock();

		callback(std::move(info));
	}

	if (closeType == TCPCONNEXION_CLOSE_TYPE::ERROR_RECV || closeType == TCPCONNEXION_CLOSE_TYPE::ERROR_SEND)
		endOperation.notify_all();
}

std::uint32_t TcpConnexion::getStatus() const
{
	return status;
}

void TcpConnexion::ThreadInitialize(TcpConnexion* This)
{
	bool initialized = This->initialize();

	This->mutex.lock();

	This->status &= ~TCPCONNEXION_STATUS::INITIALIZING;

	if (initialized && (!(This->status & TCPCONNEXION_STATUS::CLOSED)))
	{
		This->status |= TCPCONNEXION_STATUS::OPEN;

		This->mutex.unlock();

		TcpConnexionCallbackInfo info;
		info.eventType = TCPCONNEXION_CALLBACK_EVENT::INITIALISED;
		info.conn = This;

		This->io_context.post([This, info]()
			{
				This->callback(info);
			});

		This->recv();
	}
	else
	{
		This->mutex.unlock();

		This->io_context.post([This]()
			{
				This->internalClose(TCPCONNEXION_CLOSE_TYPE::ERROR_INITIALIZE);
			});
	}
}

void TcpConnexion::recv()
{
	status |= TCPCONNEXION_STATUS::RECEVING;

	std::shared_ptr<std::vector<char>> dataRecv = std::make_shared<std::vector<char>>();

	dataRecv->resize(sizeof(std::uint32_t));

	asio::async_read(sock, asio::buffer(*dataRecv),
		[this, dataRecv](asio::error_code er, std::size_t size)
		{
			mutex.lock();

			if (er || size > MAX_DATA_SIZE || dataRecv->size() != size)
			{
				status &= ~TCPCONNEXION_STATUS::RECEVING;
				dataRecv->clear();

				mutex.unlock();

				internalClose(TCPCONNEXION_CLOSE_TYPE::ERROR_RECV);

				return;
			}

			std::uint32_t sizeData = reinterpret_cast<std::uint32_t&>((*dataRecv)[0]);

			if (!sizeData)
			{
				status &= ~TCPCONNEXION_STATUS::RECEVING;
				dataRecv->clear();

				mutex.unlock();

				internalClose(TCPCONNEXION_CLOSE_TYPE::ERROR_RECV);

				return;
			}

			dataRecv->resize(sizeData);

			mutex.unlock();

			asio::async_read(sock, asio::buffer(*dataRecv),
				[this, dataRecv](asio::error_code er, std::size_t size)
				{
					mutex.lock();

					if (er || size > MAX_DATA_SIZE || dataRecv->size() != size)
					{
						status &= ~TCPCONNEXION_STATUS::RECEVING;
						dataRecv->clear();

						mutex.unlock();

						internalClose(TCPCONNEXION_CLOSE_TYPE::ERROR_RECV);

						return;
					}
					
					status &= ~TCPCONNEXION_STATUS::RECEVING;

					mutex.unlock();

					TcpConnexionCallbackInfo info;

					info.eventType = TCPCONNEXION_CALLBACK_EVENT::DATA_AVAILABLE;
					info.conn = this;
					info.data = std::move(*dataRecv);

					callback(std::move(info));

					recv();
				});
		});
}

bool TcpConnexion::send(std::span<const char> data)
{
	assert(!data.empty() && data.size() <= MAX_DATA_SIZE);

	std::unique_lock<std::mutex> lock(mutex);

	if (!(status & TCPCONNEXION_STATUS::OPEN) || status & TCPCONNEXION_STATUS::SENDING)
	{
		lock.unlock();

		return false;
	}

	status |= TCPCONNEXION_STATUS::SENDING;

	std::shared_ptr<std::vector<char>> dataSend = std::make_shared<std::vector<char>>();;

	dataSend->resize(sizeof(std::uint32_t) + data.size());

	reinterpret_cast<std::uint32_t&>( (*dataSend)[0]) = static_cast<std::uint32_t>(data.size());
	std::copy(data.begin(), data.end(), dataSend->begin() + sizeof(std::uint32_t));

	asio::async_write(sock, asio::buffer(*dataSend),
		[this, dataSend](asio::error_code er, std::size_t size)
		{
			std::unique_lock<std::mutex> lock(mutex);

			status &= ~TCPCONNEXION_STATUS::SENDING;

			if (er || size > MAX_DATA_SIZE || dataSend->size() != size)
			{
				lock.unlock();

				internalClose(TCPCONNEXION_CLOSE_TYPE::ERROR_SEND);

				return;
			}

			lock.unlock();

			TcpConnexionCallbackInfo info;

			info.eventType = TCPCONNEXION_CALLBACK_EVENT::DATA_SENT;
			info.conn = this;

			callback(std::move(info));
		});

	return true;
}