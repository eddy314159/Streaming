
//#include "testStreamClient.h"

#include <iostream>

#include "Streamer.h"

int main()
{
	
	/*
	std::cout << "token : " << std::endl;
	std::string token;
	std::cin >> token;
	std::cout << "time sleep : " << std::endl;
	int timeSleep;
	std::cin >> timeSleep;

	std::string _token;
	for (int i = 0; i < 16; i++)
		_token += token;

	StreamClient stream;
	*/

	/*
	Client stream;

	std::string l;
	std::cin >> l;

	std::string token;
	for (int i = 0; i < 16; i++)
		token += l;

	stream.run("noelfic.fr", "12345", token, 0, 8'000'000, 60, 1920, 1080);
	//stream.run("127.0.0.1", "12345", token, 0, 4'000'000, 30, 1920, 1080);
	*/

	std::string _token;
	for (int i = 0; i < 16; i++)
		_token += "a";

	Streamer stream;

	stream.connect("127.0.0.1", "12345", _token);
	
	while (stream.getStatus() != CLIENT_STATUS::CONNECTED)
	{
		Sleep(10);
	}

	stream.start(60, 5'000'000);

	system("pause");
	
	stream.stop();
	stream.disconnect();

	return 0;
}