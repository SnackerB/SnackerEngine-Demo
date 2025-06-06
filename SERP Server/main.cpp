#include <iostream>
#include "Server.h"
#include "Network\Network.h"
#include "Network\TCP\TCP.h"

#include <WinSock2.h>

void main()
{
	std::cout << "Starting SERP server!" << std::endl;
	try {
		SnackerEngine::initializeNetwork();
		SERPServer server;
		server.run();
	}
	catch (std::exception& e) {
		std::cout << "exception occured: " << e.what() << std::endl;
	}
}