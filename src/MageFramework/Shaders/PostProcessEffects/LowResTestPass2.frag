#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D inputImageSampler;

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 outColor;

 const float SCREEN_GAMMA = 2.2; // Assume the monitor is calibrated to the sRGB color space

void main() 
{
	ivec2 dim = textureSize(inputImageSampler, 0);
	ivec2 pixelPos = clamp( ivec2(round(float(dim.x) * in_uv.x), round(float(dim.y) * in_uv.y)), 
							ivec2(0.0), 
							ivec2(dim.x - 1, dim.y - 1) );

	vec3 in_color = texture(inputImageSampler, in_uv).rgb;

	if( (pixelPos.x < int(float((dim.x - 1)*0.85f))) &&
		(pixelPos.x > int(float((dim.x - 1)*0.70f))) )
	{
		outColor = vec4(0.0, 0.0, in_color.b, 1.0);
	}
	else
	{
		outColor = vec4(in_color, 1.0);
	}

	// Gamma Correction
    // vec3 colorGammaCorrected = pow(outColor.rgb, vec3(1.0/SCREEN_GAMMA));
    // outColor = vec4(colorGammaCorrected, 1.0);
}