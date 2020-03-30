# MAGE Framework
Vulkan Framework for testing out and demoing shader techniques I've been intrigued by

# Design Goals:
1) Framework is meant to ease exploration of graphical techniques and ideas.
2) This framework will optimize for the individual techniques rather than the framework itself. The framework is not meant to be slow but resources will be put towards exploring techniques and creative ideas rather than fine grained optimization of the framework.
3) Cross API; Currently it only supports Vulkan but I want to have it support both Vulkan and DX12/DXR


# To Do List (Features/Tasks/Demos)
- Support use of push constants (test with tonemap)
- Texture Arrays
- Scene Rendering (Multuiple meshes, textures, and shaders. Meshes are stored and rendered from a single buffer using vertex offsets. Use push constants, separate per-model and scene descriptor sets are used to pass data to the shaders).
- Sky Compute Shader
- Grass Shaders (Compute, Tesselation and Frag shader)
- Witcher 3 painted world post process shader
- VK_NV_raytracing
- Terrain
- Visibility Buffer
- Clouds
- Sea/Water
- Demo
- DXR/DX12 support


# Design Decisions
## Writing to the Swapchain Image:
I've elected to write to the swapchain only after all of the post process passes have been executed.
These post process passes write to their own frame buffers. And after the last pass is executed I copy the image from the frame buffer into the swapchain via vkCmdCopyImage(...)
I use a copy image command instead of using a subpass because:
1) I want the post process pipeline to be relatively independent of the swapchain, this way if I choose to reorder the order of the passes I dont have to worry about removing and adding the subpass that writes to the swapchain image.
2) Setting #1 above to work automatically is definitely possible but requires more effort than the current approach. 
3) Unsure of the performance benefit (I'm sure there is one I just don't know if it will be significant enough)  to justify the increase in code complexity necessitated by the automated setup mentioned in #2 
4) The framework has a design goal of efficiency in the individual techniques rather than the framework itself. Because trying to optimize a framework/engine is it's own beast.

## Render Order
Compute passes
forward render pass / deferred rendering
Composite compute onto forward render
Post process Passes:
	- High Res passes
	- Tone Map
	- Low Res passes
Copy last post process pass's framebuffer into swapchain
render imgui UI to the swapchain 

# Adding Post Process Passes
1) Create the descriptors (expand pool, create descriptor set and layout, write to and upadte descriptor set)
2) Add the pass in void VulkanRendererBackend::createAllPostProcessEffects(...)

3) (Optional), decide how often it is updated (not a generic process yet)

### Credits/Dependencies:
- [Vulkan](https://www.khronos.org/vulkan/)
- [Imgui](https://github.com/ocornut/imgui)
- [glfw](https://www.glfw.org/)
- [glm](https://github.com/g-truc/glm)
- [stb](https://github.com/nothings/stb)
- [tiny_gltf](https://github.com/syoyo/tinygltf)
- [tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader)

### Tested On:
- Windows 10
- Vulkan 1.2.131.2
- Visual Studio 2019
- GPU: Nvidia GeForce RTX 2080 Ti
- CPU: AMD Ryzen 7 3700X 8-core Processor