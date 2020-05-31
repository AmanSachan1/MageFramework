#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D inputImageSampler;
layout(set = 1, binding = 0, rgba8) uniform readonly image2D computeImage;

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 outColor;

void main() 
{
	ivec2 dim = textureSize(inputImageSampler, 0);
	ivec2 pixelPos = clamp( ivec2(round(float(dim.x) * in_uv.x), round(float(dim.y) * in_uv.y)), 
							ivec2(0.0), 
							ivec2(dim.x - 1, dim.y - 1) );

	vec3 in_color = texture(inputImageSampler, in_uv).rgb;
	vec3 computeImageColor = imageLoad(computeImage, ivec2(pixelPos.xy)).rgb;

	vec3 color = in_color.rgb * vec3(0.7f) + computeImageColor * vec3(0.3f);

	if(pixelPos.x < int(float((dim.x - 1)*0.15f)))
	{
		outColor = vec4(color.r, 0.0, 0.0, 1.0);
	}
	else
	{
		outColor = vec4(color, 1.0);
	}	
}