#version 410 core

in  vec3 v_normal;
in  vec2 v_uv;
out vec4 fragColor;

uniform vec3 u_sunDir;  // normalized, world space
uniform vec3 u_color;   // base road surface color
uniform sampler2D u_texture;
uniform int u_useTexture;

void main() {
    vec3  N    = normalize(v_normal);
    float diff = max(dot(N, u_sunDir), 0.0);
    vec3  base = (u_useTexture != 0) ? texture(u_texture, v_uv).rgb : u_color;
    vec3  col  = base * (0.35 + 0.65 * diff);
    fragColor  = vec4(col, 1.0);
}
