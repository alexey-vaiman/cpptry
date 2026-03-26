#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "shaders.h"

// Math utilities
struct Vec3 { float x, y, z; };
struct Mat4 { float m[16]; };

Mat4 perspective(float fov, float aspect, float near, float far) {
    float f = 1.0f / tan(fov * 0.5f);
    Mat4 res = {0};
    res.m[0] = f / aspect;
    res.m[5] = f;
    res.m[10] = (far + near) / (near - far);
    res.m[11] = -1.0f;
    res.m[14] = (2.0f * far * near) / (near - far);
    return res;
}

Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
    float f_len = sqrt(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= f_len; f.y /= f_len; f.z /= f_len;
    
    Vec3 s = { f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z, f.x * up.y - f.y * up.x };
    float s_len = sqrt(s.x*s.x + s.y*s.y + s.z*s.z);
    s.x /= s_len; s.y /= s_len; s.z /= s_len;
    
    Vec3 u = { s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x };
    
    Mat4 res = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    res.m[0] = s.x; res.m[4] = s.y; res.m[8] = s.z;
    res.m[1] = u.x; res.m[5] = u.y; res.m[9] = u.z;
    res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z;
    res.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    res.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    res.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    return res;
}

Mat4 identity() {
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
}

Mat4 translate(Mat4 m, Vec3 v) {
    m.m[12] += m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z;
    m.m[13] += m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z;
    m.m[14] += m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z;
    m.m[15] += m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z;
    return m;
}

Mat4 rotateY(Mat4 m, float angle) {
    float c = cos(angle), s = sin(angle);
    float m0 = m.m[0], m1 = m.m[1], m2 = m.m[2], m3 = m.m[3];
    float m8 = m.m[8], m9 = m.m[9], m10 = m.m[10], m11 = m.m[11];
    m.m[0] = m0 * c - m8 * s; m.m[1] = m1 * c - m9 * s; m.m[2] = m2 * c - m10 * s; m.m[3] = m3 * c - m11 * s;
    m.m[8] = m0 * s + m8 * c; m.m[9] = m1 * s + m9 * c; m.m[10] = m2 * s + m10 * c; m.m[11] = m3 * s + m11 * c;
    return m;
}
Mat4 scale(Mat4 m, Vec3 v) {
    m.m[0] *= v.x; m.m[1] *= v.x; m.m[2] *= v.x; m.m[3] *= v.x;
    m.m[4] *= v.y; m.m[5] *= v.y; m.m[6] *= v.y; m.m[7] *= v.y;
    m.m[8] *= v.z; m.m[9] *= v.z; m.m[10] *= v.z; m.m[11] *= v.z;
    return m;
}

// Global state
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
GLuint cubeVAO, cubeVBO, mirrorVAO, mirrorVBO;
GLuint cubeProgram, mirrorProgram, solidProgram;
GLuint cubeTexture, reflectionTexture, reflectionFBO, reflectionDepthRB;

int viewportWidth = 800, viewportHeight = 600;

// Camera state (Spherical coordinates)
float camTheta = 0.5f; 
float camPhi = 1.0f;   // 0 = top down, PI/2 = side view
float camRadius = 8.0f;
bool isMouseDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

// Mouse callbacks
EM_BOOL on_mousedown(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    isMouseDown = true;
    lastMouseX = e->targetX;
    lastMouseY = e->targetY;
    return EM_TRUE;
}

EM_BOOL on_mouseup(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    isMouseDown = false;
    return EM_TRUE;
}

EM_BOOL on_mousemove(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    if (isMouseDown) {
        float dx = (float)(e->targetX - lastMouseX);
        float dy = (float)(e->targetY - lastMouseY);
        
        // Reduced sensitivity and adjusted mapping
        camTheta -= dx * 0.005f; // Inverted for more natural drag
        camPhi -= dy * 0.005f;   // Drag up to look up
        
        // Clamp camPhi to avoid flipping at poles (0 to PI)
        if (camPhi > 2.2f) camPhi = 2.2f; // Slightly below horizon but above ground
        if (camPhi < 0.1f) camPhi = 0.1f;
        
        lastMouseX = e->targetX;
        lastMouseY = e->targetY;
    }
    return EM_TRUE;
}

EM_BOOL on_wheel(int eventType, const EmscriptenWheelEvent *e, void *userData) {
    camRadius += (float)e->deltaY * 0.01f;
    if (camRadius < 2.0f) camRadius = 2.0f;
    if (camRadius > 50.0f) camRadius = 50.0f;
    return EM_TRUE;
}

