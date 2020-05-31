Vertex unpack(uint index)
{
	vec4 data0 = vertices.v[3 * index + 0];
	vec4 data1 = vertices.v[3 * index + 1];
	vec4 data2 = vertices.v[3 * index + 2];

	Vertex v;
	v.pos = data0.xyz;
	v.normal = data1.xyz;
	v.uv = data2.xy;
	return v;
}