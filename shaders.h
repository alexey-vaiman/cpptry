#ifndef SHADERS_H
#define SHADERS_H

// Vertex shader for the cube and standard objects
const char* cubeVertexShader = R"glsl(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;

void main() {
    vFragPos = vec3(uModel * vec4(aPosition, 1.0));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(vFragPos, 1.0);
}
)glsl";

// Fragment shader for the cube
const char* cubeFragmentShader = R"glsl(#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D uSampler;
uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

out vec4 fragColor;

void main() {
    vec3 ambient = uAmbientStrength * uLightColor;
    
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // Specular highlight
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * uLightColor;
    
    vec4 texColor = texture(uSampler, vTexCoord);
    fragColor = vec4((ambient + diffuse + specular) * texColor.rgb, 1.0);
}
)glsl";

// Fragment shader for the mirror (using projective mapping)
const char* mirrorFragmentShader = R"glsl(#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D uReflectionTexture;
uniform vec3 uViewPos;

out vec4 fragColor;

void main() {
    // In our simplified setup, the mirror captures everything reflected on it
    // For a simple plane reflection, we use projective coordinate based on gl_FragCoord or computed screen space
    // Since we're rendering to screen, we can use screen-space UVs
    vec2 screenUV = gl_FragCoord.xy / vec2(viewportWidth, viewportHeight); // viewportWidth/Height must be uniforms
    
    // We'll pass the resolution as uniforms for simplicity in main.cpp
    // vec4 reflectedColor = texture(uReflectionTexture, screenUV);
    // fragColor = reflectedColor * 0.8 + vec4(0.1, 0.1, 0.15, 1.0) * 0.2; // Tint slightly
    
    // This shader is just a placeholder, in main.cpp we'll provide real projective logic or screen-space mapping
    fragColor = vec4(0.5, 0.5, 0.5, 1.0); // Fallback color
}
)glsl";

// Simple fragment shader for the mirror with real screen-space mapping
const char* mirrorFragmentShaderReal = R"glsl(#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D uReflectionTexture;
uniform vec2 uResolution;

out vec4 fragColor;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    // Invert X because the reflection pass effectively mirrors the scene
    vec2 flippedUV = vec2(1.0 - screenUV.x, screenUV.y);
    vec4 reflectedColor = texture(uReflectionTexture, flippedUV);
    
    // Add a procedural grid for perspective
    vec2 gridUV = vFragPos.xz * 2.0; // scale of grid
    vec2 grid = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
    float line = min(grid.x, grid.y);
    float gridPattern = 1.0 - min(line, 1.0);
    
    // Mix reflection with grid and floor color
    vec4 floorColor = vec4(0.05, 0.05, 0.1, 1.0);
    vec4 finalColor = mix(reflectedColor, floorColor * (1.0 + gridPattern * 0.2), 0.15);
    
    fragColor = finalColor;
}
)glsl";

// Simple fragment shader for rendering solid colors (with uColor uniform)
const char* solidFragmentShader = R"glsl(#version 300 es
precision highp float;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(uColor, 1.0);
}
)glsl";

#endif
