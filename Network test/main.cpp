#include "Network\SERP2\SERP2.h"

int main()
{
	SnackerEngine::SERPRequest request(420, SnackerEngine::RequestStatusCode::GET, "pictures/cats/cute_cat.jpg");
	request.finalize();
	SnackerEngine::Buffer buffer = request.serialize();
	if (buffer.size() >= sizeof(SnackerEngine::SERPHeader)) {
		std::optional<SnackerEngine::SERPHeader> header = SnackerEngine::SERPHeader::parse(buffer.getBufferView(0, sizeof(SnackerEngine::SERPHeader)));
		if (header.has_value()) {
			std::unique_ptr<SnackerEngine::SERPMessage> message = SnackerEngine::SERPMessage::parse(header.value(), buffer.getBufferView(sizeof(SnackerEngine::SERPHeader)));
			if (message) {
				// Response
				SnackerEngine::SERPResponse response(*static_cast<SnackerEngine::SERPRequest*>(message.get()), SnackerEngine::ResponseStatusCode::OK, SnackerEngine::Buffer("This is a cute cat picture :)"));
				response.finalize();
				buffer = response.serialize();
				if (buffer.size() >= sizeof(SnackerEngine::SERPHeader)) {
					header = SnackerEngine::SERPHeader::parse(buffer.getBufferView(0, sizeof(SnackerEngine::SERPHeader)));
					if (header.has_value()) {
						message = SnackerEngine::SERPMessage::parse(header.value(), buffer.getBufferView(sizeof(SnackerEngine::SERPHeader)));
					}
				}
			}
		}
	}
}