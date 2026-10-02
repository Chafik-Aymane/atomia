#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

// We pass the center and radius to handle aliasing
uniform vec2 center;
uniform float radius;
uniform vec2 lightPos;

void main() {
    // Calculate distance from pixel to center
    vec2 p = gl_FragCoord.xy - center;
    float dist = length(p);
    
    // Standard AA using smoothstep
    float alpha = 1.0 - smoothstep(radius - 1.5, radius, dist);
    if (alpha <= 0.0) discard;
    // fake 3d normals
    vec2 n2D = p / radius;
    float nz = sqrt(1.0 - dot(n2D, n2D));
    vec3 normal = vec3(n2D, nz);
    // Lighting (Adjust these to move the "studio lights")
    vec3 lightDir = normalize(vec3(lightPos - center, 600.0)); // Top-right light
    vec3 viewDir  = vec3(0.0, 0.0, 1.0);
    // 4. Diffuse Shading (The subtle gradient)
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 baseColor = fragColor.rgb * (diffuse * 0.5 + 0.5); // Ambient + Diffuse
    // 5. Specular Highlights (The glossy resin shine)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0);

    // Secondary "rim" or "room" highlight
    float spec2 = pow(max(dot(normal, normalize(vec3(-0.3, 0.7, 0.8))), 0.0), 32.0) * 0.3;

    // Combine colors: Base + High-intensity white highlights
    vec3 finalRGB = baseColor + (vec3(1.0) * spec) + (vec3(0.8, 0.9, 1.0) * spec2);

    
    finalColor = vec4(finalRGB, fragColor.a * alpha);
}