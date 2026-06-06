#include "ServerStream.h"
#include <iostream>

int main()
{
	ServerStream s;

	s.start(12345, 1);

	std::vector<std::string> tokens;

	while (true)
	{
		std::cout << "operation : " << std::endl;
		std::string operation;
		std::cin >> operation;
		std::cout << "token : " << std::endl;
		std::string token;
		std::cin >> token;

		std::string _token;
		for (int i = 0; i < 16; i++)
			_token += token;

		if (operation == "add")
		{
			if (s.addClient(_token))
			{
				std::cout << "client added" << std::endl;

				tokens.push_back(_token);
			}
		}
		else if (operation == "del")
		{
			if (s.delClient(_token))
			{
				for (std::string& t : tokens)
				{
					if (t == _token)
						t.clear();
				}

				std::cout << "client deleted" << std::endl;
			}
		}
		else if (operation == "adda")
		{
			std::cout << "associed client token : " << std::endl;
			std::string tokena;
			std::cin >> tokena;

			std::string _tokena;
			for (int i = 0; i < 16; i++)
				_tokena += tokena;

			if (s.addAssociatedClient(_token, _tokena))
			{
				std::cout << "associed client added" << std::endl;
			}
		}
		else if (operation == "dela")
		{
			std::cout << "associed client token : " << std::endl;
			std::string tokena;
			std::cin >> tokena;

			std::string _tokena;
			for (int i = 0; i < 16; i++)
				_tokena += tokena;

			if (s.delAssociatedClient(_token, _tokena))
			{
				std::cout << "associed client added" << std::endl;
			}
		}
		else if (operation == "stop")
		{
			s.stop();
			break;
		}

		for (const std::string& t : tokens)
		{
			if (t.empty())
				continue;

			uint32_t status;

			if (s.getClientSatus(t, status))
			{
				std::cout << "client token : " << t << " status : " << status << std::endl;
			}
		}
	}

	return 0;
}