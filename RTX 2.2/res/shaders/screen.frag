#version 130
#define GAUSSIAN_SAMPLES 12
#define GAUSSIAN_SIGMA float(GAUSSIAN_SAMPLES) * 0.25

varying vec2 texcoord;

uniform vec2 screenResolution;
uniform vec2 textureResolution;

uniform sampler2D colorSampler;

float gaussian(vec2 uv) {
    return exp(-0.5 * dot(uv /= GAUSSIAN_SIGMA, uv)) / (6.28 * GAUSSIAN_SIGMA * GAUSSIAN_SIGMA);
}

vec3 blur(sampler2D sampler, vec2 uv, vec2 resolution) {
    vec4 result;
    
    float scale = 0.02;
    int samples = GAUSSIAN_SAMPLES;
    for(int i = 0; i < samples * samples; i++) {
        vec2 d = vec2(i - floor(i / float(samples)) * float(samples), i / samples) - float(samples) / 2.0;
        result += gaussian(d * scale) * texture2D(sampler, uv + d / 10.0 * scale);
    }
    
    return result.rgb / result.a;
}

void main() {
    gl_FragColor.a = 1.0;

    gl_FragColor.rgb = texture2D(colorSampler, texcoord).rgb;
    //vec3 blurredColor = blur(colorSampler, texcoord, screenResolution);
    //gl_FragColor.rgb = mix(gl_FragColor.rgb, blurredColor, clamp(pow(length(gl_FragColor.rgb - blurredColor), 2.0) * 2.0, 0.0, 1.0));
    //gl_FragColor.rgb *= 1.15;
    //gl_FragColor.rgb = blurredColor;

    vec2 uv = texcoord * 2.0 - 1.0;
    uv.x *= screenResolution.x / screenResolution.y;

    if(uv.x <= 0.001 && uv.x >= -0.001 && uv.y <= 0.01 && uv.y >= -0.01) gl_FragColor.rgb = 1.0 - gl_FragColor.rgb;
    else if(uv.x <= 0.01 && uv.x >= -0.01 && uv.y <= 0.001 && uv.y >= -0.001) gl_FragColor.rgb = 1.0 - gl_FragColor.rgb;
}