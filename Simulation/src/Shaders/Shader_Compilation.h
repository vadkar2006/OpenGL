#pragma once

#include <iostream>
#include <fstream>
#include <sstream>

std::string readShaderFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Compilation_VertexFragment_Shader_Program() {
    // Определяем пути к шейдерам (относительно рабочей директории)
    std::string vertexPath = "../src/Shaders/Vertex_Shader.glsl";
    std::string fragmentPath = "../src/Shaders/Fragment_Shader.glsl";
    
    // Читаем шейдеры из файлов
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);
    
    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Failed to read shader files!" << std::endl;
        return 0;
    }
    
    // Компилируем вершинный шейдер
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexSource = vertexCode.c_str();
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error:\n" << infoLog << std::endl;
        return 0;
    }
    
    // Компилируем фрагментный шейдер
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentSource = fragmentCode.c_str();
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error:\n" << infoLog << std::endl;
        return 0;
    }
    
    // Создаем программу
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Program Error:\n" << infoLog << std::endl;
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

GLuint Compilation_Compute_Shader_Program(std::string name_file){
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        std::string file = readShaderFile("../src/Shaders/" + name_file);
        const char* computeSrc = file.c_str();
        glShaderSource(computeShader, 1, &computeSrc, NULL);
        glCompileShader(computeShader);

                // === 1. ПРОВЕРКА КОМПИЛЯЦИИ ШЕЙДЕРА ===
        int compileSuccess;
        char infoLog[1024]; // Увеличил буфер, так как ошибки шейдеров могут быть длинными
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &compileSuccess);
        if (!compileSuccess) {
            glGetShaderInfoLog(computeShader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
            // Стоит ли здесь выбрасывать исключение или завершать программу?
        }

        GLuint proga = glCreateProgram();
        glAttachShader(proga, computeShader);
        glLinkProgram(proga);

        // === 2. ПРОВЕРКА ЛИНКОВКИ ПРОГРАММЫ ===
        int linkSuccess;
        glGetProgramiv(proga, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            glGetProgramInfoLog(proga, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(computeShader);

        return proga;
    }

