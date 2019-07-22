#pragma once

#include <global.h>
#include <forward.h>

#include <Utilities\pipelineUtility.h>
#include <Utilities\commandUtility.h>
#include <Utilities\shaderUtility.h>
#include <Utilities\renderUtility.h>
#include <Utilities\generalUtility.h>

#include "VulkanSetup\vulkanDevices.h"
#include "VulkanSetup\vulkanPresentation.h"

#include "Camera.h"
#include "Scene.h"
#include "SceneElements\model.h"
#include "SceneElements\texture.h"
#include "renderPassManager.h"
#include "pipelineManager.h"

static constexpr unsigned int WORKGROUP_SIZE = 32;

enum class RenderAPI { VULKAN, DX12 };

struct RendererOptions 
{
	RenderAPI renderAPI;
	bool TXAA; // Geometry Anti-Aliasing
	bool enableSampleRateShading; // Shading Anti-Aliasing (enables processing more than one sample per fragment)
	float minSampleShading; // value between 0.0f and 1.0f --> closer to one is smoother
	bool enableAnisotropy; // Anisotropic filtering -- image sampling will use anisotropic filter
	float anisotropy; //controls level of anisotropic filtering
};

class Renderer 
{
public:
	Renderer() = delete; // To enforce the creation of a the type of renderer we want without leaving the vulkan device, vulkan swapchain, etc as assumptions or nullptrs
	Renderer(GLFWwindow* window, RendererOptions rendererOptions, VulkanDevices* devices, VulkanPresentation* presentation, Camera* camera, uint32_t width, uint32_t height);
	~Renderer();

	void initialize();
	void recreate();
	void cleanup();

	void renderLoop();
	
	// Commands
	void recordAllCommandBuffers();
	void recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex);
	void recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex);
	void createCommandPoolsAndBuffers();
	
	// Descriptors
	void setupDescriptorSets();

	// Pipelines
	void createAllPipelines();

	// Helpers
	void setResizeFlag(bool value) { m_resizeFrameBuffer = value; }

private:
	GLFWwindow* m_window;
	VulkanDevices* m_devices; // manages both the logical device (VkDevice) and the physical Device (VkPhysicalDevice)
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VulkanPresentation* m_presentationObject;
	VkQueue	m_graphicsQueue;
	VkQueue	m_computeQueue;
	
	Camera* m_camera;
	Scene* m_scene;
	RenderPassManager* m_renderPassManager;
	PipelineManager* m_pipelineManager;
	
	bool m_swapPingPongBuffers = false;
	bool m_resizeFrameBuffer = false;

	// --- Command Buffers and Memory Pools
	// ----- Compute -----
	std::vector<VkCommandBuffer> m_computeCommandBuffers;
	VkCommandPool m_computeCommandPool;
	
	// ----- Standard Graphics -----
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	VkCommandPool m_graphicsCommandPool;

	// Descriptor Pools
	VkDescriptorPool descriptorPool;

	////Tone Map
	//VkDescriptorSetLayout m_DSL_toneMap;
	//std::vector<VkDescriptorSet> m_DS_toneMap;

	//// Descriptor Sets for pingPonged TXAA
	//VkDescriptorSetLayout m_DSL_TXAA;
	//std::vector<VkDescriptorSet> m_DS_TXAA;
};