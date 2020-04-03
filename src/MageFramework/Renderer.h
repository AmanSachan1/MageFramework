#pragma once

#include <global.h>
#include <forward.h>
#include <Utilities/generalUtility.h>
#include <Vulkan/Utilities/vPipelineUtil.h>
#include <Vulkan/Utilities/vCommandUtil.h>
#include <Vulkan/Utilities/vShaderUtil.h>
#include <Vulkan/Utilities/vRenderUtil.h>
#include <Vulkan/RendererBackend/vRendererBackend.h>
#include <Vulkan/vulkanManager.h>

#include "UIManager.h"

#include "Camera.h"
#include "Scene.h"
#include "SceneElements/model.h"
#include "SceneElements/texture.h"

class Renderer 
{
public:
	Renderer() = delete; // To enforce the creation of a the type of renderer we want without leaving the vulkan device, vulkan swapchain, etc as assumptions or nullptrs
	Renderer(GLFWwindow* window, std::shared_ptr<VulkanManager> vulkanManager, 
		std::shared_ptr<Camera> camera, JSONItem::Scene& scene, uint32_t width, uint32_t height);
	~Renderer();

	void recreate();
	void renderLoop(float frameStartTime);
	
private:
	void initialize(JSONItem::Scene& scene);
	void cleanup();

	// Render Loop Helpers
	void updateRenderState();
	void acquireNextSwapChainImage();
	void presentCurrentImageToSwapChainImage();

	// Commands
	void recordAllCommandBuffers();
		
	// Descriptors
	void setupDescriptorSets();
	void writeToAndUpdateDescriptorSets();

	// Pipelines
	void createAllPipelines();	

private:
	GLFWwindow* m_window;
	RendererOptions m_rendererOptions;
	RendererConstants m_rendererConstants;
	std::shared_ptr<VulkanManager> m_vulkanManager;
	std::shared_ptr<VulkanRendererBackend> m_rendererBackend;
	std::shared_ptr<UIManager> m_UI;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<Scene> m_scene;
};