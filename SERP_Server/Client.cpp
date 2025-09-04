#include "Client.h"
#include <iostream>

void Client::runSenderThread()
{
	while (connected) {
		std::unique_lock<std::mutex> lock(mutex);
		conditionVariable.wait(lock);
		// Check if we are still connected
		if (!connected) return;
		// We have the lock and can process the first element in the messagesToBeSent vector.
		std::unique_ptr<SnackerEngine::SERPMessage> message = nullptr;
		if (!messagesToBeSent.empty()) {
			message = std::move(messagesToBeSent.front());
			messagesToBeSent.pop();
		}
		// Before we send, unlock the queue again, st. other threads don't have to wait
		lock.unlock();
		// Now we can send the message
		endpoint.finalizeAndSendMessage(*message, false);
		std::cout << "SENT MESSAGE!" << std::endl;
		// Check if there are now more messages we can send
		while (true) {
			std::unique_ptr<SnackerEngine::SERPMessage> message = nullptr;
			{
				std::lock_guard lockGuard(mutex);
				if (!messagesToBeSent.empty()) {
					message = std::move(messagesToBeSent.front());
					messagesToBeSent.pop();
				}
			}
			// If we are disconnected, stop the thread
			if (!connected) return;
			// If the queue was empty, leave the loop
			if (!message) break;
			// Else just keep sending messages
			endpoint.finalizeAndSendMessage(*message, false);
			std::cout << "SENT MESSAGE!" << std::endl;
		}
	}
}

void Client::disconnect()
{
	connected = false;
	conditionVariable.notify_one();
	senderThread.join();
}

void Client::sendMesage(std::unique_ptr<SnackerEngine::SERPMessage> message)
{
	{
		std::lock_guard lockGuard(mutex);
		messagesToBeSent.push(std::move(message));
	}
	conditionVariable.notify_one();
}

Client::Client(SnackerEngine::SocketTCP socket, SnackerEngine::SERPID serpID)
	: endpoint{ std::move(socket) }, serpID{ serpID }, messagesToBeSent{}, senderThread{}, receiverThread{}, 
	mutex{}, conditionVariable{}, connected{ true }, receiverThreadFinished{ false}, fileDescriptorRecievingMessages {}
{
	SnackerEngine::setToNonBlocking(endpoint.getTCPEndpoint().getSocket());
}

Client::~Client()
{
	receiverThread.join();
}