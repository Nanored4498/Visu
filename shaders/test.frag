#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform Camera {
	vec3 center;
	vec3 u, v, w;
} cam;

void main() {
    vec2 uv = inUV - round(inUV);
    vec3 color = vec3(0.65 * pow(max(0., dot(normal, cam.w) / length(normal)), 1.5));
    if(abs(uv.x) < .1 || abs(uv.y) < .1) color *= 0.11;
    outColor = vec4(color, 1.0);
}