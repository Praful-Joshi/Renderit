#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace Renderer {
/**
 * @brief Shader class responsible for managing vertex
 *          and fragment shaders.
 *
 *  - constructor(): Compiles both shaders. Creates a 
 *                  shader program and saves its ID in
 *                  m_programID. Links compiled shaders 
 *                  to this program.
 * 
 */
class Shader {
public:
    // Read GLSL files from disk, compile and link on construction.
    // Throws std::runtime_error if compilation or linking fails.
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Non-copyable — owns a GPU program handle
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Movable
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Bind this shader program for subsequent draw calls
    void bind()   const;
    void unbind() const;

    GLuint getID() const { return m_programID; }

    // ── Uniform setters ──────────────────────────────────────────────────────
    // Each one binds the program, sets the value, done.
    // Named clearly so call sites read like English:
    //   shader.setInt("textureSampler", 0);
    //   shader.setMat4("model", modelMatrix);

    void setBool (const std::string& name, bool value)               const;
    void setInt  (const std::string& name, int value)                const;
    void setFloat(const std::string& name, float value)              const;
    void setVec2 (const std::string& name, const glm::vec2& value)   const;
    void setVec3 (const std::string& name, const glm::vec3& value)   const;
    void setVec4 (const std::string& name, const glm::vec4& value)   const;
    void setMat3 (const std::string& name, const glm::mat3& value)   const;
    void setMat4 (const std::string& name, const glm::mat4& value)   const;

private:
    GLuint m_programID = 0;

    // Read a file into a string. Throws if file cannot be opened.
    static std::string  readFile(const std::string& path);

    // Compile a single shader stage. Throws if compilation fails.
    static GLuint       compileShader(GLenum type, const std::string& source);

    // Check and report errors
    static void         checkCompileErrors(GLuint shader, const std::string& label);
    static void         checkLinkErrors(GLuint program);

    // Cached uniform location lookup — avoids string hashing every frame
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace Renderer