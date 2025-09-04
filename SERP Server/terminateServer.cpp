#include <fstream>
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <cstring>

int main() 
{
    // First read the pid
    std::ifstream inputFile;
    inputFile.open("logs/pid.txt");
    if (!inputFile.is_open()) {
        std::cout << "[INFO]: Didn't find file \"logs/pid.txt\". Server is likely not running." << std::endl;
        return 0;
    }
    int pid;
    if (!(inputFile >> pid)) {
        std::cout << "[INFO]: Wasn't able to read int from file \"logs/pid.txt\". Server is likely not running." << std::endl;
    }
    if (pid == -1) {
        std::cout << "[INFO]: SERP Server is not running." << std::endl;
    }
    else {
        std::cout << "[INFO]: Terminating SERP Server ..." << std::endl;
        int result = kill(pid, SIGKILL);
        if (result == 0) {
            std::cout << "[INFO] SERP Server terminated!" << std::endl;
            std::ofstream outputFile;
            outputFile.open("logs/pid.txt");
            outputFile << -1;
            outputFile.close();
        }
        else {
            std::cout << "[ERROR]: error code " << errno << ": " << strerror(errno) << std::endl;
        }
    }
}