#pragma once
#include "UIManager.h"

// Disable Warning C26451: Arithmetic overflow; Because of imgui_draw.cpp
#pragma warning( disable : 26451 )

#include <imgui.cpp>
#include <imgui_demo.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_impl_glfw.cpp>
#include <imgui_impl_vulkan.cpp>

UIManager::UIManager(GLFWwindow* window, VulkanManager* vulkanObj, RendererOptions rendererOptions)
	: m_vulkanObj(vulkanObj), 
	m_logicalDevice(vulkanObj->getLogicalDevice()), 
	m_queue(m_vulkanObj->getQueue(QueueFlags::Graphics)),
	m_rendererOptions(rendererOptions)
{
	setupVulkanObjectsForImgui();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark(); // Setup Dear ImGui style

	ImGui_ImplGlfw_InitForVulkan(window, true);
	setupPlatformAndRendererBindings(window);
	uploadFonts();

	const VkExtent2D extents = m_vulkanObj->getSwapChainVkExtent();
	m_windowWidth = extents.width;
	m_windowHeight = extents.height;
	m_stateChanged = true;

	// Set options
	{
		m_options.framerateInFPS = false;
		m_options.transparentWindows = true;
		m_options.showStatisticsWindow = true;
		// Re-enable when the option to actually toggle this stuff exists in the renderer
		m_options.showOptionsWindow = false;

		m_options.boundaryPadding = 5;
	}

#if IMGUI_REFERENCE_DEMO
	show_demo_window = true;
	show_another_window = false;
	clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#endif
}
UIManager::~UIManager()
{
	clean();
	vkDestroyDescriptorPool(m_vulkanObj->getLogicalDevice(), m_UIDescriptorPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_UICommandPool, nullptr);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
void UIManager::clean()
{
	// Resources to destroy on swapchain recreation
	for (auto framebuffer : m_UIFrameBuffers) 
	{
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
	}
	vkDestroyRenderPass(m_logicalDevice, m_UIRenderPass, nullptr);
	
	vkFreeCommandBuffers(m_logicalDevice, m_UICommandPool, static_cast<uint32_t>(m_UICommandBuffers.size()), m_UICommandBuffers.data());
}
void UIManager::resize(GLFWwindow* window)
{
	const VkExtent2D extents = m_vulkanObj->getSwapChainVkExtent();
	m_windowWidth = extents.width;
	m_windowHeight = extents.height;
	m_stateChanged = true;

	// Destroys CommandBuffers, Framebuffers, and Renderpasses
	clean();

	// Recreate CommandBuffers
	const int numSWImages = m_vulkanObj->getSwapChainImageCount();
	m_UICommandBuffers.resize(numSWImages);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_UICommandPool, m_UICommandBuffers);

	// Recreate FrameBuffers and Renderpasses again
	createRenderPass();
	createFrameBuffers();
}


