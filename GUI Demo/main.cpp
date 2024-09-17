#include "Core/Engine.h"
#include "Gui/GuiManager.h"
#include "Utility/Keys.h"
#include "Gui/GuiManager.h"
#include "Gui\GuiElements\GuiButton.h"
#include "Gui\GuiElements\GuiSlider.h"
#include "Gui\Layouts\VerticalScrollingListLayout.h"
#include "Gui\GuiManager.h"

#include "Utility\Handles\VariableHandle.h"

#include <queue>

class GuiDemoScene : public SnackerEngine::Scene
{
	SnackerEngine::GuiManager guiManager;
	SnackerEngine::EventHandle buttonHandle;
	SnackerEngine::VariableHandle<double> doubleHandle;
	std::queue<std::string> guiSceneQueue;
	std::string currentScene;
	SnackerEngine::VariableHandle<float> variableHandleFloat{ 0.0f };
	bool toggle = false;
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
		currentScene = guiSceneQueue.front();
		guiSceneQueue.push(guiSceneQueue.front());
		guiSceneQueue.pop();
		// Setup special functionality for some scenes
		if (currentScene == "test/animationTest.json") {
			const auto buttonAnimate = guiManager.getGuiElement<SnackerEngine::GuiButton>("buttonAnimate");
			if (buttonAnimate) buttonAnimate->subscribeToEventButtonPress(buttonHandle);
			const auto slider = guiManager.getGuiElement<SnackerEngine::GuiSliderFloat>("slider");
			if (slider) slider->getVariableHandle().connect(variableHandleFloat);
		}
	}

	GuiDemoScene()
	{
		guiSceneQueue.push("test/selectionBoxTest.json");
		guiSceneQueue.push("test/animationTest.json");
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
			buttonHandle.reset();
			if (currentScene == "test/animationTest.json") {
				// Animate panel pos
				auto guiPanel = guiManager.getGuiElement<SnackerEngine::GuiPanel>("panel");
				if (!toggle) {
					if (guiPanel) guiManager.signUpAnimatable(guiPanel->animatePositionX(guiPanel->getPositionX(), 500, 1.0));
				}
				else {
					if (guiPanel) guiManager.signUpAnimatable(guiPanel->animatePositionX(guiPanel->getPositionX(), 100, 1.0, SnackerEngine::AnimationFunction::easeInOutExponential));
				}
				// Animate slider value
				auto guiSlider = guiManager.getGuiElement<SnackerEngine::GuiSliderFloat>("slider");
				if (!toggle) {
					if (guiSlider) guiManager.signUpAnimatable(guiSlider->animateValue(guiSlider->getValue(), 0.7f, 0.5, SnackerEngine::AnimationFunction::easeOutElastic));
				}
				else {
					if (guiSlider) guiManager.signUpAnimatable(guiSlider->animateValue(guiSlider->getValue(), 0.0f, 1.0, SnackerEngine::AnimationFunction::easeOutBounce));
				}
				toggle = !toggle;
			}
			else {
				SnackerEngine::infoLogger << SnackerEngine::LOGGER::BEGIN <<
					"Button pressed!!" << SnackerEngine::LOGGER::ENDL;
			}
		}
		if (variableHandleFloat.isActive()) {
			variableHandleFloat.reset();
			if (currentScene == "test/animationTest.json") {	
				// Set offset percentage of scrolling list layout
				auto scrollingList = guiManager.getGuiElement<SnackerEngine::GuiVerticalScrollingListLayout>("scrollingList");
				if (scrollingList) scrollingList->setTotalOffsetPercentage(variableHandleFloat);
			}
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