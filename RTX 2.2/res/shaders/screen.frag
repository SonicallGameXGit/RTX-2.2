#version 120

varying vec2 texcoord;

uniform vec2 screenResolution;
uniform sampler2D colorSampler;

void main() {
    gl_FragColor = texture2D(colorSampler, texcoord);
}