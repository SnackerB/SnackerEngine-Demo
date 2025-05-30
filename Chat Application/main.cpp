#include "SnackerEngine\Core\Scene.h"
#include "SnackerEngine\Core\Engine.h"
#include "SnackerEngine\Core\Log.h"
#include "SnackerEngine\Graphics\Renderer.h"
#include "SnackerEngine\AssetManager\AssetManager.h"
#include "Network\Network.h"
#include "SnackerEngine\Gui\GuiManager.h"
#include "Utility\Json.h"
#include "Gui\GuiElements\GuiEditVariable.h"
#include "Network\SERP\SERPID.h"
#include "Gui\GuiElements\GuiButton.h"
#include "Gui\Layouts\VerticalScrollingListLayout.h"
#include "SERP\SerpManager.h"
#include "Core\Log.h"

class ChatScene : public SnackerEngine::Scene
{
private:
	// Guimanager
	SnackerEngine::GuiManager guiManager{};
	// SERPManager
	SnackerEngine::SERPManager serpManager{};
	// Mode we're currently in
	enum Mode {
		MAIN_MENU,
		HOST,
		JOIN
	};
	Mode mode{ Mode::MAIN_MENU };
	/// JSON with gui style
	std::optional<nlohmann::json> guiStyle;
	/// GUI for the main menu
	SnackerEngine::EventHandle eventHandleButtonHostWasPressed{};
	SnackerEngine::EventHandle eventHandleButtonJoinWasPressed{};
	/// GUI for the chatroom
	SnackerEngine::EventHandle eventHandleButtonPostMessageWasPressed{};
	/// Stores the SERPID of the server (in JOIN mode)
	SnackerEngine::SERPID server;
	/// Stores the SERPIDs of all connected clients (in HOST mode)
	std::vector<SnackerEngine::SERPID> clients;
protected:
	/// Draws the scene to the screen. Called by the Engine class!
	void draw() override { guiManager.draw(); }
	/// Posts a message to the chatbox
	void postMessageLocal(const std::string& message, bool myMessage=true);
	/// Sends a message to all connected clients (in HOST mode) or to the server (in JOIN mode)
	void sendMessage(const std::string& message);
	/// Relays a message to all connected clients except the sender (only works in HOST mode)
	void relayMessage(const std::string& message, SnackerEngine::SERPID sender);
	/// Helper function that initializes the main menue.
	void initializeMainMenu();
	/// Helper function that initializes the HOST or JOIN chatroom.
	void initializeChatroom();
	/// Helper function that updates the main menue.
	void updateMainMenu();
	/// Helper function that updates the HOST or JOIN chatroom.
	void updateChatroom();
public:
	/// Initializes the scene (eg adds update functions, loads data, ...). This is called by the engine
	void initialize() override;
	/// Callback function for keyboard input
	void callbackKeyboard(const int& key, const int& scancode, const int& action, const int& mods) override { guiManager.callbackKeyboard(key, scancode, action, mods); }
	/// Callback function for mouse button input
	void callbackMouseButton(const int& button, const int& action, const int& mods) override { guiManager.callbackMouseButton(button, action, mods); }
	/// Callback function for mouse motion
	void callbackMouseMotion(const SnackerEngine::Vec2d& position) override { guiManager.callbackMouseMotion(position); }
	/// Callback function for resizing the window
	void callbackWindowResize(const SnackerEngine::Vec2i& screenDims) override { guiManager.callbackWindowResize(screenDims); }
	/// Callback function for scrolling the mouse wheel
	void callbackMouseScroll(const SnackerEngine::Vec2d& offset) override { guiManager.callbackMouseScroll(offset); }
	/// Callback function for the input of unicode characters
	void callbackCharacterInput(const unsigned int& codepoint) override { guiManager.callbackCharacterInput(codepoint); }
	/// this function get calls regularly by the Engine class.
	void update(const double& dt) override;
	/// Deconstructor
	~ChatScene();
};

