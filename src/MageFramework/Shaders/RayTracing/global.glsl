struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
};

struct RayPayload 
{
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

// Max recursion depth is passed via a specialization constant
layout (constant_id = 0) const int MAX_RECURSION_DEPTH = 0;
const float T_MIN = 0.001f;
const float T_MAX = 10000.0f;
const uint CULL_MASK = 0xFF;