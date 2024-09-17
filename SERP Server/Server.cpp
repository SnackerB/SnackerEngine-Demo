#include "Server.h"
#include <exception>
#include <iostream>
#include <WinSock2.h>

#include "Network\SERP\SERPEndpoint.h"
#include "Network\SERP\SERP.h"
#include "Network\SERP\SERPID.h"
#include "Utility\Formatting.h"

bool SERPServer::isValidSerpID(SnackerEngine::SERPID id)
{
	if (id > 9999) return false;
	for (const auto& client : clients) {
		if (client.getSerpID() == id) return false;
	}
	return true;
}

void SERPServer::connectNewClient(SnackerEngine::SocketTCP socket)
{
	// First we check if a client with the given address alredy has connected
	for (auto& client : clients) {
		if (SnackerEngine::compare(client.getEndpoint().getTCPEndpoint().getSocket().addr, socket.addr)) {
			std::cout << "client already exists!" << std::endl;
			return;
		}
	}
	bool success = false;
	SnackerEngine::SERPID serpID = 0;
	for (unsigned i = 0; i < numberOfRetriesSerpID; ++i) {
		serpID = SnackerEngine::getRandomSerpID();
		if (isValidSerpID(serpID)) {
			success = true;
			break;
		}
	}
	if (success) {
		clients.push_back(Client(std::move(socket), serpID));
		pollFileDescriptors.push_back(pollfd(clients.back().getEndpoint().getTCPEndpoint().getSocket().sock, POLLRDNORM, NULL));
		SnackerEngine::setToNonBlocking(clients.back().getEndpoint().getTCPEndpoint().getSocket());
		std::cout << "new client has connected!" << std::endl;
	}
}

void SERPServer::disconnectClient(unsigned index)
{
	if (index < clients.size()) {
		clients.erase(clients.begin() + index);
		pollFileDescriptors.erase(pollFileDescriptors.begin() + index + 1);
	}
}

void SERPServer::handleIncomingMessage(Client& client)
{
	auto result = client.getEndpoint().receiveMessages();
	for (auto& message : result) {
		if (message->isRequest()) {
			handleRequest(client, static_cast<SnackerEngine::SERPRequest&>(*message));
			break;
		}
		else {
			handleResponse(client, static_cast<SnackerEngine::SERPResponse&>(*message));
			break;
		}
	}
}

void SERPServer::handleRequest(Client& client, SnackerEngine::SERPRequest& request)
{
	if (request.getHeader().destination == 0) {
		// The message is directed at the server!
		handleRequestToServer(client, request);
	}
	else {
		// Check if the given client is connected
		std::optional<std::size_t> clientIndex = findClientIndex(request.getHeader().destination);
		if (!clientIndex.has_value()) {
			sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + SnackerEngine::to_string(request.getHeader().destination) + " is currently connected.");
			std::cout << "no client with serpID " << request.getHeader().destination << " is currently connected." << std::endl;
		}
		else {
			std::cout << "relaying request to " << request.getHeader().destination << "!" << std::endl;
			relayRequest(client, clients[clientIndex.value()], request);
		}
	}
}

void SERPServer::handleRequestToServer(Client& client, SnackerEngine::SERPRequest& request)
{
	std::vector<std::string> path = SnackerEngine::SERPRequest::splitTargetPath(request.target);
	if (path.size() <= 2 && request.getRequestStatusCode() == SnackerEngine::RequestStatusCode::GET) {
		if (path.back() == "ping") {
			answerPingRequest(request, client);
			std::cout << "ping request detected!" << std::endl;
			return;
		}
		else if (path.back() == "serpID") {
			answerSerpIDRequest(request, client);
			std::cout << "serpID request detected!" << std::endl;
			return;
		}
		else if (path.size() == 2 && path[0] == "clients") {
			answerClientExistsRequest(request, client, path[1]);
			std::cout << "client exists request detected!" << std::endl;
			return;
		}
	}
	sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::NOT_FOUND, ("Did not find target \"" + request.target + "\""));
	std::cout << "Did not find target \"" << request.target << "\"" << std::endl;
}

void SERPServer::handleResponse(Client& client, SnackerEngine::SERPResponse& response)
{
	// Check if the given client is connected
	std::optional<std::size_t> clientIndex = findClientIndex(response.getHeader().destination);
	if (!clientIndex.has_value()) {
		std::cout << "no client with serpID " << response.getHeader().destination << " is currently connected." << std::endl;
	}
	else {
		std::cout << "relaying response to " << response.getHeader().destination << "!" << std::endl;
		relayResponse(client, clients[clientIndex.value()], response);
	}
}

std::optional<std::size_t> SERPServer::findClientIndex(SnackerEngine::SERPID serpID)
{
	for (std::size_t i = 0; i < clients.size(); ++i) {
		if (clients[i].getSerpID() == serpID) return i;
	}
	return {};
}

void SERPServer::answerPingRequest(const SnackerEngine::SERPRequest& request, Client& client)
{
	sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::OK, "");
}

