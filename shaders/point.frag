#version 410 core

in  vec3 v_color;
out vec4 fragColor;

void main() {
    // Discard fragments outside the inscribed circle → round point
    vec2 c = gl_PointCoord - vec2(0.5);
    if (dot(c, c) > 0.25) discard;
    fragColor = vec4(v_color, 1.0);
}
