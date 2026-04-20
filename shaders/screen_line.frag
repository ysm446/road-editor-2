#version 410 core

in vec3 g_color;
out vec4 fragColor;

void main() {
    fragColor = vec4(g_color, 1.0);
}
