#version 120

in vec2 position;
varying vec2 texcoord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    texcoord = position / 2.0 + 0.5;
}