void UIManager::update(float frameTime)
{
	// New Frame
	ImGui_ImplVulkan_NewFrame(); // empty
	ImGui_ImplGlfw_NewFrame(); // handles inputs, screen resiziing, etc
	ImGui::NewFrame();

	// Update UI
	updateState(frameTime);

	// Record new state into command buffers
	ImGui::Render();
}
void UIManager::submitDrawCommands()
{
	unsigned int frameIndex = m_vulkanObj->getIndex();
	VkRect2D renderArea = {};
	renderArea.extent = m_vulkanObj->getSwapChainVkExtent();
	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	vkResetCommandBuffer(m_UICommandBuffers[frameIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	VulkanCommandUtil::beginCommandBuffer(m_UICommandBuffers[frameIndex]);
	VulkanCommandUtil::beginRenderPass(m_UICommandBuffers[frameIndex], m_UIRenderPass, m_UIFrameBuffers[frameIndex], renderArea, 1, &clearColor);

	// Record Imgui Draw Data and draw funcs into command buffer
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_UICommandBuffers[frameIndex]);

	vkCmdEndRenderPass(m_UICommandBuffers[frameIndex]);
	VulkanCommandUtil::endCommandBuffer(m_UICommandBuffers[frameIndex]);

	VulkanCommandUtil::submitToQueue(m_queue, 1, m_UICommandBuffers[frameIndex]);
}


// Update Imgui State
// Any and all UI options that one would need to create are done through this function
void UIManager::updateState(float frameTime)
{
#if IMGUI_REFERENCE_DEMO
	createImguiDefaultDemo();
#endif
	
	// The ordering here is important, window positioning depends on previous window position and size
	if(m_options.showStatisticsWindow) statisticsWindow(frameTime);
	if(m_options.showOptionsWindow) optionsWindow();

	m_stateChanged = false;
}
void UIManager::statisticsWindow(float frameTime)
{
	if (m_stateChanged)
	{
		m_options.statisticsWindowSize.x = 200; // width
		m_options.statisticsWindowSize.y = 50; // height
		const float& width = m_options.statisticsWindowSize.x;
		const float& height = m_options.statisticsWindowSize.y;
		const float xPos = m_options.boundaryPadding;
		const float yPos = m_options.boundaryPadding;

		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiCond_Always); // handles resizing if ImGuiCond_Always is set
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Appearing);
	}

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (m_options.transparentWindows) window_flags |= ImGuiWindowFlags_NoBackground;

	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Statistics", &m_options.showStatisticsWindow, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	// This is the average framerate of the Application
	if (m_options.framerateInFPS)
	{
		ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);
	}
	else
	{
		// Use commented out portion when using your own timer system
		//ImGui::Text("Framerate: %.3f ms/frame", frameTime);
		ImGui::Text("Framerate: %.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	}
	
	ImGui::End();
}
void UIManager::optionsWindow()
{
	if (m_stateChanged)
	{
		m_options.optionsWindowSize.x = 300; // width
		m_options.optionsWindowSize.y = 150; // height
		const float& width = m_options.optionsWindowSize.x;
		const float& height = m_options.optionsWindowSize.y;
		const float xPos = m_windowWidth - m_options.optionsWindowSize.x - m_options.boundaryPadding;
		const float yPos = m_options.boundaryPadding;

		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiCond_Always); // handles resizing if ImGuiCond_Always is set
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Appearing);

		// Make the window start off in a collapsed state
		ImGui::SetWindowCollapsed(true, ImGuiCond_Once);
	}

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (m_options.transparentWindows) window_flags |= ImGuiWindowFlags_NoBackground;

	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Renderer Options", &m_options.showOptionsWindow, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	int rendererAnisotrpy = static_cast<int>(m_rendererOptions.anisotropy);
	ImGui::Checkbox("MSAA", &m_rendererOptions.MSAA);
	ImGui::Checkbox("FXAA", &m_rendererOptions.FXAA);
	ImGui::Checkbox("TXAA", &m_rendererOptions.TXAA);
	ImGui::PushItemWidth(-150); // slider will fill the space and leave 100 pixels for the label
	ImGui::SliderInt("Anisotropy", &rendererAnisotrpy, 0, 16);
	ImGui::SliderFloat("Variable Rate Shading", &m_rendererOptions.minSampleShading, 0.0f, 1.0f);

	m_rendererOptions.anisotropy = static_cast<float>(rendererAnisotrpy);

	ImGui::End();
}