void ChatScene::postMessageLocal(const std::string& message, bool myMessage)
{
	// Add message to chatBox GUI
	std::optional<nlohmann::json> textMessageJson = SnackerEngine::Engine::loadJSONRelativeToResourcePath("gui/textMessage.json");
	if (!textMessageJson.has_value()) {
		SnackerEngine::errorLogger << "Couldn't find JSON file \"gui/textMessage.json\".";
		return;
	}
	SnackerEngine::GuiTextBox messageBox(textMessageJson.value(), guiStyle.has_value() ? &guiStyle.value() : nullptr);
	messageBox.setText(message);
	auto chatBox = guiManager.getGuiElement<SnackerEngine::GuiVerticalScrollingListLayout>("chatBox");
	if (chatBox) {
		if (myMessage) {
			messageBox.setBackgroundColor(SnackerEngine::Color4f(0.1f, 0.3f, 0.1f, 1.0f));
			chatBox->registerChild(messageBox, SnackerEngine::AlignmentHorizontal::RIGHT);
		}
		else {
			messageBox.setBackgroundColor(SnackerEngine::Color4f(0.1f, 0.1f, 0.3f, 1.0f));
			chatBox->registerChild(messageBox, SnackerEngine::AlignmentHorizontal::LEFT);
		}
		guiManager.moveElement(std::move(messageBox));
	}
}

void ChatScene::sendMessage(const std::string& message)
{
	if (mode == Mode::HOST) {

	}
	else if (mode == Mode::JOIN) {
		SnackerEngine::SERPRequest request{};
		//serpManager.sendRequest();
	}
	// TODO: Implement
}

void ChatScene::relayMessage(const std::string& message, SnackerEngine::SERPID sender)
{
	// TODO: Implement
}

void ChatScene::initializeMainMenu()
{
	// Load GUI json
	std::optional<nlohmann::json> mainMenuJson = SnackerEngine::Engine::loadJSONRelativeToResourcePath("gui/mainMenu.json");
	guiStyle = SnackerEngine::Engine::loadJSONRelativeToResourcePath("gui/guiStyle.json");
	if (mainMenuJson.has_value()) guiManager.loadAndRegisterJSON(mainMenuJson.value(), guiStyle.has_value() ? &guiStyle.value() : nullptr);
	else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "Couldn't find JSON file \"gui/mainMenu.json\" in resource folder." << SnackerEngine::LOGGER::ENDL;
	// Connect button handles
	auto buttonHost = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonHost");
	if (buttonHost) buttonHost->subscribeToEventButtonPress(eventHandleButtonHostWasPressed);
	else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "GUI from \"gui/mainMenu.json\" doesn't have a button with name \"buttonHost\"." << SnackerEngine::LOGGER::ENDL;
	auto buttonJoin = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonJoin");
	if (buttonJoin) buttonJoin->subscribeToEventButtonPress(eventHandleButtonJoinWasPressed);
	else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "GUI from \"gui/mainMenu.json\" doesn't have a button with name \"buttonJoin\"." << SnackerEngine::LOGGER::ENDL;
}

void ChatScene::initializeChatroom()
{
	std::optional<nlohmann::json> chatRoomJson;
	if (eventHandleButtonHostWasPressed.isActive()) chatRoomJson = SnackerEngine::Engine::loadJSONRelativeToResourcePath("gui/chatRoomHost.json");
	else chatRoomJson = SnackerEngine::Engine::loadJSONRelativeToResourcePath("gui/chatRoomJoin.json");
	if (chatRoomJson.has_value()) guiManager.loadAndRegisterJSON(chatRoomJson.value(), guiStyle.has_value() ? &guiStyle.value() : nullptr);
	// Connect button handle
	auto buttonPostMessage = guiManager.getGuiElement<SnackerEngine::GuiButton>("postMessageButton");
	if (buttonPostMessage) buttonPostMessage->subscribeToEventButtonPress(eventHandleButtonPostMessageWasPressed);
	else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "chatroom GUI doesn't have a button with name \"postMessageButton\"." << SnackerEngine::LOGGER::ENDL;
	// Do special GUI stoff for when we're in HOST mode
	if (mode == Mode::HOST) {
		// Display textBox with SERPID
		auto textBoxSerpID = guiManager.getGuiElement<SnackerEngine::GuiTextBox>("textBoxSerpID");
		if (textBoxSerpID) textBoxSerpID->setText("SerpID: " + SnackerEngine::to_string(serpManager.getSerpID()));
		else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "chatroom GUI doesn't have a text box with name \"textBoxSerpID\"." << SnackerEngine::LOGGER::ENDL;
	}
}