void SERPServer::answerSerpIDRequest(const SnackerEngine::SERPRequest& request, Client& client)
{
	sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::OK, SnackerEngine::to_string(client.getSerpID()));
}

void SERPServer::answerClientExistsRequest(const SnackerEngine::SERPRequest& request, Client& client, const std::string& requestedClient)
{
	auto requestedClientID = SnackerEngine::from_string<SnackerEngine::SERPID>(requestedClient);
	if (requestedClientID.has_value()) {
		auto requestedClientIndex = findClientIndex(requestedClientID.value());
		if (requestedClientIndex.has_value()) {
			sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::OK, "");
		}
		else {
			sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + requestedClient + " is currently connected!");
		}
	}
	else {
		sendMessageResponse(request, client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "\"" + requestedClient + "\" is not a valid SerpID!");
	}
}

void SERPServer::sendMessageResponse(const SnackerEngine::SERPRequest& request, Client& client, SnackerEngine::ResponseStatusCode responseStatusCode, const std::string& message)
{
	SnackerEngine::SERPResponse response(request, responseStatusCode, SnackerEngine::Buffer(message));
	client.getEndpoint().sendMessage(response);
}

bool SERPServer::prepareForRelay(SnackerEngine::SERPMessage& message, Client& source)
{
	// Check if the source is correct
	if (message.getHeader().source != source.getSerpID()) {
		if (message.isRequest()) {
			sendMessageResponse(static_cast<SnackerEngine::SERPRequest&>(message), source, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "Attempted to relay message but gave incorrect serpID as source!");
		}
		return false;
	}
	return true;
}

void SERPServer::relayRequest(Client& source, Client& destination, SnackerEngine::SERPRequest& request)
{
	if (!prepareForRelay(static_cast<SnackerEngine::SERPMessage&>(request), source)) return;
	// Relay message
	destination.getEndpoint().sendMessage(request, false);
}

void SERPServer::relayResponse(Client& source, Client& destination, SnackerEngine::SERPResponse& response)
{
	if (!prepareForRelay(response, source)) return;
	// Relay message
	destination.getEndpoint().sendMessage(response, false);
}

SERPServer::SERPServer()
	: clients{}, pollFileDescriptors{}, incomingRequestSocket{}, storageBuffer(100'000)
{
	auto result = SnackerEngine::createSocketTCP(SnackerEngine::getSERPServerPort());
	if (result.has_value()) {
		incomingRequestSocket = std::move(result.value());
		pollFileDescriptors = { pollfd(incomingRequestSocket.sock, POLLRDNORM, NULL)};
	}
	else throw std::exception("Could not create incomingRequestSocket!");
}

SERPServer::~SERPServer()
{
}

// Helper function that throws an exception if the given revent is an error
void checkForPollError(SHORT revents)
{
	if (revents & POLLERR) {
		throw std::exception("POLLERR occured!");
	}
	if (revents & POLLNVAL) {
		throw std::exception("POLLNVAL occured");
	}
}

void SERPServer::run()
{
	if (!SnackerEngine::markAsListen(incomingRequestSocket)) throw std::exception("Could not mark incomingRequestSocket as listening!");
	// main loop
	while (true) {
		// First process events
		int result = WSAPoll(&pollFileDescriptors[0], pollFileDescriptors.size(), 1000);
		if (result == SOCKET_ERROR) throw std::runtime_error(std::string("Socket error with error code " + std::to_string(WSAGetLastError()) + " occured during call to poll()!"));
		if (result > 0) {
			// Handle incomingRequestSocket
			if (pollFileDescriptors[0].revents != NULL) {
				checkForPollError(pollFileDescriptors[0].revents);
				if (pollFileDescriptors[0].revents & POLLHUP) {
					throw std::exception("Connection to incomingRequestSocket failed!");
				}
				if (pollFileDescriptors[0].revents & POLLRDNORM) {
					std::optional<SnackerEngine::SocketTCP> clientSocket = SnackerEngine::acceptConnectionRequest(incomingRequestSocket);
					if (clientSocket.has_value()) {
						connectNewClient(std::move(clientSocket.value()));
					}
				}
			}
			// Handle remaining sockets
			for (unsigned i = 1; i < pollFileDescriptors.size(); ++i) {
				if (pollFileDescriptors[i].revents & POLLNVAL) {
					throw std::exception("POLLNVAL occured");
				}
				else if (pollFileDescriptors[i].revents & POLLERR) {
					// Client socket disconnected with error
					disconnectClient(i - 1);
					std::cout << "client has disconnected with error!" << std::endl;
				}
				else if (pollFileDescriptors[i].revents & POLLHUP) {
					// Client has disconnected
					disconnectClient(i - 1);
					std::cout << "client has disconnected!" << std::endl;
				}
				else if (pollFileDescriptors[i].revents & POLLRDNORM) {
					// Client has sent a message
					std::cout << "client has sent a message!" << std::endl;
					handleIncomingMessage(clients[static_cast<std::size_t>(i) - 1]);
				}
			}
		}
		else {
			std::cout << "no event detected!" << std::endl;
		}
	}
}
