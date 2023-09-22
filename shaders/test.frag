#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;

void main() {
    vec2 uv = fragColor.xy - round(fragColor.xy);
    if(abs(uv.x) < .1 || abs(uv.y) < .1) uv = vec2(0.1, 0.1);
    else uv = vec2(0.9, 0.9);
    outColor = vec4(uv, fragColor.z, 1.0);
}