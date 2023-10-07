#pragma once

#include "Network\TCP\TCP.h"
#include "Network\SERP\SERPID.h"
#include "Network\HTTP\HTTP.h"

class Client
{
private:
	/// Each client has a HTTP endpoint
	SnackerEngine::HTTPEndpoint httpEndpoint;
	/// The SERPID of this client
	SnackerEngine::SERPID serpID;
public:
	Client(SnackerEngine::SocketTCP socket, SnackerEngine::SERPID serpID)
		: httpEndpoint(std::move(socket), false), serpID(serpID) {}
	/// Getters
	SnackerEngine::SERPID getSerpID() const { return serpID; }
	SnackerEngine::HTTPEndpoint& getEndpoint() { return httpEndpoint; }
	const SnackerEngine::HTTPEndpoint& getEndpoint() const { return httpEndpoint; }
};