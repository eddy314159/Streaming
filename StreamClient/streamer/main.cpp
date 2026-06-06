#include <iostream>

#include "StreamClient.h"

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

	StreamClient stream;

	std::string token;
	std::cin >> token;

	std::cout << stream.run("localhost", "12345", token, 1, 4'000'000, 1920, 1080);

	return 0;
}