void ChatScene::updateMainMenu()
{
	if (eventHandleButtonHostWasPressed.isActive() || eventHandleButtonJoinWasPressed.isActive()) {
		// Client wants to host or join a chatroom. Thus we need to first connect to the SERP server
		SnackerEngine::SERPManager::ConnectResult connectResult = serpManager.connectToSERPServer();
		// Lock buttons
		auto buttonHost = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonHost");
		if (buttonHost) buttonHost->lock();
		auto buttonJoin = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonJoin");
		if (buttonJoin) buttonJoin->lock();
		// Check result
		if (connectResult.result == SnackerEngine::SERPManager::ConnectResult::Result::SUCCESS) {
			SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN <<"SUCCESS" << SnackerEngine::LOGGER::ENDL;
			// Change mode
			if (eventHandleButtonHostWasPressed.isActive()) mode = Mode::HOST;
			else mode = Mode::JOIN;
			// Load either host or join Scene
			guiManager.clear();
			initialize();
		}
		else if (connectResult.result == SnackerEngine::SERPManager::ConnectResult::Result::ERROR) {
			SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN << "ERROR" << SnackerEngine::LOGGER::ENDL;
			// Display error message
			// TODO
			// Unlock buttons
			auto buttonHost = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonHost");
			if (buttonHost) buttonHost->unlock();
			auto buttonJoin = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonJoin");
			if (buttonJoin) buttonJoin->unlock();
		}
		else {
			SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN << "PENDING" << SnackerEngine::LOGGER::ENDL;
		}
	}
}

void ChatScene::updateChatroom()
{
	if (eventHandleButtonPostMessageWasPressed.isActive()) {
		eventHandleButtonPostMessageWasPressed.reset();
		// TODO: Send message to clients or server
		auto messageEditBox = guiManager.getGuiElement<SnackerEngine::GuiEditBox>("messageEditBox");
		if (messageEditBox) postMessageLocal(messageEditBox->getText());
		else SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "chatroom GUI doesn't have a editBox named \"messageEditBox\".";
	}
}

/// Initializes the scene (eg adds update functions, loads data, ...). This is called by the engine
void ChatScene::initialize()
{
	if (mode == Mode::MAIN_MENU) {
		initializeMainMenu();
	}
	else if (mode == Mode::HOST || mode == Mode::JOIN) {
		initializeChatroom();
	}
}

/// this function get calls regularly by the Engine class.
void ChatScene::update(const double& dt)
{
	// update the serpManager
	serpManager.update(dt);
	// update the guiManager
	guiManager.update(dt);
	// depending on the current mode, perform different logic
	if (mode == Mode::MAIN_MENU) {
		updateMainMenu();
	}
	else if (mode == Mode::HOST || mode == Mode::JOIN) {
		updateChatroom();
	}
}

ChatScene::~ChatScene()
{
	// TODO
}

int main()
{
	// Initialize the Engine and create the window
	if (!SnackerEngine::Engine::initialize(1200, 700, "Chat demo")) {
		SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "startup failed!" << SnackerEngine::LOGGER::ENDL;
		return 0;
	}

	// Set clear color
	SnackerEngine::Renderer::setClearColor(SnackerEngine::Color3f(0.4f, 0.4f, 0.4f));

	// Register new GUIelements
	SnackerEngine::GuiManager::registerGuiElementType<SnackerEngine::GuiEditVariable<SnackerEngine::SERPID>>("_SERPID");

	// Initialize network code
	SnackerEngine::initializeNetwork();
	
	// Create, initialize and run scene
	{
		ChatScene chatScene;
		chatScene.initialize();
		SnackerEngine::Engine::setActiveScene(chatScene);
		SnackerEngine::Engine::startup();
	}
	
	// Cleanup
	SnackerEngine::Engine::terminate();
}