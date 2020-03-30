#version 450
#extension GL_ARB_separate_shader_objects : enable

// Binding multiple descriptor sets
// You need to specify a descriptor layout for each descriptor set when creating the pipeline layout. 
// Shaders can then reference specific descriptor sets like this:
// layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
// You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets.
// In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.

layout(set = 0, binding = 0) uniform ModelUBO {
    mat4 modelMat;
};

layout(set = 1, binding = 0) uniform CameraUBO {
	mat4 view;
	mat4 proj;
	vec4 eye;
	vec2 tanFovBy2;
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec4 f_color;
layout(location = 1) out vec2 f_uv;
layout(location = 2) out vec3 f_nor;

void main() 
{
    gl_Position = proj * view * modelMat * vec4(inPos, 1.0);
	f_uv = inUV;
	f_nor = inNor;
	f_color = inColor;
}