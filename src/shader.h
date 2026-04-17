#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string   vertCode, fragCode;
        std::ifstream vFile, fFile;

        vFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vFile.open(vertexPath);
            fFile.open(fragmentPath);
            std::stringstream vss, fss;
            vss << vFile.rdbuf();
            fss << fFile.rdbuf();
            vertCode = vss.str();
            fragCode = fss.str();
        } catch (std::ifstream::failure& e) {
            std::cerr << "[Shader] File read error: " << e.what()
                      << "\n  vert: " << vertexPath
                      << "\n  frag: " << fragmentPath << std::endl;
        }

        const char* vSrc = vertCode.c_str();
        const char* fSrc = fragCode.c_str();

        unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vSrc, NULL);
        glCompileShader(vert);
        checkErrors(vert, "VERTEX");

        unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fSrc, NULL);
        glCompileShader(frag);
        checkErrors(frag, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        checkErrors(ID, "PROGRAM");

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(ID); }

    void setBool (const std::string& n, bool  v)              const { glUniform1i (glGetUniformLocation(ID, n.c_str()), (int)v); }
    void setInt  (const std::string& n, int   v)              const { glUniform1i (glGetUniformLocation(ID, n.c_str()), v); }
    void setFloat(const std::string& n, float v)              const { glUniform1f (glGetUniformLocation(ID, n.c_str()), v); }
    void setVec3 (const std::string& n, const glm::vec3& v)   const { glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v)); }
    void setMat4 (const std::string& n, const glm::mat4& v)   const { glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(v)); }

private:
    void checkErrors(unsigned int obj, const std::string& type) {
        int  success;
        char log[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(obj, 1024, NULL, log);
                std::cerr << "[Shader] Compile error (" << type << "):\n" << log << std::endl;
            }
        } else {
            glGetProgramiv(obj, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(obj, 1024, NULL, log);
                std::cerr << "[Shader] Link error:\n" << log << std::endl;
            }
        }
    }
};
