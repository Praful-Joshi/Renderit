// ─────────────────────────────────────────────────────────────────────────────
//  sandbox.cpp — self-contained OpenGL scratchpad
//
//  Everything in one file: window, shaders, buffers, draw loop.
//  No wrappers, no abstractions — raw OpenGL calls only.
//  Use this to prototype ideas, demo concepts, or debug in isolation.
//
//  Build:  cmake --build build --target Sandbox
//  Run:    ./build/Sandbox
// ─────────────────────────────────────────────────────────────────────────────

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  Config — change these to explore
// ─────────────────────────────────────────────────────────────────────────────
static const int   WINDOW_W  = 800;
static const int   WINDOW_H  = 600;
static const char* TITLE     = "Sandbox";

// ─────────────────────────────────────────────────────────────────────────────
//  GLSL source strings — inline so the file is fully self-contained.
//  In production code these would be .vert/.frag files on disk.
// ─────────────────────────────────────────────────────────────────────────────
static const char* VERT_SRC = R"glsl(
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

uniform mat4 u_mvp;

out vec3 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color     = a_color;
}
)glsl";

static const char* FRAG_SRC = R"glsl(
#version 330 core

in  vec3 v_color;
out vec4 FragColor;

void main() {
    FragColor = vec4(v_color, 1.0);
}
)glsl";

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers — minimal inline versions of what our wrappers do
// ─────────────────────────────────────────────────────────────────────────────

// Compile one shader stage, print errors, return ID
static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "[Shader error] " << log << "\n";
    }
    return shader;
}

// Link vert + frag into a program, return ID
static GLuint linkProgram(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "[Link error] " << log << "\n";
    }
    return prog;
}

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main() {

    // ── Window + context ─────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, TITLE, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    // ── Shader program ───────────────────────────────────────────────────────
    GLuint vert    = compileShader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint program = linkProgram(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // ── Geometry — a colored triangle ────────────────────────────────────────
    // Layout per vertex: [ x, y, z,   r, g, b ]
    float vertices[] = {
        // position          color
         0.0f,  0.5f, 0.0f,  1.0f, 0.3f, 0.3f,  // top    — red
        -0.5f, -0.5f, 0.0f,  0.3f, 1.0f, 0.3f,  // left   — green
         0.5f, -0.5f, 0.0f,  0.3f, 0.3f, 1.0f,  // right  — blue
    };

    // ── VAO / VBO — raw setup showing exactly what our Buffer class wraps ────
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position: location 0, 3 floats, stride 24, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color: location 1, 3 floats, stride 24, offset 12
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // ── Uniforms — get locations once before the loop ────────────────────────
    GLint mvpLoc = glGetUniformLocation(program, "u_mvp");

    // ── Matrices ─────────────────────────────────────────────────────────────
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 3),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_W / WINDOW_H,
        0.1f, 100.0f
    );

    float rotation = 0.0f;
    float lastTime = (float)glfwGetTime();

    // ── Render loop ───────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {

        float now       = (float)glfwGetTime();
        float deltaTime = now - lastTime;
        lastTime        = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Update
        rotation += 45.0f * deltaTime;
        glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                      glm::radians(rotation),
                                      glm::vec3(0, 1, 0));
        glm::mat4 mvp = proj * view * model;

        // Render
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    glfwTerminate();

    return 0;
}