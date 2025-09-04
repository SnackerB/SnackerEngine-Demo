#include <iostream>
#include "Server.h"
#include "Network/Network.h"

int main()
{
	try {
		SnackerEngine::initializeNetwork();
		Server server;
		server.run();
	}
	catch (std::exception& e) {
		std::cout << "exception occured: " << e.what() << std::endl;
	}
	return 0;
}