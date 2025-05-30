#pragma once
#include "Client.h"
#include <unordered_map>

class Server 
{
private:
	/// Timeout in ms for poll file descriptors
	unsigned pollFdTimeout = 1000;
	/// Number of retries for generating new SerpID
	unsigned numberOfRetriesSerpID = 10;
	/// Mutex for printing to console
	std::mutex printToConsoleMutex;
	/// Map of connected clients, with mutex for thread safe access
	std::unordered_map<unsigned, std::shared_ptr<Client>> clients;
	std::mutex clientsMapMutex;
	/// Vector of disconnected clients where the receiver thread hasn't yet ended
	std::vector<std::shared_ptr<Client>> disconnectedClients;
	/// Thread safe helper function for writing messages to the console/output
	void printMessage(const std::string& message);
	/// Thread safe helper function that looks for a client with the given SerpID and returns a shared pointer to the client
	/// (or nullptr if the client is not connected)
	std::shared_ptr<Client> getClient(SnackerEngine::SERPID serpID);
	/// Socket for accepting incoming requests
	SnackerEngine::SocketTCP incomingConnectRequestSocket;
	/// File descriptor for incoming requests
	pollfd incomingRequestFileDescriptor;
	/// Thread safe helper function for connecting a new client and assigning a new serpID.
	void connectClient(SnackerEngine::SocketTCP socket);
	/// Thread safe helper function for removing a client from the clients map
	void disconnectClient(SnackerEngine::SERPID serpID);
	/// Helper function that sends the given response to the given client (by putting it in the appropriate queue. The
	/// actual sending is then done by the sender thread of the appropriate client).
	void sendMessageResponse(const SnackerEngine::SERPRequest& request, Client& client, SnackerEngine::ResponseStatusCode responseStatusCode, const std::string& message);
	/// Helper function that prepares a message for relay. Returns false if the message is in any way invalid.
	bool prepareForRelay(const SnackerEngine::SERPMessage& message, Client& source);
	/// Helper function that trys to relay a request from client to client.
	void relayRequest(Client& source, SnackerEngine::SERPID destination, std::unique_ptr<SnackerEngine::SERPMessage> request);
	/// Helper function that trys to relay a response from client to client.
	void relayResponse(Client& source, SnackerEngine::SERPID destination, std::unique_ptr<SnackerEngine::SERPMessage> response);
	/// Helper function that answers a request from a client that asks if another client exists.
	void answerClientExistsRequest(Client& client, const SnackerEngine::SERPRequest& request, const std::string& requestedClient);
	/// Helper function that handles an incoming request to the server (thats us!)
	void handleIncomingRequestToServer(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> request);
	/// Helper function that handles an incoming request from the given client
	void handleIncomingRequest(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> request);
	/// Helper function that handles an incoming response from the given client
	void handleIncomingResponse(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> response);
	/// Helper function that receives a message from a client and relays/answers the message.
	void handleIncomingMessage(Client& client);
	/// Helper function that runs a receiver thread on the given client, listening for messages and relaying/answering them.
	void runReceiverThread(std::shared_ptr<Client> client);
public:
	/// Constructor
	Server();
	/// Runs the main loop, listening for connection requests and invoking new threads for connected clients.
	void run();
	/// Destructor
	~Server();


};