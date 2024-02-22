#version 120

varying vec2 texcoord;

uniform vec2 screenResolution;
uniform sampler2D colorSampler;

uniform int firstFrame;

void main() {
    gl_FragColor = texture2D(colorSampler, texcoord);

    if(firstFrame == 1) {
        int blurSize = 4;
        for(int x = -blurSize / 2; x < blurSize / 2; x++) {
            for(int y = -blurSize / 2; y < blurSize / 2; y++) {
                gl_FragColor.rgb += texture2D(colorSampler, texcoord + vec2(x, y) * 2.0 / screenResolution).rgb;
            }
        }

        gl_FragColor.rgb /= blurSize * blurSize;
    }
}