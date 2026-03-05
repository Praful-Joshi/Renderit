// =============================================================================
//  sandbox.cpp — Textured Cube with Blinn-Phong Lighting
//
//  Fully self-contained raw OpenGL demo.
//  No abstractions, no Assimp, no project headers.
//  Every OpenGL call is explicit and commented.
//
//  Demonstrates:
//    - Vertex data layout (position + normal + UV, interleaved)
//    - VAO / VBO / EBO setup
//    - Vertex + fragment shaders (GLSL inline)
//    - MVP matrix transformations
//    - Per-face textures via texture switching between draw calls
//    - Blinn-Phong lighting (ambient + diffuse + specular + attenuation)
//    - Light visualizer cube (unlit shader)
//    - Depth testing and double buffering
//
//  Build:  cmake --build build --target Sandbox
//  Run:    ./build/Sandbox
//
//  Textures: place 5 square PNG images at these paths relative to build/:
//    ../sandbox/textures/face_front.png
//    ../sandbox/textures/face_back.png
//    ../sandbox/textures/face_left.png
//    ../sandbox/textures/face_right.png
//    ../sandbox/textures/face_top.png
//  If a texture fails to load, that face renders magenta as a fallback.
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <string>
#include <array>
#include <cmath>

// =============================================================================
//  CONFIG
// =============================================================================

static const int   WIN_W             = 900;
static const int   WIN_H             = 700;
static const char* WIN_TITLE         = "Textured Cube — Blinn-Phong";
static const float CUBE_ROTATE_SPEED = 20.0f;   // degrees per second around Y
static const float LIGHT_ORBIT_SPEED = 30.0f;   // degrees per second
static const float LIGHT_ORBIT_R     = 1.5f;    // orbit radius
static const float LIGHT_HEIGHT      = 1.5f;

// =============================================================================
//  GLSL — LIT SHADER
//  Full Blinn-Phong lighting: ambient + diffuse + specular + attenuation.
// =============================================================================

static const char* LIT_VERT = R"glsl(
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_fragPos;
out vec3 v_normal;
out vec2 v_texCoord;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position   = u_projection * u_view * worldPos;

    v_fragPos  = vec3(worldPos);
    v_texCoord = a_texCoord;

    // Normal matrix: transpose of inverse of model matrix.
    // Correctly handles non-uniform scaling of normals.
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    v_normal = normalize(normalMatrix * a_normal);
}
)glsl";

static const char* LIT_FRAG = R"glsl(
#version 330 core

in vec3 v_fragPos;
in vec3 v_normal;
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec3  u_lightPos;
uniform vec3  u_lightColor;
uniform vec3  u_cameraPos;
uniform float u_shininess;

out vec4 FragColor;

void main() {
    vec3 surfaceColor = texture(u_texture, v_texCoord).rgb;

    // --- Vectors ---
    // All must be unit length. Interpolation across the triangle can
    // slightly un-normalize them, so we renormalize in the fragment shader.
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_lightPos - v_fragPos);   // toward light
    vec3 V = normalize(u_cameraPos - v_fragPos);  // toward camera
    vec3 H = normalize(L + V);                    // Blinn halfway vector

    // --- Attenuation: light fades with distance ---
    float d           = length(u_lightPos - v_fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * d + 0.032 * d * d);

    // --- Ambient: constant indirect light ---
    vec3 ambient = 0.15 * u_lightColor * surfaceColor;

    // --- Diffuse: Lambert's cosine law ---
    // dot(N,L) = cos(angle). max() clamps when light is behind surface.
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * u_lightColor * surfaceColor;

    // --- Specular: Blinn-Phong highlight ---
    // pow() creates sharp falloff. shininess controls tightness.
    float spec     = pow(max(dot(N, H), 0.0), u_shininess);
    vec3  specular = spec * u_lightColor * vec3(0.4);

    // --- Combine ---
    // Ambient is NOT attenuated (it's scene-wide indirect light).
    // Diffuse and specular come from the point light, so they attenuate.
    vec3 result = ambient + (diffuse + specular) * attenuation;
    FragColor = vec4(result, 1.0);
}
)glsl";