// Texture creation with numbers (1-6)
GLuint createNumberedTexture() {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    int size = 128; // Pack 6 numbers into a 3x2 grid
    std::vector<unsigned char> data(size * 3 * size * 2 * 4, 0);
    
    auto drawDot = [&](int n, int x, int y) {
        int ox = (n % 3) * size + x;
        int oy = (n / 3) * size + y;
        int r = size / 10;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (dx*dx + dy*dy <= r*r) {
                    int px = ox + dx;
                    int py = oy + dy;
                    if (px >= 0 && px < size*3 && py >= 0 && py < size*2) {
                        int idx = (py * size * 3 + px) * 4;
                        data[idx] = data[idx+1] = data[idx+2] = 255; // White dot
                    }
                }
            }
        }
    };

    for (int n = 0; n < 6; ++n) {
        int ox = (n % 3) * size;
        int oy = (n / 3) * size;
        // Background colors for variety
        unsigned char r = (n == 0 || n == 3 || n == 4) ? 200 : 20;
        unsigned char g = (n == 1 || n == 4 || n == 5) ? 200 : 20;
        unsigned char b = (n == 2 || n == 3 || n == 5) ? 200 : 20;
        
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int idx = ((oy + y) * size * 3 + (ox + x)) * 4;
                data[idx] = r; data[idx+1] = g; data[idx+2] = b; data[idx+3] = 255;
            }
        }
        
        // Draw Dots (Standard Dice Patterns)
        int m = size / 2;
        int q1 = size / 4, q3 = 3 * size / 4;
        
        if (n == 0) { drawDot(0, m, m); } // 1
        else if (n == 1) { drawDot(1, q1, q1); drawDot(1, q3, q3); } // 2
        else if (n == 2) { drawDot(2, q1, q1); drawDot(2, m, m); drawDot(2, q3, q3); } // 3
        else if (n == 3) { drawDot(3, q1, q1); drawDot(3, q3, q1); drawDot(3, q1, q3); drawDot(3, q3, q3); } // 4
        else if (n == 4) { drawDot(4, q1, q1); drawDot(4, q3, q1); drawDot(4, q1, q3); drawDot(4, q3, q3); drawDot(4, m, m); } // 5
        else if (n == 5) { drawDot(5, q1, q1); drawDot(5, q3, q1); drawDot(5, q1, q3); drawDot(5, q3, q3); drawDot(5, q1, m); drawDot(5, q3, m); } // 6
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size * 3, size * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

void initGeometry() {
    // Cube: Pos (3), Tex (2), Normal (3)
    float cubeVertices[] = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.33f, 0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.33f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.33f, 0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.5f,  0.0f,  0.0f, -1.0f,
        // Front face (similar UV logic needed for each face)
        // ... (truncated for brevity, using a fuller version in the file)
    };
    
    // Complete cube geometry with numbers mapped to 3x2 grid
    std::vector<float> vs;
    auto addFace = [&](Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n, int n_idx) {
        float tx = (n_idx % 3) / 3.0f;
        float ty = (n_idx / 3) / 2.0f;
        float tw = 1.0f / 3.0f;
        float th = 1.0f / 2.0f;
        
        float face[] = {
            p1.x, p1.y, p1.z, tx, ty, n.x, n.y, n.z,
            p2.x, p2.y, p2.z, tx+tw, ty, n.x, n.y, n.z,
            p3.x, p3.y, p3.z, tx+tw, ty+th, n.x, n.y, n.z,
            p3.x, p3.y, p3.z, tx+tw, ty+th, n.x, n.y, n.z,
            p4.x, p4.y, p4.z, tx, ty+th, n.x, n.y, n.z,
            p1.x, p1.y, p1.z, tx, ty, n.x, n.y, n.z
        };
        for (float f : face) vs.push_back(f);
    };
    
    addFace({-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {-0.5,0.5,-0.5}, {0,-0,-1}, 0); // Back
    addFace({-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5}, {0,0,1}, 1);   // Front
    addFace({-0.5,0.5,0.5}, {-0.5,0.5,-0.5}, {-0.5,-0.5,-0.5}, {-0.5,-0.5,0.5}, {-1,0,0}, 2); // Left
    addFace({0.5,0.5,0.5}, {0.5,0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,-0.5,0.5}, {1,0,0}, 3);   // Right
    addFace({-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,-0.5,0.5}, {-0.5,-0.5,0.5}, {0,-1,0}, 4);  // Bottom
    addFace({-0.5,0.5,-0.5}, {0.5,0.5,-0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5}, {0,1,0}, 5);    // Top

    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(float), vs.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); glEnableVertexAttribArray(2);

    // Mirror Plane
    float mirrorVs[] = {
        -5.0f, 0.0f, -5.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
         5.0f, 0.0f, -5.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
         5.0f, 0.0f,  5.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         5.0f, 0.0f,  5.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f,  5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f, -5.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f
    };
    glGenVertexArrays(1, &mirrorVAO); glGenBuffers(1, &mirrorVBO);
    glBindVertexArray(mirrorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mirrorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mirrorVs), mirrorVs, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); glEnableVertexAttribArray(2);
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, NULL);
    glCompileShader(s);
    return s;
}

GLuint linkProgram(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    return p;
}

void initFBO() {
    if (reflectionFBO) {
        glDeleteFramebuffers(1, &reflectionFBO);
        glDeleteTextures(1, &reflectionTexture);
        glDeleteRenderbuffers(1, &reflectionDepthRB);
    }

    glGenFramebuffers(1, &reflectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    
    glGenTextures(1, &reflectionTexture);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);
    
    glGenRenderbuffers(1, &reflectionDepthRB);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, viewportWidth, viewportHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflectionDepthRB);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float cubeAngle = 0.0f;
