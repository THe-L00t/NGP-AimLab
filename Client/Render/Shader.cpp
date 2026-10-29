#include "Render/Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compile(const std::string& source, GLenum type, const std::string& path) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "셰이더 컴파일 실패 (" << path << "):\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    destroy();
    const std::string vs = readFile(vertexPath);
    const std::string fs = readFile(fragmentPath);
    if (vs.empty() || fs.empty()) {
        std::cerr << "셰이더 파일을 열 수 없습니다: " << (vs.empty() ? vertexPath : fragmentPath) << "\n";
        return false;
    }

    GLuint v = compile(vs, GL_VERTEX_SHADER, vertexPath);
    GLuint f = compile(fs, GL_FRAGMENT_SHADER, fragmentPath);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, v);
    glAttachShader(program_, f);
    glLinkProgram(program_);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::cerr << "셰이더 링크 실패 (" << vertexPath << "):\n" << log << "\n";
        destroy();
        return false;
    }
    std::cout << "셰이더 로드: " << vertexPath << ", " << fragmentPath << "\n";
    return true;
}

void Shader::destroy() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(v));
}

void Shader::setVec4(const char* name, const glm::vec4& v) const {
    glUniform4fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(v));
}

void Shader::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(program_, name), v);
}