// =============================================================================
//  GLSL — UNLIT SHADER
//  For the light visualizer cube — always renders at full brightness.
// =============================================================================

static const char* UNLIT_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
void main() {
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
)glsl";

static const char* UNLIT_FRAG = R"glsl(
#version 330 core
uniform vec3 u_color;
out vec4 FragColor;
void main() { FragColor = vec4(u_color, 1.0); }
)glsl";

// =============================================================================
//  VERTEX DATA
//
//  24 vertices: 6 faces x 4 corners each.
//  Each face has its own 4 vertices with unique normals and UVs.
//  Layout per vertex (8 floats = 32 bytes):
//    [0-2] position xyz
//    [3-5] normal   xyz
//    [6-7] texcoord uv
//
//  36 indices: 6 faces x 2 triangles x 3 vertices.
//  Pattern per face: 0,1,2 and 0,2,3 (CCW winding from outside).
// =============================================================================

static const float CUBE_VERTS[] = {
    // FRONT (z=+0.5, normal +Z)
    -0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,  0.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,  1.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,  1.0f,1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,  0.0f,1.0f,
    // BACK (z=-0.5, normal -Z)
     0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,0.0f,
    -0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,0.0f,
    -0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,1.0f,
     0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,1.0f,
    // LEFT (x=-0.5, normal -X)
    -0.5f,-0.5f,-0.5f,  -1.0f,0.0f,0.0f,  0.0f,0.0f,
    -0.5f,-0.5f, 0.5f,  -1.0f,0.0f,0.0f,  1.0f,0.0f,
    -0.5f, 0.5f, 0.5f,  -1.0f,0.0f,0.0f,  1.0f,1.0f,
    -0.5f, 0.5f,-0.5f,  -1.0f,0.0f,0.0f,  0.0f,1.0f,
    // RIGHT (x=+0.5, normal +X)
     0.5f,-0.5f, 0.5f,  1.0f,0.0f,0.0f,  0.0f,0.0f,
     0.5f,-0.5f,-0.5f,  1.0f,0.0f,0.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  1.0f,0.0f,0.0f,  1.0f,1.0f,
     0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,  0.0f,1.0f,
    // TOP (y=+0.5, normal +Y)
    -0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  0.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,  1.0f,1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,  0.0f,1.0f,
    // BOTTOM (y=-0.5, normal -Y) — not visible but included
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,  0.0f,0.0f,
     0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,  1.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,  1.0f,1.0f,
    -0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,  0.0f,1.0f,
};

static const unsigned int CUBE_INDICES[] = {
     0, 1, 2,  0, 2, 3,   // front
     4, 5, 6,  4, 6, 7,   // back
     8, 9,10,  8,10,11,   // left
    12,13,14, 12,14,15,   // right
    16,17,18, 16,18,19,   // top
    20,21,22, 20,22,23,   // bottom
};

// =============================================================================
//  HELPERS
// =============================================================================

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "[Shader compile error]\n" << log << "\n";
    }
    return s;
}