float lightAngle = 0.2f;

void render() {
    // Check for canvas resize
    int w, h;
    emscripten_get_canvas_element_size("#canvas", &w, &h);
    if (w != viewportWidth || h != viewportHeight) {
        viewportWidth = w;
        viewportHeight = h;
        initFBO();
    }

    cubeAngle += 0.02f;
    
    // Target the floating cube (at Y ≈ 2.5 for better clearance)
    Vec3 target = {0, 2.5f, 0}; 
    
    // Calculate camera position as an offset from the target
    // x = r * sin(phi) * sin(theta)
    // y = r * cos(phi)
    // z = r * sin(phi) * cos(theta)
    Vec3 camPos = {
        target.x + camRadius * sinf(camPhi) * sinf(camTheta),
        target.y + camRadius * cosf(camPhi),
        target.z + camRadius * sinf(camPhi) * cosf(camTheta)
    };
    Vec3 up = {0, 1, 0};
    
    // Light circles around the top
    // Vec3 lightPos = { 8.0f * cos(cubeAngle), 10.0f, 8.0f * sin(cubeAngle) };
    Vec3 lightPos = { 8.0f * cos(lightAngle), 10.0f, 8.0f * sin(lightAngle) };
    Vec3 lightColor = { 1.0f, 1.0f, 1.0f };
    float ambientStrength = 0.4f;

    // Adjust field of view for better perspective
    Mat4 projection = perspective(50.0f * (3.14159f / 180.0f), (float)viewportWidth / viewportHeight, 0.1f, 100.0f);
    
    // Pass 1: Reflection
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f); // Brighter background for reflection
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Reflected camera (Y is inverted since mirror is on XZ plane at Y=0)
    Vec3 refCamPos = { camPos.x, -camPos.y, camPos.z };
    Vec3 refTarget = { target.x, -target.y, target.z }; // Focus on reflected position
    Vec3 refCamUp = { 0, -1, 0 }; 
    Mat4 refView = lookAt(refCamPos, refTarget, refCamUp);
    
    // Invert winding order for reflection (mirror flips objects)
    glFrontFace(GL_CW);
    
    glUseProgram(cubeProgram);
    Mat4 model = rotateY(translate(identity(), {0, 2.0f + 0.5f * sin(cubeAngle * 2.0f), 0}), cubeAngle);
    glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "uView"), 1, GL_FALSE, refView.m);
    glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "uModel"), 1, GL_FALSE, model.m);
    glUniform3f(glGetUniformLocation(cubeProgram, "uViewPos"), refCamPos.x, refCamPos.y, refCamPos.z);
    glUniform3f(glGetUniformLocation(cubeProgram, "uLightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(cubeProgram, "uLightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform1f(glGetUniformLocation(cubeProgram, "uAmbientStrength"), ambientStrength);

    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Draw Light Source Helper (Reflection)
    glUseProgram(solidProgram);
    Mat4 lightModel = scale(translate(identity(), lightPos), {0.2f, 0.2f, 0.2f});
    glUniformMatrix4fv(glGetUniformLocation(solidProgram, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(solidProgram, "uView"), 1, GL_FALSE, refView.m);
    glUniformMatrix4fv(glGetUniformLocation(solidProgram, "uModel"), 1, GL_FALSE, lightModel.m);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    glFrontFace(GL_CCW);
    
    // Pass 2: Main Scene
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    Mat4 view = lookAt(camPos, target, up);
    
    // Render Cube
    glUseProgram(cubeProgram);
    glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "uView"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "uModel"), 1, GL_FALSE, model.m);
    glUniform3f(glGetUniformLocation(cubeProgram, "uViewPos"), camPos.x, camPos.y, camPos.z);
    // (uLightPos, uLightColor, uAmbientStrength are already set and program is same)
    
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Render Mirror
    glUseProgram(mirrorProgram);
    glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "uView"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "uModel"), 1, GL_FALSE, identity().m);
    glUniform2f(glGetUniformLocation(mirrorProgram, "uResolution"), (float)viewportWidth, (float)viewportHeight);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glUniform1i(glGetUniformLocation(mirrorProgram, "uReflectionTexture"), 0);

    glBindVertexArray(mirrorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 2;
    glContext = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(glContext);
    
    // Get actual canvas size
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    viewportWidth = (int)w; viewportHeight = (int)h;
    
    cubeProgram = linkProgram(cubeVertexShader, cubeFragmentShader);
    mirrorProgram = linkProgram(cubeVertexShader, mirrorFragmentShaderReal);
    solidProgram = linkProgram(cubeVertexShader, solidFragmentShader);
    
    initGeometry();
    cubeTexture = createNumberedTexture();
    initFBO();
    
    // Register interactive callbacks
    emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, on_mousedown);
    emscripten_set_mouseup_callback("#canvas", NULL, EM_TRUE, on_mouseup);
    emscripten_set_mousemove_callback("#canvas", NULL, EM_TRUE, on_mousemove);
    emscripten_set_wheel_callback("#canvas", NULL, EM_TRUE, on_wheel);

    emscripten_set_main_loop(render, 0, 1);
    
    return 0;
}
