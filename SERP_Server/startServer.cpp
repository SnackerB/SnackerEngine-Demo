#include "Server.h"
#include "Network/Network.h"

#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>

int main()
{
    // Before we create the daemon, check if there is already a server running!
    // First read the pid
    std::ifstream inputFile;
    inputFile.open("logs/pid.txt");
    if (inputFile.is_open()) {
        int pid;
        if (inputFile >> pid && pid != -1) {
            std::cout << "[INFO]: Server is already running with PID " << pid << "." << std::endl;
            return 0;
        }
    }

    pid_t pid;
    // Fork off the parent process
    pid = fork();
    // An error occured
    if (pid < 0) {
        std::cout << "[ERROR]: Failed to start server, error code " << pid << std::endl;
        return -1;
    }
    // On Success: Let the parent process terminate
    if (pid > 0) {
        std::cout << "[INFO]: Started SERP server!" << std::endl;
        return 0;
    }
    // On success: The child process becomes session leader
    int result = setsid();
    if (result < 0) {
        std::cout << "[ERROR]: Failed to start server, setsid() returned " << result << std::endl;
        return -1;
    }
    
    // Fork off for the second time
    pid = fork();
    // An error occured
    if (pid < 0) return -1;
    // Succsess: Let the parent terminate
    if (pid > 0) return 0;
    // Set new file permissions
    umask(0);
    // Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    // Now the code we are running on the server:
    // open log file
    freopen("logs/log.txt", "w", stdout);
    pid = getpid();
    // Write pid to extra file
    std::ofstream myfile;
    myfile.open("logs/pid.txt");
    myfile << pid;
    myfile.close();
    std::cout << "Started server with pid " << getpid() << std::endl;
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