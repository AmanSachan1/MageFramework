#pragma once
#include <global.h>
#include <Vulkan/vulkanManager.h>

#include "UIManager.h"
#include "camera.h"
#include "renderer.h"

std::shared_ptr<VulkanManager> vulkanManager;
std::shared_ptr<Renderer> renderer;
std::shared_ptr<Camera> camera;

namespace InputUtil
{
	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		renderer->m_windowResized = true;
		//renderer->recreate();
	}

	static bool leftMouseDown = false;
	static bool changeCameraMode = false;
	static double previousX = 0.0f;
	static double previousY = 0.0f;
	static float deltaForRotation = 0.4f;
	static float deltaForMovement = 0.015f;

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}

		if (key == GLFW_KEY_M && action == GLFW_PRESS)
		{
			changeCameraMode = true;
			camera->switchCameraMode();
		}
	}

	void keyboardInputs(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			camera->translateAlongLook(deltaForMovement);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			camera->translateAlongLook(-deltaForMovement);
		}

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			camera->translateAlongRight(-deltaForMovement);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			camera->translateAlongRight(deltaForMovement);
		}

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			camera->translateAlongUp(deltaForMovement);
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			camera->translateAlongUp(-deltaForMovement);
		}

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			camera->rotateAboutRight(deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			camera->rotateAboutRight(-deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			camera->rotateAboutUp(deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			camera->rotateAboutUp(-deltaForRotation);
		}

		camera->updateUniformBuffer(vulkanManager->getFrameIndex());
		camera->copyToGPUMemory(vulkanManager->getFrameIndex());
	}

	void mouseDownCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				leftMouseDown = true;
				glfwGetCursorPos(window, &previousX, &previousY);
			}
			else if (action == GLFW_RELEASE) {
				leftMouseDown = false;
			}
		}
	}

	void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition)
	{
		if (leftMouseDown)
		{
			double sensitivity = 0.1;
			float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
			float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);
			previousX = xPosition;
			previousY = yPosition;

			camera->rotateAboutUp(deltaX);
			camera->rotateAboutRight(deltaY);
		}
	}

	void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
			(glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS))
		{
			deltaForMovement += static_cast<float>(yoffset) * 0.001f;
			deltaForMovement = glm::clamp(deltaForMovement, 0.0f, 10.0f);
		}
		else
		{
			camera->translateAlongLook(static_cast<float>(yoffset) * 0.05f);
		}
	}
}

class GraphicsPlaygroundApplication
{
public:
	void run();

private:
	GLFWwindow* window;

	void initialize();
	void initWindow(int width, int height, const char* name);

	void mainLoop();
	void cleanup();
};

void GraphicsPlaygroundApplication::initWindow(int width, int height, const char* name)
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported())
	{
		fprintf(stderr, "Vulkan not supported\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);

	if (!window)
	{
		fprintf(stderr, "Failed to initialize GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void GraphicsPlaygroundApplication::initialize()
{
	static constexpr char* applicationName = "Mage Framework";
	// Loads in the main camera and the scene
	// JSONContents jsonContent = loadingUtil::loadJSON("gltfTest_box.json");
	// JSONContents jsonContent = loadingUtil::loadJSON("emptyScene.json");
	// JSONContents jsonContent = loadingUtil::loadJSON("objTestChalet.json");
	JSONContents jsonContent = loadingUtil::loadJSON("gltfTest_gltf_and_obj.json");
	const int window_width = jsonContent.mainCamera.width;
	const int window_height = jsonContent.mainCamera.height;

	RendererOptions rendererOptions =
	{
		RENDER_API::VULKAN, // API
		RENDER_TYPE::RAYTRACE, // RAYTRACE   RASTERIZATION
		false, false, false, // Anti-Aliasing 
		false, 1.0f, // Sample Rate Shading
		true, 16.0f // Anisotropy
	};

	initWindow(window_width, window_height, applicationName);
	vulkanManager = std::make_shared<VulkanManager>(window, applicationName);

	TimerUtil::initTimer();
	camera = std::make_shared<Camera>(vulkanManager, jsonContent.mainCamera, vulkanManager->getSwapChainImageCount(), CameraMode::FLY, rendererOptions.renderType);
	renderer = std::make_shared<Renderer>(window, vulkanManager, camera, jsonContent.scene, rendererOptions, window_width, window_height);

	glfwSetWindowSizeCallback(window, InputUtil::resizeCallback);
	glfwSetFramebufferSizeCallback(window, InputUtil::resizeCallback);

	glfwSetKeyCallback(window, InputUtil::key_callback);
	glfwSetScrollCallback(window, InputUtil::scrollCallback);
	glfwSetMouseButtonCallback(window, InputUtil::mouseDownCallback);
	glfwSetCursorPosCallback(window, InputUtil::mouseMoveCallback);
}

void GraphicsPlaygroundApplication::mainLoop()
{
	TIME_POINT frameStartTime;
	float prevFrameTime = 0.0f;

	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	while (!glfwWindowShouldClose(window))
	{
		frameStartTime = std::chrono::high_resolution_clock::now();
		//Mouse inputs, window resize callbacks, and certain key press events
		glfwPollEvents();
		InputUtil::keyboardInputs(window);

		renderer->renderLoop(prevFrameTime);

		prevFrameTime = TimerUtil::getTimeElapsedSinceStart(frameStartTime);
	}
}

void GraphicsPlaygroundApplication::cleanup()
{
	// Wait for the device to finish executing before cleanup
	vkDeviceWaitIdle(vulkanManager->getLogicalDevice());
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GraphicsPlaygroundApplication::run()
{
	initialize();
	mainLoop();
	cleanup();
}

int main()
{
	GraphicsPlaygroundApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "\nSuccessfully Exiting Mage Framework" << std::endl;
	return EXIT_SUCCESS;
}