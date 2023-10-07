#include "Core/Engine.h"
#include "Gui/GuiManager.h"
#include "Utility/Keys.h"
#include "Gui/GuiManager.h"

#include <queue>

class GuiDemoScene : public SnackerEngine::Scene
{
	SnackerEngine::GuiManager guiManager;
	SnackerEngine::GuiEventHandle buttonHandle;
	SnackerEngine::GuiVariableHandle<double> doubleHandle;
	std::queue<std::string> guiSceneQueue;
public:

	/// Helper function that loads the next GuiScene
	void loadNextGuiScene()
	{
		guiManager.clear();
		try
		{
			auto fullPath = SnackerEngine::Engine::getFullPath(guiSceneQueue.front());
			if (fullPath.has_value()) {
				SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN << "Loaded file " << guiSceneQueue.front() << SnackerEngine::LOGGER::ENDL;
				auto json = SnackerEngine::loadJSON(fullPath.value());
				guiManager.loadAndRegisterJSON(json, nullptr);
			}
			else {
				SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "Could not find file " << guiSceneQueue.front() << SnackerEngine::LOGGER::ENDL;
			}
		}
		catch (const std::exception& e)
		{
			SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << e.what() << SnackerEngine::LOGGER::ENDL;
		}
		guiSceneQueue.push(guiSceneQueue.front());
		guiSceneQueue.pop();
	}

	GuiDemoScene()
	{
		guiSceneQueue.push("test/normalDebugWindow.json");
		guiSceneQueue.push("test/gridLayoutTest.json");
		guiSceneQueue.push("test/testListLayouts.json");
		guiSceneQueue.push("test/testMultipleWindows.json");
		guiSceneQueue.push("test/WeightedHorizontalLayout.json");
		loadNextGuiScene();
	}

	void draw() override
	{
		guiManager.draw();
	}

	void callbackKeyboard(const int& key, const int& scancode, const int& action, const int& mods) override
	{
		if (key == SnackerEngine::KEY_R && action == SnackerEngine::ACTION_PRESS)
		{
			loadNextGuiScene();
		}
		guiManager.callbackKeyboard(key, scancode, action, mods);
	}

	virtual void callbackMouseButton(const int& button, const int& action, const int& mods) override
	{
		guiManager.callbackMouseButton(button, action, mods);
	}

	virtual void callbackMouseMotion(const SnackerEngine::Vec2d& position) override
	{
		guiManager.callbackMouseMotion(position);
	}

	void callbackWindowResize(const SnackerEngine::Vec2i& screenDims) override
	{
		guiManager.callbackWindowResize(screenDims);
	}

	void callbackMouseScroll(const SnackerEngine::Vec2d& offset) override
	{
		guiManager.callbackMouseScroll(offset);
	}

	virtual void callbackCharacterInput(const unsigned int& codepoint) override
	{
		guiManager.callbackCharacterInput(codepoint);
	}

	virtual void update(const double& dt) override
	{
		guiManager.update(dt);
		if (buttonHandle.isActive()) {
			SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN <<
				"Button pressed!!" << SnackerEngine::LOGGER::ENDL;
			buttonHandle.reset();
		}
	}
};

int main()
{
	if (!SnackerEngine::Engine::initialize(1200, 700, "Demo: GUI")) {
		SnackerEngine::errorLogger << SnackerEngine::LOGGER::BEGIN << "startup failed!" << SnackerEngine::LOGGER::ENDL;
		return 0;
	}

	{
		GuiDemoScene scene;
		SnackerEngine::Engine::setActiveScene(scene);
		SnackerEngine::Engine::startup();
	}

	SnackerEngine::Engine::terminate();
}