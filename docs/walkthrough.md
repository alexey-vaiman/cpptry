# Walkthrough: C++ WebGL Mirror Cube with WebAssembly

This project demonstrates a high-performance 3D scene rendered using C++ compiled to WebAssembly, utilizing WebGL 2.0 for graphics and interactive controls.

## 🚀 Key Achievements

- **Multipass Reflection**: Implemented a real-time mirror effect by rendering the scene from a reflected perspective into an off-screen Framebuffer Object (FBO).
- **Interactive Orbit Camera**: Developed a stable spherical camera system with mouse-drag rotation and scroll-to-zoom, centered on the floating cube.
- **High-DPI & Responsive Design**: Added automatic canvas resizing and resolution scaling based on `devicePixelRatio` to ensure crisp visuals on All displays (Retina/4K).
- **Automated CI/CD**: Set up a GitHub Actions workflow that compiles the C++ code using the Emscripten SDK and deploys the production build to GitHub Pages.

---

## 🏗️ System Architecture

### 1. The Core (C++)
Located in `main.cpp`, this is the heart of the application.
- **Emscripten Integration**: Uses the HTML5 API to capture mouse/wheel events and bridge them to the C++ logic.
- **Spherical Orbit Math**: Calculates the camera's Cartesian position based on user input (`theta`, `phi`, `radius`) relative to a fixed target point.
- **Procedural Textures**: Generates a 384x256 texture atlas containing the 6 faces of a die with dot patterns (1-6).

### 2. Shaders (GLSL 3.0 ES)
Found in `shaders.h`, these programs run directly on the GPU.
- **Cube Shader**: Implements Blinn-Phong lighting (Ambient + Diffuse + Specular) to give the cube a metallic/plastic 3D look.
- **Mirror Shader**: 
    - Samples the reflection texture using screen-space UVs.
    - Adds a procedural grid based on world-space coordinates (`vFragPos.xz`) to provide a sense of scale and ground orientation.

### 3. Rendering Logic (The Multipass)
1. **Reflection Pass**: 
    - Bind the FBO.
    - Flip the camera over the Y=0 plane.
    - Invert the winding order (`glFrontFace(GL_CW)`).
    - Render the cube into the texture.
2. **Main Pass**:
    - Bind the screen.
    - Render the cube normally.
    - Render the mirror plane using the texture from Pass 1.

---

## 🛠️ How to Build & Run Locally

1. **Install Emscripten**:
   ```bash
   chmod +x setup_emsdk.sh
   ./setup_emsdk.sh
   source emsdk/emsdk_env.sh
   ```
2. **Compile**:
   ```bash
   ./build.sh
   ```
3. **Serve**:
   ```bash
   python3 -m http.server 8000
   ```
   Open `http://localhost:8000` in your browser.

---

## 🌐 Deployment Logic

The file `.github/workflows/deploy.yml` automates everything. Whenever you push to the `master` branch:
1. A GitHub runner spawns.
2. It installs the Emscripten SDK.
3. It compiles the project into `index.js` and `index.wasm`.
4. It extracts only the needed files into a `/dist` folder.
5. It pushes that folder to the `gh-pages` branch, which serves your site.

---

## ✅ Verification Results

- [x] WebAssembly module loads and initializes.
- [x] WebGL 2.0 context created with High-DPI support.
- [x] Reflection is correctly aligned with the original cube.
- [x] Interactive controls work smoothly without gimbal lock.
- [x] CI/CD pipeline successfully builds and deploys.

> [!TIP]
> If you want to change the cube's floating height or movement speed, look for `cubeAngle` and the `target` vector in `main.cpp`.
