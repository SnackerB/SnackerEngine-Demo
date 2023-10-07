#include "Server.h"
#include <exception>
#include <iostream>
#include <WinSock2.h>

#include "Network\SERP\SERP.h"

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
	for (const auto& client : clients) {
		if (SnackerEngine::compare(client.getEndpoint().getSocket().addr, socket.addr)) {
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
		pollFileDescriptors.push_back(pollfd(clients.back().getEndpoint().getSocket().sock, POLLRDNORM, NULL));
		SnackerEngine::setToNonBlocking(clients.back().getEndpoint().getSocket());
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
	auto result = client.getEndpoint().receiveMessages(0.0);
	if (result.empty()) sendMessageResponse(client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "HTTP version not supported or invalid message format!");
	for (const auto& message : result) {
		switch (message->getMessageType())
		{
		case SnackerEngine::HTTPMessage::MessageType::REQUEST:
		{
			SnackerEngine::HTTPRequest& request = static_cast<SnackerEngine::HTTPRequest&>(*message);
			handleRequest(client, request);
			break;
		}
		case SnackerEngine::HTTPMessage::MessageType::RESPONSE:
		{
			SnackerEngine::HTTPResponse& response = static_cast<SnackerEngine::HTTPResponse&>(*message);
			handleResponse(client, response);
			break;
		}
		}
	}
}

void SERPServer::handleRequest(const Client& client, SnackerEngine::HTTPRequest& request)
{
	// Look if a destination SERPID is given
	std::optional<std::string_view> destination = request.getHeaderValue("destinationID");
	if (destination.has_value()) {
		// Parse destinationID
		std::optional<SnackerEngine::SERPID> destinationID = SnackerEngine::parseSERPID(std::string(destination.value()));
		if (!destinationID.has_value()) {
			// Invalid destinationID
			sendMessageResponse(client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "bad request: \"" + std::string(destination.value()) + "\" is not a valid serpID.");
			std::cout << "bad request: \"" << destination.value() << "\" is not a valid serpID." << std::endl;;
		}
		else if (destinationID.value() == 0) {
			// The message is directed at the server!
			handleRequestToServer(client, request);
		}
		else {
			// Check if the given client is connected
			std::optional<std::size_t> clientIndex = findClientIndex(destinationID.value());
			if (!clientIndex.has_value()) {
				sendMessageResponse(client, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + std::string(destination.value()) + " is currently connected.");
				std::cout << "no client with serpID " << destination.value() << " is currently connected." << std::endl;
			}
			else {
				std::cout << "relaying request to " << destinationID.value() << "!" << std::endl;
				relayRequest(client, clients[clientIndex.value()], request);
			}
		}
	}
	else {
		// The message is directed at the server!
		handleRequestToServer(client, request);
	}
}

void SERPServer::handleRequestToServer(const Client& client, SnackerEngine::HTTPRequest& request)
{
	std::vector<std::string> path = request.splitPath();
	if (path.size() <= 2 && request.requestMethod == request.HTTP_GET) {
		if (path.back() == "ping") {
			answerPingRequest(client);
			std::cout << "ping request detected!" << std::endl;
			return;
		}
		else if (path.back() == "serpID") {
			answerSerpIDRequest(client);
			std::cout << "serpID request detected!" << std::endl;
			return;
		}
	}
	sendMessageResponse(client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "bad request: \"" + request.path + "\"");
	std::cout << "bad request: \"" << request.path << "\"" << std::endl;
}

void SERPServer::handleResponse(const Client& client, SnackerEngine::HTTPResponse& response)
{
	// Look at the header structure if the response is valid. If it is valid, there should be a header 'destination' with the+
	// SerpID of the destination client
	std::optional<std::string_view> destination = response.getHeaderValue("destinationID");
	if (!destination.has_value()) {
		sendMessageResponse(client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "bad request: No destination serpID provided in HTTP response.");
		std::cout << "bad request : No destination serpID provided in HTTP response." << std::endl;
		return;
	}
	std::optional<SnackerEngine::SERPID> destinationID = SnackerEngine::parseSERPID(std::string(destination.value()));
	if (!destinationID.has_value()) {
		sendMessageResponse(client, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "bad request: \"" + std::string(destination.value()) + "\" is not a valid serpID.");
		std::cout << "bad request: \"" << destination.value() << "\" is not a valid serpID." << std::endl;
		return;
	}
	// Check if the given client is connected
	std::optional<std::size_t> clientIndex = findClientIndex(destinationID.value());
	if (!clientIndex.has_value()) {
		sendMessageResponse(client, SnackerEngine::ResponseStatusCode::NOT_FOUND, "no client with serpID " + std::string(destination.value()) + " is currently connected.");
		std::cout << "no client with serpID " << destination.value() << " is currently connected." << std::endl;
		return;
	}
	std::cout << "relaying response to " << destination.value() << "!" << std::endl;
	relayResponse(client, clients[clientIndex.value()], response);
}

std::optional<std::size_t> SERPServer::findClientIndex(SnackerEngine::SERPID serpID)
{
	for (std::size_t i = 0; i < clients.size(); ++i) {
		if (clients[i].getSerpID() == serpID) return i;
	}
	return {};
}

void SERPServer::answerPingRequest(const Client& client)
{
	sendMessageResponse(client, SnackerEngine::ResponseStatusCode::OK, "");
}

void SERPServer::answerSerpIDRequest(const Client& client)
{
	sendMessageResponse(client, SnackerEngine::ResponseStatusCode::OK, SnackerEngine::to_string(client.getSerpID()));
}

void SERPServer::sendMessageResponse(const Client& client, SnackerEngine::ResponseStatusCode responseStatusCode, const std::string& message)
{
	SnackerEngine::SERPResponse response(0, client.getSerpID(), 
		SnackerEngine::HTTPResponse(responseStatusCode, {}, message));
	SnackerEngine::sendResponse(client.getEndpoint().getSocket(), response.response);
}

bool SERPServer::prepareForRelay(const Client& source, SnackerEngine::HTTPMessage& message)
{
	std::optional<std::string_view> sourceSerpID = message.getHeaderValue("sourceID");
	if (sourceSerpID.has_value()) {
		// Check if the source is correct
		if (sourceSerpID.value() != SnackerEngine::to_string(source.getSerpID())) {
			sendMessageResponse(source, SnackerEngine::ResponseStatusCode::BAD_REQUEST, "Attempted to relay message but gave incorrect serpID as source!");
			return false;
		}
	}
	else {
		message.headers.push_back(SnackerEngine::HTTPHeader("sourceID", SnackerEngine::to_string(source.getSerpID())));
	}
	return true;
}

void SERPServer::relayRequest(const Client& source, const Client& destination, SnackerEngine::HTTPRequest& request)
{
	if (!prepareForRelay(source, request)) return;
	// Relay message
	SnackerEngine::sendRequest(destination.getEndpoint().getSocket(), request);
}

void SERPServer::relayResponse(const Client& source, const Client& destination, SnackerEngine::HTTPResponse& response)
{
	if (!prepareForRelay(source, response)) return;
	// Relay message
	SnackerEngine::sendResponse(destination.getEndpoint().getSocket(), response);
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