// Helpers
void UIManager::setupPlatformAndRendererBindings(GLFWwindow* window)
{
	// Setup Platform/Renderer bindings
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_vulkanObj->getVkInstance();
	init_info.PhysicalDevice = m_vulkanObj->getPhysicalDevice();
	init_info.Device = m_vulkanObj->getLogicalDevice();
	init_info.QueueFamily = m_vulkanObj->getQueueIndex(QueueFlags::Graphics);;
	init_info.Queue = m_queue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = m_UIDescriptorPool; // Separate Descriptor Pool for UI
	init_info.Allocator = nullptr;
	init_info.MinImageCount = m_vulkanObj->getSwapChainImageCount();
	init_info.ImageCount = m_vulkanObj->getSwapChainImageCount();
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, m_UIRenderPass); // Separate Render Pass for UI
}
void UIManager::uploadFonts()
{
	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	VkCommandBuffer cmdBuffer;
	VulkanCommandUtil::beginSingleTimeCommand(m_logicalDevice, m_UICommandPool, cmdBuffer);
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	VulkanCommandUtil::endAndSubmitSingleTimeCommand(m_logicalDevice, m_queue, m_UICommandPool, cmdBuffer);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

// Vulkan Setup
void UIManager::setupVulkanObjectsForImgui()
{
	createCommandPoolAndCommandBuffers();
	createDescriptorPool();
	createRenderPass();
	createFrameBuffers();
}
void UIManager::createCommandPoolAndCommandBuffers()
{
	// Do not need multiple command pools, just multiple command buffers
	const int numSWImages = m_vulkanObj->getSwapChainImageCount();
	m_UICommandBuffers.resize(numSWImages);

	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be
	// individually reset to the initial state; either by calling vkResetCommandBuffer, or via the 
	// implicit reset when calling vkBeginCommandBuffer.
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_UICommandPool, 
		m_vulkanObj->getQueueIndex(QueueFlags::Graphics), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_UICommandPool, m_UICommandBuffers);
}
void UIManager::createDescriptorPool()
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	VkResult err = vkCreateDescriptorPool(m_vulkanObj->getLogicalDevice(), &pool_info, nullptr, &m_UIDescriptorPool);
	check_vk_result(err);
}
void UIManager::createRenderPass()
{
	// Create Color Attachment 
	VkAttachmentDescription	colorAttachment =
		RenderPassUtil::attachmentDescription(m_vulkanObj->getSwapChainImageFormat(), VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,			  //color data
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,    //stencil data
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);  //initial and final layout

	// Create Color Attachment References
	VkAttachmentReference colorAttachmentReference = RenderPassUtil::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	// Subpass -- UI Render Pass
	VkSubpassDescription subpassDescription =
		RenderPassUtil::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr, //input attachments
			1, &colorAttachmentReference, //color attachments
			nullptr, //resolve attachments -- only comes into play when you have multiple samples (think MSAA)
			nullptr, //depth and stencil
			0, nullptr); //preserve attachments

	// Subpass Description -- details how to transition into the renderpass/subpass
	VkSubpassDependency subpassDependency =
		RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 /* Or use VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT */,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	VkDevice logicalDevice = m_vulkanObj->getLogicalDevice();
	RenderPassUtil::createRenderPass(logicalDevice,
		m_UIRenderPass,
		1, &colorAttachment,
		1, &subpassDescription,
		1, &subpassDependency);

	// NOTE: Since UI render pass is now the presenting render pass, we need to change our other renderpasses to reflect the same.
	// Basically changing finalLayouts from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
}
void UIManager::createFrameBuffers()
{
	uint32_t numSwapChainImages = m_vulkanObj->getSwapChainImageCount();
	VkImageView frameBufferColorAttachment[1];
	VkFormat swapChainImageFormat = m_vulkanObj->getSwapChainImageFormat();
	VkExtent2D extents = m_vulkanObj->getSwapChainVkExtent();

	m_UIFrameBuffers.resize(numSwapChainImages);
	for (uint32_t i = 0; i < numSwapChainImages; i++)
	{
		frameBufferColorAttachment[0] = m_vulkanObj->getSwapChainImageView(i);
		FrameResourcesUtil::createFrameBuffer(m_logicalDevice, m_UIFrameBuffers[i],	m_UIRenderPass, extents, 1, frameBufferColorAttachment);
	}
}


// Reference Imgui Demo
void UIManager::createImguiDefaultDemo()
{
	//------------------------------------------------------------
	// To test imgui copy paste the 3 lines below to the end of imguiSetup()
	//show_demo_window = true;
	//show_another_window = false;
	//clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);
	
	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}