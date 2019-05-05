#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 2) uniform sampler2D  texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() 
{
	//outColor = texture(texSampler, fragUV);
	outColor = fragColor * texture(texSampler, fragUV);
    //outColor = vec4(fragUV, 0.0, 1.0);
	//outColor = fragColor;
}