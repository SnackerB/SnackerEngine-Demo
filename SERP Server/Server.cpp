#include "Server.h"
#include <iostream>
#include "Utility/Formatting.h"

void Server::printMessage(const std::string& message)
{
	// Acquire lock
	std::lock_guard lockGuard(printToConsoleMutex);
	// Write to console
	std::cout << message << std::endl;
}

std::shared_ptr<Client> Server::getClient(SnackerEngine::SERPID serpID)
{
	// Acquire lock
	std::lock_guard lockGuard(clientsMapMutex);
	// Look for client with the given ID
	auto result = clients.find(static_cast<unsigned int>(serpID));
	// If the client was found, return the client, else return nullptr
	if (result == clients.end()) return nullptr;
	else return result->second;
}

void Server::connectClient(SnackerEngine::SocketTCP socket)
{
	std::shared_ptr<Client> newClient = nullptr;
	SnackerEngine::SERPID newSerpID = static_cast<unsigned int>(0);
	{
		// Acquire lock
		std::lock_guard lock(clientsMapMutex);
		// First we check if a client with the given address alredy has connected
		for (auto& client : clients) {
			if (SnackerEngine::compare(client.second->endpoint.getTCPEndpoint().getSocket().addr, socket.addr)) {
				printMessage("Detected new connection request from client that was already connected.");
				return;
			}
		}
		bool success = false;
		for (unsigned i = 0; i < numberOfRetriesSerpID; ++i) {
			newSerpID = SnackerEngine::getRandomSerpID();
			auto result = clients.find(static_cast<unsigned int>(newSerpID));
			if (result == clients.end()) {
				success = true;
				break;
			}
		}
		if (success) {
			newClient = std::make_shared<Client>(std::move(socket), newSerpID);
			clients.insert(std::make_pair<>(static_cast<unsigned int>(newSerpID), newClient));
			// Start sender and receiver threads
			newClient->senderThread = std::thread(&Client::runSenderThread, newClient.get());
			newClient->receiverThread = std::thread(&Server::runReceiverThread, this, newClient);
		}
	}
	if (newClient) {
		printMessage("New client with serpID " + SnackerEngine::to_string(newSerpID) + " connected.");
	}
}

void Server::disconnectClient(SnackerEngine::SERPID serpID)
{
	// Acquire lock
	std::lock_guard lockGuard(clientsMapMutex);
	// Erase client from clients map
	auto client = clients.find(static_cast<unsigned int>(serpID));
	if (client != clients.end()) {
		client->second->disconnect();
		disconnectedClients.push_back(client->second);
		clients.erase(client);
	}
}

void Server::sendMessageResponse(const SnackerEngine::SERPRequest& request, Client& client, SnackerEngine::ResponseStatusCode responseStatusCode, const std::string& message, SnackerEngine::SERPID sourceID)
{
	// Create response with message buffer, etc.
	std::unique_ptr<SnackerEngine::SERPMessage> messageSERP = std::make_unique<SnackerEngine::SERPResponse>(request, responseStatusCode, SnackerEngine::Buffer(message));
	messageSERP->getHeader().source = sourceID;
	// This function is already thread safe
	client.sendMesage(std::move(messageSERP));
}

bool Server::prepareForRelay(const SnackerEngine::SERPMessage& message, Client& source)
{
	// Check if the source is correct
	if (message.getHeader().source != source.serpID) {
		if (message.isRequest()) {
			if (message.getHeader().getMultiSendFlag()) {
				for (SnackerEngine::SERPID destination : message.getDestinations()) {
					sendMessageResponse(static_cast<const SnackerEngine::SERPRequest&>(message), source, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "Attempted to relay message but gave incorrect serpID as source!", destination);
					printMessage("Failed to relay request from client " + SnackerEngine::to_string(source.serpID) + " due to giving an incorrect serpID as source.");
				}
			}
			else {
				sendMessageResponse(static_cast<const SnackerEngine::SERPRequest&>(message), source, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "Attempted to relay message but gave incorrect serpID as source!", message.getHeader().destination);
				printMessage("Failed to relay request from client " + SnackerEngine::to_string(source.serpID) + " due to giving an incorrect serpID as source.");
			}
		}
		return false;
	}
	return true;
}

