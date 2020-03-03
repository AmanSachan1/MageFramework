#pragma once
#include <global.h>
#include <forward.h>
#include <Vulkan/vulkanManager.h>

#include "UIManager.h"
#include "camera.h"
#include "renderer.h"

int window_height = 720;
int window_width = 1284;

Renderer* renderer;
Camera* camera;
VulkanManager* vulkanObject;

namespace InputUtil
{
	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		renderer->recreate();
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

		camera->updateUniformBuffer(vulkanObject->getIndex());
		camera->copyToGPUMemory(vulkanObject->getIndex());
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
			deltaForMovement = glm::clamp(deltaForMovement, 0.0f, 1.0f);
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
	static constexpr char* applicationName = "Graphics Playground";
	initWindow(window_width, window_height, applicationName);
	vulkanObject = new VulkanManager(window, applicationName);

	TimerUtil::initTimer();

	camera = new Camera(vulkanObject, glm::vec3(0.0f, 3.0f, -8.0f), glm::vec3(0.0f, 0.0f, 0.0f),
		window_width, window_height, 45.0f, float(window_width) / float(window_height), 0.1f, 1000.0f,
		vulkanObject->getSwapChainImageCount(), CameraMode::ORBIT);

	renderer = new Renderer(window, vulkanObject, camera, window_width, window_height);

	glfwSetWindowSizeCallback(window, InputUtil::resizeCallback);
	glfwSetFramebufferSizeCallback(window, InputUtil::resizeCallback);

	glfwSetKeyCallback(window, InputUtil::key_callback);
	glfwSetScrollCallback(window, InputUtil::scrollCallback);
	glfwSetMouseButtonCallback(window, InputUtil::mouseDownCallback);
	glfwSetCursorPosCallback(window, InputUtil::mouseMoveCallback);
}

void GraphicsPlaygroundApplication::mainLoop()
{
	TIME_POINT frameStartTime = std::chrono::high_resolution_clock::now();
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
	vkDeviceWaitIdle(vulkanObject->getLogicalDevice());

	delete renderer;
	delete camera;
	delete vulkanObject;

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

	std::cout << "Mage Engine Successfull Exit" << std::endl;
	return EXIT_SUCCESS;
}