static GLuint buildProgram(const char* vert, const char* frag) {
    GLuint v = compileShader(GL_VERTEX_SHADER,   vert);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, frag);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "[Program link error]\n" << log << "\n";
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static GLuint loadTexture(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data) {
        std::cout << "[Texture] " << path << " (" << w << "x" << h << ")\n";
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        std::cerr << "[Texture] Failed: " << path << " — using magenta fallback\n";
        unsigned char magenta[] = {255, 0, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magenta);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static void onResize(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

// =============================================================================
//  MAIN
// =============================================================================

int main() {

    // ── Window + context ─────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, WIN_TITLE, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << "\n";

    // ── Shaders ───────────────────────────────────────────────────────────────
    GLuint litProg   = buildProgram(LIT_VERT,   LIT_FRAG);
    GLuint unlitProg = buildProgram(UNLIT_VERT, UNLIT_FRAG);

    // Cache all uniform locations — string lookup, do once not per frame
    GLint litModel  = glGetUniformLocation(litProg, "u_model");
    GLint litView   = glGetUniformLocation(litProg, "u_view");
    GLint litProj   = glGetUniformLocation(litProg, "u_projection");
    GLint litTex    = glGetUniformLocation(litProg, "u_texture");
    GLint litLPos   = glGetUniformLocation(litProg, "u_lightPos");
    GLint litLCol   = glGetUniformLocation(litProg, "u_lightColor");
    GLint litCam    = glGetUniformLocation(litProg, "u_cameraPos");
    GLint litShine  = glGetUniformLocation(litProg, "u_shininess");
    GLint ulModel   = glGetUniformLocation(unlitProg, "u_model");
    GLint ulView    = glGetUniformLocation(unlitProg, "u_view");
    GLint ulProj    = glGetUniformLocation(unlitProg, "u_projection");
    GLint ulColor   = glGetUniformLocation(unlitProg, "u_color");

    // ── Upload geometry ───────────────────────────────────────────────────────
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);  // start recording layout into VAO

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

        const GLsizei STRIDE = 8 * sizeof(float);  // 32 bytes per vertex

        // location 0: position (3 floats, offset 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
        glEnableVertexAttribArray(0);
        // location 1: normal (3 floats, offset 12)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        // location 2: texcoord (2 floats, offset 24)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);

    glBindVertexArray(0);  // stop recording — VAO is configured

    // ── Textures — one per visible face ──────────────────────────────────────
    std::array<GLuint, 6> textures = {
        loadTexture("../sandbox/textures/face_front.png"),
        loadTexture("../sandbox/textures/face_back.png"),
        loadTexture("../sandbox/textures/face_left.png"),
        loadTexture("../sandbox/textures/face_right.png"),
        loadTexture("../sandbox/textures/face_top.png"),
        loadTexture("../sandbox/textures/face_bottom.png"),  // bottom not visible
    };

    // ── Camera ────────────────────────────────────────────────────────────────
    glm::vec3 cameraPos  = glm::vec3(0.0f, 3.5f, 6.0f);
    glm::mat4 view       = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            (float)WIN_W/WIN_H, 0.1f, 100.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 0.95f, 0.85f);

    float cubeRot  = 0.0f;
    float lightAng = 0.0f;
    float lastTime = (float)glfwGetTime();

    // ── Render loop ───────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {

        float now = (float)glfwGetTime();
        float dt  = now - lastTime;
        lastTime  = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Update
        cubeRot  += CUBE_ROTATE_SPEED * dt;
        lightAng += LIGHT_ORBIT_SPEED * dt;

        glm::mat4 cubeModel = glm::rotate(glm::mat4(1.0f),
                                          glm::radians(cubeRot),
                                          glm::vec3(0,1,0));

        float lr = glm::radians(lightAng);
        glm::vec3 lightPos(std::cos(lr) * LIGHT_ORBIT_R,
                           LIGHT_HEIGHT,
                           std::sin(lr) * LIGHT_ORBIT_R);

        // Clear
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ── Draw main cube (lit, textured) ────────────────────────────────────
        glUseProgram(litProg);
        glUniformMatrix4fv(litModel, 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniformMatrix4fv(litView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(litProj,  1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(litLPos,  1, glm::value_ptr(lightPos));
        glUniform3fv(litLCol,  1, glm::value_ptr(lightColor));
        glUniform3fv(litCam,   1, glm::value_ptr(cameraPos));
        glUniform1f (litShine, 64.0f);
        glUniform1i (litTex,   0);        // sampler2D reads texture unit 0

        glBindVertexArray(vao);

        // Draw each face separately with its own texture.
        // Each face = 6 indices, starting at byte offset face*6*4.
        for (int face = 0; face < 6; ++face) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[face]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                           (void*)(face * 6 * sizeof(unsigned int)));
        }

        // ── Draw light cube (unlit, white) ────────────────────────────────────
        glUseProgram(unlitProg);
        glm::mat4 lcModel = glm::scale(glm::translate(glm::mat4(1.0f), lightPos),
                                       glm::vec3(0.18f));
        glUniformMatrix4fv(ulModel, 1, GL_FALSE, glm::value_ptr(lcModel));
        glUniformMatrix4fv(ulView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(ulProj,  1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(ulColor, 1, glm::value_ptr(lightColor));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (GLuint t : textures) glDeleteTextures(1, &t);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(litProg);
    glDeleteProgram(unlitProg);
    glfwTerminate();
    return 0;
}