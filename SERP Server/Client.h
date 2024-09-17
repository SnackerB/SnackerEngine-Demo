#pragma once

#include "Network\TCP\TCP.h"
#include "Network\SERP\SERPEndpoint.h"

class Client
{
private:
	/// Each client has a SERP endpoint
	SnackerEngine::SERPEndpoint endpoint;
	/// The SERPID of this client
	SnackerEngine::SERPID serpID;
public:
	Client(SnackerEngine::SocketTCP socket, SnackerEngine::SERPID serpID)
		: endpoint(std::move(socket)), serpID(serpID) {}
	/// Getters
	SnackerEngine::SERPID getSerpID() const { return serpID; }
	SnackerEngine::SERPEndpoint& getEndpoint() { return endpoint; }
};