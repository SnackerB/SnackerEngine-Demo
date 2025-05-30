#include "Network\SERP\SERPEndpoint.h"
#include <mutex>
#include <condition_variable>
#include <queue>

/// This class represents a connected client.
class Client
{
private:
	friend class Server;
	/// The endpoint through which data is both received and sent.
	SnackerEngine::SERPEndpoint endpoint;
	/// The SERPID of this client.
	SnackerEngine::SERPID serpID;
	/// Vector of messages to be sent.
	std::queue<std::unique_ptr<SnackerEngine::SERPMessage>> messagesToBeSent;
	/// threads responsible for sending/receiving messages
	std::thread senderThread;
	std::thread receiverThread;
	/// Mutex and condition variable for thread-safe sending
	std::mutex mutex;
	std::condition_variable conditionVariable;
	/// atomic boolean that is true if the client is currently connected
	std::atomic<bool> connected;
	/// atomic boolean that is set to true when the receiver thread has finished executing
	std::atomic<bool> receiverThreadFinished;
	/// Filedescriptor for receiving messages
	pollfd fileDescriptorRecievingMessages;
	/// Function that is continuously run by a sender thread during the lifetime of the Client.
	void runSenderThread();
	/// Helper function that cleans up loose end when disconnecting a client. Should be
	/// called before the client is deleted.
	void disconnect();
public:
	/// Puts the given message into the messagesToBeSent vector and wakes up the sender thread.
	void sendMesage(std::unique_ptr<SnackerEngine::SERPMessage> message);
	/// Constructor
	Client(SnackerEngine::SocketTCP socket, SnackerEngine::SERPID serpID);
	/// Destructor
	~Client();
	/// Deleted Copy and move constructors and assignment operators
	Client(Client& other) = delete;
	Client(Client&& other) = delete;
	Client& operator=(Client& other) = delete;
	Client& operator=(Client&& other) = delete;
};