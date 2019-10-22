#pragma once
#include <global.h>
#include <stdlib.h>
#include <Utilities/commandUtility.h>
#include <Utilities/renderUtility.h>
#include <Vulkan/vulkanManager.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imconfig.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define IMGUI_REFERENCE_DEMO 0

static void check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

class UIManager
{
public:
	UIManager() = delete;
	UIManager(GLFWwindow* window, VulkanManager* vulkanObj);
	~UIManager();
	
	void clean();
	void resize();
	void recreate(GLFWwindow* window);
	
	void update();
	void submitDrawCommands();

private:
	VulkanManager* m_vulkanObj;
	VkDevice m_logicalDevice;
	VkQueue m_queue;

	//-------------------------------------------
	// UI Specific vulkan objects for rendering
	//-------------------------------------------
	// This separation makes it easy to separate memory and resources used by the UI from the regular render operations
	// In addition, it is hard to know how much memory to reserve for UI (especially immediate mode UI) since states can change every frame.
	// New windows or other UI objects can be created each frame, this means the descriptor pool is purposefully reserving extra stuff,
	// It also means command buffers are recreated every frame. To prevent regular render operation command buffers from being recreated every time
	// we separate UI vulkan stuff.
	VkCommandPool m_UICommandPool;
	std::vector<VkCommandBuffer> m_UICommandBuffers;
	VkDescriptorPool m_UIDescriptorPool;

	VkRenderPass m_UIRenderPass;
	std::vector<VkFramebuffer> m_UIFrameBuffers;

	unsigned int windowWidth, windowHeight;

	bool m_showOptionsWindow;
	bool m_showStatisticsWindow;

#ifdef IMGUI_REFERENCE_DEMO
	bool show_demo_window;
	bool show_another_window;
	ImVec4 clear_color;
#endif

private:
	void setupPlatformAndRendererBindings(GLFWwindow* window);

	void updateState();
	void optionsWindow();
	void statisticsWindow();


	// Helpers
	void uploadFonts();

	void setupVulkanObjectsForImgui();
	void createCommandPoolAndCommandBuffers();
	void createDescriptorPool();
	void createRenderPass();
	void createFrameBuffers();


	// Reference Imgui Demo
	void createImguiDefaultDemo();
};