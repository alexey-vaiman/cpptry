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

// Simple fragment shader for the mirror with checkerboard + reflection
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
    vec2 flippedUV = vec2(1.0 - screenUV.x, screenUV.y);
    vec4 reflectedColor = texture(uReflectionTexture, flippedUV);
    
    // Checkerboard pattern
    vec2 sq = floor(vFragPos.xz * 2.0);
    float checker = mod(sq.x + sq.y, 2.0);
    vec4 darkSquare  = vec4(0.08, 0.08, 0.12, 1.0);
    vec4 lightSquare = vec4(0.25, 0.25, 0.30, 1.0);
    vec4 boardColor = mix(darkSquare, lightSquare, checker);
    
    // 50/50 blend: half reflection, half checkerboard
    fragColor = mix(boardColor, reflectedColor, 0.5);
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
