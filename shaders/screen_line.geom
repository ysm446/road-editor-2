#version 410 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 v_color[];
out vec3 g_color;

uniform vec2 u_viewportSize;
uniform float u_lineWidth;
uniform float u_depthBias;

void main() {
    vec4 clip0 = gl_in[0].gl_Position;
    vec4 clip1 = gl_in[1].gl_Position;
    if (clip0.w <= 0.0 || clip1.w <= 0.0)
        return;

    vec2 ndc0 = clip0.xy / clip0.w;
    vec2 ndc1 = clip1.xy / clip1.w;
    vec2 delta = ndc1 - ndc0;
    float len = length(delta);
    if (len <= 1e-6)
        return;

    vec2 dir = delta / len;
    vec2 normal = vec2(-dir.y, dir.x);
    vec2 pxToNdc = vec2(2.0 / max(u_viewportSize.x, 1.0),
                        2.0 / max(u_viewportSize.y, 1.0));
    vec2 offsetNdc = normal * (u_lineWidth * 0.5) * pxToNdc;

    clip0.z -= u_depthBias * clip0.w;
    clip1.z -= u_depthBias * clip1.w;

    g_color = v_color[0];
    gl_Position = clip0 + vec4(offsetNdc * clip0.w, 0.0, 0.0);
    EmitVertex();

    gl_Position = clip0 - vec4(offsetNdc * clip0.w, 0.0, 0.0);
    EmitVertex();

    g_color = v_color[1];
    gl_Position = clip1 + vec4(offsetNdc * clip1.w, 0.0, 0.0);
    EmitVertex();

    gl_Position = clip1 - vec4(offsetNdc * clip1.w, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
