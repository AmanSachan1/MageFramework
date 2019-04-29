#version 450
#extension GL_ARB_separate_shader_objects : enable

// Binding multiple descriptor sets
// You need to specify a descriptor layout for each descriptor set when creating the pipeline layout. 
// Shaders can then reference specific descriptor sets like this:
// layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
// You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets.
// In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.

layout(binding = 0) uniform CameraUBO {
	mat4 view;
	mat4 proj;
	vec4 eye;
	vec2 tanFovBy2;
};

layout(binding = 0) uniform ModelUBO {
    mat4 modelMat;
}

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() 
{
    gl_Position = proj * view * modelMat * inPos;
    fragColor = inColor;
}