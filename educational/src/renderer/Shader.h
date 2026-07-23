#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace Renderer
{

/**
 * @class Shader
 * @brief RAII wrapper around an OpenGL shader program.
 * 
 * Responsible for:
 *  - Reading GLSL files from disk
 *  - Compiling vertex and fragment shaders
 *  - Linking them into a program
 *  - Providing uniform setters
 *
 * Ownership:
 *  - Owns one OpenGL program (GLuint)
 *  - Non-copyable
 *  - Movable
 *
 * Throws std::runtime_error if compilation or linking fails.
 */
class Shader
{
  public:
    /**
     * @brief Constructs and links a shader program from two GLSL files.
     *
     * @param vertexPath Path to vertex shader file.
     * @param fragmentPath Path to fragment shader file.
     *
     * @throws std::runtime_error if file reading, compilation, or linking fails.
     */
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Activates this shader program.
     */
    void bind() const;

    /**
     * @brief Deactivates any active shader program.
     */
    void unbind() const;

    /**
     * @return OpenGL program ID.
     */
    GLuint getID() const
    {
        return m_glShaderProgramID;
    }

    /// Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

  private:
    GLuint m_glShaderProgramID = 0;

    static std::string readFile(const std::string& path);
    static GLuint      compileShader(GLenum type, const std::string& source);
    static void        checkCompileErrors(GLuint shader, const std::string& label);
    static void        checkLinkErrors(GLuint program);

    /**
     * @brief Retrieves uniform location.
     *
     * @warning This performs a driver call. Consider caching for performance.
     */
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace Renderer