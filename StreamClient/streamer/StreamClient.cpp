#include "StreamClient.h"
#include "DirectXScreenshot.h"
#include "H264Encoder.h"

void StreamClient::callback(TcpConnexionCallbackInfo info)
{
    if (info.eventType == TCPCONNEXION_CALLBACK_EVENT::INITIALISED || info.eventType == TCPCONNEXION_CALLBACK_EVENT::DATA_SENT)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeSleep));

        std::vector<char> screen;

        do
        {
            screenshot.screen(screen);
        } while (screen.empty());

        std::vector<char> dataEncoded;
        encoder.encode(screen, dataEncoded);

        info.conn->send(dataEncoded);
    }
    if (info.eventType == TCPCONNEXION_CALLBACK_EVENT::ERROR_RECV || info.eventType == TCPCONNEXION_CALLBACK_EVENT::ERROR_SEND)
    {
        info.conn->close();

        
    }
}

bool StreamClient::run(const std::string& ip, const std::string& port, const std::string& token, int timeSleep, int bitrate, int width, int height)
{
    asio::io_context* _io_context = &io_context;
    auto work = asio::make_work_guard(io_context);

    tcp::socket sock(io_context);

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(ip, port);

    asio::error_code er;

    asio::connect(sock, endpoints, er);

    if (er)
        return false;

    this->timeSleep = timeSleep;

    encoder.start(width, height, 24, bitrate);
    screenshot.start();

    std::unique_ptr<ClientConn> conn = std::make_unique<ClientConn>(io_context, token);
    conn->run(std::move(sock), [this](TcpConnexionCallbackInfo info)
        {
            callback(std::move(info));
        });

    std::vector<std::thread> threads;

    for (int i = 0; i < 1; i++)
    {
        threads.push_back(std::thread([_io_context]()
            {
                _io_context->run();
            }));
    }

    for (std::thread& t : threads)
        t.join();
}