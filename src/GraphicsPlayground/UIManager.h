#pragma once
#include <global.h>
#include <stdlib.h>
#include <Vulkan\Utilities\vCommandUtil.h>
#include <Vulkan\Utilities\vRenderUtil.h>
#include <Vulkan\vulkanManager.h>

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

// If these change we need to inform the renderer and recreate everything.
struct RendererOptions
{
	RenderAPI renderAPI;
	bool MSAA;  // Geometry Anti-Aliasing
	bool FXAA;  // Fast-Approximate Anti-Aliasing
	bool TXAA;	// Temporal Anti-Aliasing
	bool enableSampleRateShading; // Shading Anti-Aliasing (enables processing more than one sample per fragment)
	float minSampleShading; // value between 0.0f and 1.0f --> closer to one is smoother
	bool enableAnisotropy; // Anisotropic filtering -- image sampling will use anisotropic filter
	float anisotropy; //controls level of anisotropic filtering
};

// Display options for various UI objects and their data
struct UIOptions
{
	bool framerateInFPS;
	bool transparentWindows;

	// Display Windows
	bool showStatisticsWindow;
	bool showOptionsWindow;

	// Positioning
	float boundaryPadding;
	glm::vec2 statisticsWindowSize;
	glm::vec2 optionsWindowSize;
};

class UIManager
{
public:
	UIManager() = delete;
	UIManager(GLFWwindow* window, VulkanManager* vulkanObj, RendererOptions rendererOptions);
	~UIManager();
	
	void clean();
	void resize();
	void recreate(GLFWwindow* window);
	
	void update(float frameTime);
	void submitDrawCommands();

private:
	VulkanManager* m_vulkanObj;
	VkDevice m_logicalDevice;
	VkQueue m_queue;
	UIOptions m_options;
	RendererOptions m_rendererOptions;
	unsigned int m_windowWidth, m_windowHeight;
	bool m_stateChangeed;

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

#ifdef IMGUI_REFERENCE_DEMO
	bool show_demo_window;
	bool show_another_window;
	ImVec4 clear_color;
#endif

private:
	void setupPlatformAndRendererBindings(GLFWwindow* window);

	void updateState(float frameTime);
	void optionsWindow();
	void statisticsWindow(float frameTime);


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