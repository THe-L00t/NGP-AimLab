#pragma once
#include "Render/GL.h"
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader() { destroy(); }
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void destroy();
    void use() const { glUseProgram(program_); }

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setInt(const char* name, int v) const;   // 정수·bool·텍스처 유닛 번호

private:
    static std::string readFile(const std::string& path);
    static GLuint compile(const std::string& source, GLenum type, const std::string& path);

    GLuint program_ = 0;
};
