#pragma once

#include <vector>

#include "Client.h"
#include "Utility\Buffer.h"
#include "Network\SERP\SERP.h"
#include "Network\SERP\SERPEndpoint.h"

class SERPServer
{
private:
	/// Collection of Clients
	std::vector<Client> clients;
	/// Collection of pollFileDescriptors for ease of use of the poll() function. Assumes that 
	/// new clients connect only rarely
	std::vector<pollfd> pollFileDescriptors;
	/// TCP Socket for accepting incoming connection requests
	SnackerEngine::SocketTCP incomingRequestSocket;
	/// A buffer that is used as temporary storage
	SnackerEngine::Buffer storageBuffer;
	/// How many tries are done to obtain a valid serpID
	static constexpr unsigned numberOfRetriesSerpID = 10;
private:
	/// Check if the given SERPID is valid
	bool isValidSerpID(SnackerEngine::SERPID id);
	/// Tries to connect a new client with the given socket
	void connectNewClient(SnackerEngine::SocketTCP socket);
	/// Disconnects the client at the given index
	void disconnectClient(unsigned index);
	/// Receives a message from the given socket
	void handleIncomingMessage(Client& client);
	/// Handles an incoming HTTP Request
	void handleRequest(Client& client, SnackerEngine::SERPRequest& request);
	/// Handles an incoming HTTP Request that is directed at the server as per SERP protocol
	void handleRequestToServer(Client& client, SnackerEngine::SERPRequest& request);
	/// Handles an incoming HTTP Response
	void handleResponse(Client& client, SnackerEngine::SERPResponse& response);
	/// Finds the index to the client with the given serpID, if it exists
	std::optional<std::size_t> findClientIndex(SnackerEngine::SERPID serpID);
	/// Answers a ping request of the given client
	void answerPingRequest(const SnackerEngine::SERPRequest& request, Client& client);
	/// Answers a serpID request of the given client
	void answerSerpIDRequest(const SnackerEngine::SERPRequest& request, Client& client);
	/// Answers a doesClientExist request from the given client. The serpID of the requested client
	/// is given as a string
	void answerClientExistsRequest(const SnackerEngine::SERPRequest& request, Client& client, const std::string& requestedClient);
	/// Sends a response with the given status code and a string in the message body to the given socket
	void sendMessageResponse(const SnackerEngine::SERPRequest& request, Client& client, SnackerEngine::ResponseStatusCode responseStatusCode, const std::string& message);
	/// Checks if the given message is ok to relay, and changes the header structure accordingly
	bool prepareForRelay(SnackerEngine::SERPMessage& message, Client& source);
	/// Relays the HTTP request from the source client to the destination client
	void relayRequest(Client& source, Client& destination, SnackerEngine::SERPRequest& request);
	/// Relays the HTTP response from the source client to the destination client
	void relayResponse(Client& source, Client& destination, SnackerEngine::SERPResponse& response);
public:
	/// Constructor
	SERPServer();
	/// Destructor
	~SERPServer();
	/// Starts the server main loop
	void run();
};