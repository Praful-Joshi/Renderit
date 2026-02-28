#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace Renderer {

// ─────────────────────────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertSource = readFile(vertexPath);
    std::string fragSource = readFile(fragmentPath);

    GLuint vertShader = compileShader(GL_VERTEX_SHADER,   vertSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource);

    // Link both compiled stages into a program
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertShader);
    glAttachShader(m_programID, fragShader);
    glLinkProgram(m_programID);
    checkLinkErrors(m_programID);

    // Compiled stages are now baked into the program — release them
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    std::cout << "[Shader] Linked program " << m_programID
              << " (" << vertexPath << ", " << fragmentPath << ")\n";
}

Shader::~Shader() {
    if (m_programID)
        glDeleteProgram(m_programID);
}

Shader::Shader(Shader&& other) noexcept
    : m_programID(other.m_programID)
{
    other.m_programID = 0; // transfer ownership, invalidate source
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programID) glDeleteProgram(m_programID);
        m_programID       = other.m_programID;
        other.m_programID = 0;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bind / Unbind
// ─────────────────────────────────────────────────────────────────────────────

void Shader::bind()   const { glUseProgram(m_programID); }
void Shader::unbind() const { glUseProgram(0); }

// ─────────────────────────────────────────────────────────────────────────────
//  Uniform Setters
// ─────────────────────────────────────────────────────────────────────────────

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
    // GL_FALSE: GLM is already column-major, matching OpenGL's expectation
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Private Helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("[Shader] Cannot open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    std::string label = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    checkCompileErrors(shader, label);

    return shader;
}

void Shader::checkCompileErrors(GLuint shader, const std::string& label) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        throw std::runtime_error(
            "[Shader] " + label + " compilation failed:\n" + log
        );
    }
}

void Shader::checkLinkErrors(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        throw std::runtime_error(
            std::string("[Shader] Program linking failed:\n") + log
        );
    }
}

GLint Shader::getUniformLocation(const std::string& name) const {
    // glGetUniformLocation is a driver call — doing it every frame for every
    // uniform is wasteful. A proper cache would store results in an
    // unordered_map<string, GLint>. We keep it simple for now and will
    // add the cache in the ResourceManager step.
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location == -1)
        std::cerr << "[Shader] Warning: uniform '" << name << "' not found\n";
    return location;
}

} // namespace Renderer