void Server::relayRequest(Client& source, SnackerEngine::SERPID destination, std::unique_ptr<SnackerEngine::SERPMessage> request)
{
	if (!prepareForRelay(*request, source)) return;
	// Check if the destination client is connected and relay the request if it is!
	std::shared_ptr<Client> destinationClient = getClient(destination);
	if (destinationClient) {
		destinationClient->sendMesage(std::move(request));
		printMessage("Relayed request from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ".");
	}
	else {
		// The requested client is not connected. Send this information to sender.
		sendMessageResponse(static_cast<const SnackerEngine::SERPRequest&>(*request), source, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + SnackerEngine::to_string(destination) + " is currently connected.", destination);
		printMessage("Tried to relay request from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ", but client " + SnackerEngine::to_string(destination) + " was not connected.");
	}
}

void Server::relayRequestMulti(Client& source, std::unique_ptr<SnackerEngine::SERPMessage> request)
{
	if (!prepareForRelay(*request, source)) return;
	// Go through destinations and send seperately
	std::unordered_set<uint16_t> destinations = request->getDestinations();
	request->getHeader().setMultiSendFlag(false);
	request->clearDestinations();
	for (auto destination : destinations) {
		// Check if the destination client is connected and relay the request if it is!
		std::shared_ptr<Client> destinationClient = getClient(destination);
		if (destinationClient) {
			request->getHeader().destination = destination;
			destinationClient->sendMesage(std::make_unique<SnackerEngine::SERPRequest>(*static_cast<SnackerEngine::SERPRequest*>(request.get())));
			printMessage("Relayed request from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ".");
		}
		else {
			// The requested client is not connected. Send this information to sender.
			sendMessageResponse(static_cast<const SnackerEngine::SERPRequest&>(*request), source, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + SnackerEngine::to_string(destination) + " is currently connected.", destination);
			printMessage("Tried to relay request from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ", but client " + SnackerEngine::to_string(destination) + " was not connected.");
		}
	}
}

void Server::relayResponse(Client& source, SnackerEngine::SERPID destination, std::unique_ptr<SnackerEngine::SERPMessage> response)
{
	if (!prepareForRelay(*response, source)) return;
	// Check if the destination client is connected and relay the response if it is!
	std::shared_ptr<Client> destinationClient = getClient(destination);
	if (destinationClient) {
		destinationClient->sendMesage(std::move(response));
		printMessage("Relayed response from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ".");
	}
	else {
		// The requested client is not connected.
		printMessage("Tried to relay response from client " + SnackerEngine::to_string(source.serpID) + " to client " + SnackerEngine::to_string(destination) + ", but client " + SnackerEngine::to_string(destination) + " was not connected.");
	}
}

void Server::answerClientExistsRequest(Client& client, const SnackerEngine::SERPRequest& request, const std::string& requestedClient)
{
	auto requestedClientID = SnackerEngine::from_string<SnackerEngine::SERPID>(requestedClient);
	if (requestedClientID.has_value()) {
		bool success = false;
		{
			// Acquire lock
			std::lock_guard lockGuard(clientsMapMutex);
			auto result = clients.find(static_cast<unsigned int>(requestedClientID.value()));
			if (result != clients.end()) success = true;
		}
		if (success) {
			sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::OK, "");
			printMessage("Answered clientExists request from client " + SnackerEngine::to_string(client.serpID) + ": Client with serpID " + requestedClient + " is currently connected!");
		}
		else {
			sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + requestedClient + " is currently connected!");
			printMessage("Answered clientExists request from client " + SnackerEngine::to_string(client.serpID) + ": No client with serpID " + requestedClient + " is currently connected!");
		}
	}
	else {
		sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "\"" + requestedClient + "\" is not a valid SerpID!");
		printMessage("Answered clientExists request from client " + SnackerEngine::to_string(client.serpID) + ": \"" + requestedClient + "\" is not a valid SerpID.");
	}
}

void Server::handleIncomingRequestToServer(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> request)
{
	const SnackerEngine::SERPRequest& requestRef = static_cast<const SnackerEngine::SERPRequest&>(*request);
	std::vector<std::string> path = SnackerEngine::SERPRequest::splitTargetPath(requestRef.target);
	if (path.size() <= 2 && requestRef.getRequestStatusCode() == SnackerEngine::RequestStatusCode::GET) {
		if (path.back() == "ping") {
			sendMessageResponse(requestRef, client, SnackerEngine::ResponseStatusCode::OK, "");
			printMessage("Answered ping request from client " + SnackerEngine::to_string(client.serpID) + ".");
			return;
		}
		else if (path.back() == "serpID") {
			sendMessageResponse(requestRef, client, SnackerEngine::ResponseStatusCode::OK, SnackerEngine::to_string(client.serpID));
			printMessage("Answered serpID request from client " + SnackerEngine::to_string(client.serpID) + ".");
			return;
		}
		else if (path.size() == 2 && path[0] == "clients") {
			answerClientExistsRequest(client, requestRef, path[1]);
			return;
		}
	}
	sendMessageResponse(requestRef, client, SnackerEngine::ResponseStatusCode::NOT_FOUND, ("Did not find target \"" + requestRef.target + "\""));
	printMessage("Client sent request with invalid target \"" + requestRef.target + "\" to server.");
}

void Server::handleIncomingRequest(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> request)
{
	if (request->getHeader().getMultiSendFlag()) {
		// The message is directed at other clients. Try to multisend relay.
		relayRequestMulti(client, std::move(request));
	}
	else if (request->getHeader().destination == 0) {
		// The message is directed at the server!
		handleIncomingRequestToServer(client, std::move(request));
	}
	else {
		// The message is directed to another client. Try to relay.
		SnackerEngine::SERPID destination = request->getHeader().destination;
		relayRequest(client, destination, std::move(request));
	}
}

void Server::handleIncomingResponse(Client& client, std::unique_ptr<SnackerEngine::SERPMessage> response)
{
	SnackerEngine::SERPID destination = response->getHeader().destination;
	relayResponse(client, destination, std::move(response));
}

void Server::handleIncomingMessage(Client& client)
{
	std::vector<std::unique_ptr<SnackerEngine::SERPMessage>> result = std::move(client.endpoint.receiveMessages());
	for (unsigned int i = 0; i < result.size(); ++i) {
		if (result[i]->isRequest()) {
			handleIncomingRequest(client, std::move(result[i]));
		}
		else {
			handleIncomingResponse(client, std::move(result[i]));
		}
	}
}

void Server::runReceiverThread(std::shared_ptr<Client> client)
{
	// Create poll file descriptor for listening to received messages on client socket
#ifdef _WINDOWS
	pollfd clientPollFD(client->endpoint.getTCPEndpoint().getSocket().sock, POLLRDNORM, NULL);
#endif // _WINDOWS
#ifdef _LINUX
	pollfd clientPollFD(client->endpoint.getTCPEndpoint().getSocket().sock, POLLRDNORM, 0);
#endif // _LINUX
	while (client->connected) {
		// Listen for message
#ifdef _WINDOWS
		int result = WSAPoll(&clientPollFD, 1, pollFdTimeout);
		if (result == SOCKET_ERROR) {
#endif // _WINDOWS
#ifdef _LINUX
		int result = poll(&clientPollFD, 1, pollFdTimeout);
		if (result == -1) {
#endif // _LINUX
		
			// Write error to chat, disconnect client and end thread
#ifdef _WINDOWS
			printMessage("Socket error with error code " + SnackerEngine::to_string(WSAGetLastError()) + " occured during call to poll() on on client with SERPID" + SnackerEngine::to_string(client->serpID) + "!");
#endif // _WINDOWS
#ifdef _LINUX
			printMessage("Socket error with error code " + std::string(strerror(errno)) + " occured during call to poll() on on client with SERPID" + SnackerEngine::to_string(client->serpID) + "!");
#endif // _LINUX
			disconnectClient(client->serpID);
			break;
		}
		else if (clientPollFD.revents & POLLNVAL) {
			// Write error to chat, disconnect client and end thread
			printMessage("Received error POLLNVAL during call to poll() on on client with SERPID" + SnackerEngine::to_string(client->serpID) + "!");
			disconnectClient(client->serpID);
			break;
		}
		else if (clientPollFD.revents & POLLERR) {
			// Client socket disconnected with error. Write error to chat, disconnect client and end thread
			printMessage("Client with SERPID " + SnackerEngine::to_string(client->serpID) + " disconnected with error.");
			disconnectClient(client->serpID);
			break;
		}
		else if (clientPollFD.revents & POLLHUP) {
			// Client has disconnected
			printMessage("Client with SERPID " + SnackerEngine::to_string(client->serpID) + " disconnected.");
			disconnectClient(client->serpID);
			break;
		}
		else if (clientPollFD.revents & POLLRDNORM) {
			// Client has sent a message
			printMessage("Client with SERPID " + SnackerEngine::to_string(client->serpID) + " has sent a message.");
			handleIncomingMessage(*client);
		}
	}
	client->receiverThreadFinished = true;
}

Server::Server()
	: printToConsoleMutex{}, clients{}, clientsMapMutex{}, incomingConnectRequestSocket{}, incomingRequestFileDescriptor{}
{
	// Initialize incomingConnectRequestSocket socket
	auto result = SnackerEngine::createSocketTCP(SnackerEngine::getSERPServerPort());
	if (result.has_value()) {
		incomingConnectRequestSocket = std::move(result.value());
#ifdef _WINDOWS
		incomingRequestFileDescriptor = pollfd(incomingConnectRequestSocket.sock, POLLRDNORM, NULL);
#endif // _WINDOWS
#ifdef _LINUX
		incomingRequestFileDescriptor = pollfd(incomingConnectRequestSocket.sock, POLLRDNORM, 0);
#endif // _LINUX
	}
	else throw std::runtime_error("Could not create incomingConnectRequestSocket!");
}

void Server::run()
{
	if (!SnackerEngine::markAsListen(incomingConnectRequestSocket)) throw std::runtime_error("Could not mark incomingConnectRequestSocket as listening!");
	printMessage("Started Server!");
	while (true) {
		// First process events
#ifdef _WINDOWS
		int result = WSAPoll(&incomingRequestFileDescriptor, 1, 5000);
		if (result == SOCKET_ERROR) throw std::runtime_error(std::string("Socket error with error code " + std::to_string(WSAGetLastError()) + " occured during call to poll()!"));
		if (incomingRequestFileDescriptor.revents != NULL) {
#endif // _WINDOWS
#ifdef _LINUX
		int result = poll(&incomingRequestFileDescriptor, 1, 5000);
		if (result == -1) throw std::runtime_error(std::string("Socket error with error code ") + std::string(strerror(errno)) + std::string(" occured during call to poll()!"));
		if (incomingRequestFileDescriptor.revents != 0) {
#endif // _LINUX
			if (incomingRequestFileDescriptor.revents & POLLRDNORM) {
				// Connect new client
				std::optional<SnackerEngine::SocketTCP> clientSocket = SnackerEngine::acceptConnectionRequest(incomingConnectRequestSocket);
				if (clientSocket.has_value()) {
					connectClient(std::move(clientSocket.value()));
				}
			}
			else if (incomingRequestFileDescriptor.revents & POLLERR) {
				throw std::runtime_error(std::string("POLLERR in incomingRequestFileDescriptor."));
			}
			else if (incomingRequestFileDescriptor.revents & POLLNVAL) {
				throw std::runtime_error(std::string("POLLNVAL in incomingRequestFileDescriptor."));
			}
			else if (incomingRequestFileDescriptor.revents & POLLHUP) {
				throw std::runtime_error(std::string("POLLHUP in incomingRequestFileDescriptor."));
			}
		}
		int numberOfConnectedClients = 0;
		{
			std::lock_guard lock(clientsMapMutex);
			numberOfConnectedClients = static_cast<int>(clients.size());
		}
		// Try to delete disconnected clients
		for (auto it = disconnectedClients.begin(); it != disconnectedClients.end();) {
			if ((*it)->receiverThreadFinished) {
				it = disconnectedClients.erase(it);
			}
			else {
				it++;
			}
		}
		printMessage("Currently " + SnackerEngine::to_string(numberOfConnectedClients) + " clients connected.");
		if (!disconnectedClients.empty()) {
			printMessage("Currently " + SnackerEngine::to_string(disconnectedClients.size()) + " clients waiting for disconnect.");
		}
	}
}

Server::~Server()
{
	// TODO: Write destructor.
}
