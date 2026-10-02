#version 330

precision mediump float;
uniform float time;
uniform vec2 cameraOffset;
uniform vec2 cameraTarget;
uniform float cameraZoom;
uniform vec2 screenRes;

out vec4 finalColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 2D Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// Fractal Brownian Motion: Layers noise for organic detail
float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotation matrix to reduce axial bias in patterns
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for (int i = 0; i < 5; ++i) {
        v += a * noise(p);
        p = rot * p * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 screenOffset = gl_FragCoord.xy - cameraOffset;
    vec2 worldPos = vec2(screenOffset.x, -screenOffset.y) / cameraZoom + cameraTarget;
    


    // this is what draws the effects
    vec2 p = worldPos / 1500.0;
    // --- DOMAIN WARPING (The "Oily" Secret) ---
    // We calculate noise, then use it to offset the next layer of noise calculation.
    vec2 q = vec2(0.0);
    q.x = fbm(p + 0.1 * time);
    q.y = fbm(p + vec2(1.0));
    vec2 r = vec2(0.0);
    r.x = fbm(p + 1.0 * q + vec2(1.7, 9.2) + 0.15 * time);
    r.y = fbm(p + 1.0 * q + vec2(8.3, 2.8) + 0.126 * time);
    // Final pattern density
    float f = fbm(p + r);
    vec3 colorDark  = vec3(0.18, 0.38, 0.32); // Deep Forest/Teal
    vec3 colorLight = vec3(0.42, 0.74, 0.58); // Bright Mint/Seafoam
    vec3 colorDeep  = vec3(0.10, 0.25, 0.22); // Deep Shadow green
    // Mix based on the warped noise layers
    vec3 finalRGB = mix(colorDark, colorLight, f);
    finalRGB = mix(finalRGB, colorDeep, pow(length(q), 2.0) * 0.5);

    // FIXED VIGNETTE: Values between 0.7 and 1.6 make it subtle
    vec2 worldCenter = vec2(0.0, 0.0);
    // Calculate distance from the world center, not the screen center
    float worldDist = length(worldPos - worldCenter);

    // 1000.0 is the radius where it starts getting dark.
    // 3000.0 is where it becomes total darkness.
    float vignette = 1.0 - smoothstep(7000.0, 15000.0, worldDist);
    // Apply the vignette to the RGB
    finalColor = vec4(finalRGB * vignette, 1.0);
}