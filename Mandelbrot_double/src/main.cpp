#include "NVIDIA/NVIDIA.h"
#include "Shaders/Fragment_Shader.h"
#include "Shaders/Vertex_Shader.h"



#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>



unsigned int SCR_WIDTH = 0;
unsigned int SCR_HEIGHT = 0;






/*
double Mandelbrot(double real, double imag, double max_mod_z, int maxIter){
    std::vector<double> Z(2, 0);
    double t;

    for(int i = 0; i < maxIter; i++){
        double new_return2 = Z[0]*Z[0] - Z[1]*Z[1] + real;
        
        Z[1] = 2*Z[0]*Z[1] + imag;
        Z[0] = new_return2;
        
        double z_mod = Z[0]*Z[0] + Z[1]*Z[1];

        t = (i + 1 - log2(log2(sqrt(z_mod)))) / maxIter;

        if (z_mod > max_mod_z) 
            return t;
    }
    return t;
}


std::vector<unsigned char> Mandelbrot_Pixels(){
    std::vector<unsigned char> pixels(SCR_WIDTH * SCR_HEIGHT * 3);

    double xMin = -2.0, xMax = 2.0;
    double yMin = -2.0, yMax = 2.0;



    int maxIter = 1000;
    double max_mod_z = 1000000000000.0;
    double cycles = 100;

    for (int y = 0; y < SCR_HEIGHT; y++)
        for(int x = 0; x < SCR_WIDTH; x++){
            double real = xMin + (double)x / SCR_WIDTH * (xMax - xMin);
            double imag = yMin + (double)y / SCR_HEIGHT * (yMax - yMin);

            double iter = Mandelbrot(real, imag, max_mod_z, maxIter);
            iter = iter * cycles - (float)iter;

            int idx = 3*(x + y*SCR_WIDTH);
            
            double R = sin(iter * 6.28318 + 0.0) * 0.5 + 0.5;
            double G = sin(iter * 6.28318 + 2.094) * 0.5 + 0.5;
            double B = sin(iter * 6.28318 + 4.188) * 0.5 + 0.5;

            pixels[idx] = (unsigned char)(R * 255);     // Красный
            pixels[idx + 1] = (unsigned char)(G * 255); // Зеленый
            pixels[idx + 2] = (unsigned char)(B * 255);   // Синий
            
        }
    
    return pixels;
}
*/



GLFWwindow* Start_Window(){
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    SCR_WIDTH = mode -> width;
    SCR_HEIGHT = mode -> height;
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    return window;
}


GLuint Create_Texture(std::vector<unsigned char>& pixels){
    GLuint texture; 
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Настройки текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Загружаем пиксели в текстуру
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 
                 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    return texture;
}


void VAO_VBO_EBO(GLuint& VAO, GLuint& VBO, GLuint& EBO, float vertices[], unsigned int indices[], unsigned int sizeof_vertices, unsigned int sizeof_indices){
    // Создаем VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Создаем VBO и загружаем вершины
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof_vertices, vertices, GL_STATIC_DRAW);
    
    // Создаем EBO и загружаем индексы
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_indices, indices, GL_STATIC_DRAW);
    
    // Настраиваем атрибуты вершин
    // Атрибут 0: позиция (2 float)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Атрибут 1: текстурные координаты (2 float)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}


GLuint Create_Shader_Program(){
        // Компилируем вершинный шейдер
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Проверяем ошибки компиляции
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error:\n" << infoLog << std::endl;
    }
    
    // Компилируем фрагментный шейдер
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error:\n" << infoLog << std::endl;
    }




    // Создаем шейдерную программу
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Program Error:\n" << infoLog << std::endl;
    }
    
    // Удаляем шейдеры (они уже в программе)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}



int main() {
    GLFWwindow* window = Start_Window();
    
    


    float vertices[] = {
        // Позиция      // Текстурные координаты
        -1.0f, -1.0f,   0.0f, 0.0f,  // нижний левый
         1.0f, -1.0f,   1.0f, 0.0f,  // нижний правый
         1.0f,  1.0f,   1.0f, 1.0f,  // верхний правый
        -1.0f,  1.0f,   0.0f, 1.0f   // верхний левый
    };
    
    // Индексы для двух треугольников
    unsigned int indices[] = {
        0, 1, 2,  // первый треугольник
        0, 2, 3   // второй треугольник
    };
    
    GLuint VAO, VBO, EBO;
    
    VAO_VBO_EBO(VAO, VBO, EBO, vertices, indices, sizeof(vertices), sizeof(indices));
    


    GLuint shaderProgram = Create_Shader_Program();



    
    


    // ====== ПАРАМЕТРЫ ФРАКТАЛА ======
    double offsetX = -0.5, offsetY = 0.0;
    double zoom = 1.0;
    double zoom_speed = 1.01;
    int maxIter = 1000;
    float colorCycles = 80.0;

    GLint uResolution = glGetUniformLocation(shaderProgram, "uResolution");
    GLint uOffset = glGetUniformLocation(shaderProgram, "uOffset");
    GLint uZoom = glGetUniformLocation(shaderProgram, "uZoom");
    GLint uMaxIter = glGetUniformLocation(shaderProgram, "uMaxIter");
    GLint uColorCycles = glGetUniformLocation(shaderProgram, "uColorCycles");



    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);


    while (!glfwWindowShouldClose(window)) {
                // ====== УПРАВЛЕНИЕ ======
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Движение (WASD)
        float moveSpeed = 0.01 / zoom;
        if (glfwGetKey(window, GLFW_KEY_W)) offsetY += moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_S)) offsetY -= moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_A)) offsetX -= moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_D)) offsetX += moveSpeed;
        
        // Зум (Стрелки)
        if (glfwGetKey(window, GLFW_KEY_UP)) zoom *= zoom_speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN)) zoom /= zoom_speed;
        
        // Качество (R - больше, F - меньше)
        if (glfwGetKey(window, GLFW_KEY_R)) maxIter += 50;
        if (glfwGetKey(window, GLFW_KEY_F)) maxIter = std::max(50, maxIter - 50);
        
        // Количество циклов цвета (1-9)
        if (glfwGetKey(window, GLFW_KEY_1)) colorCycles /= 2.0;
        if (glfwGetKey(window, GLFW_KEY_2)) colorCycles *= 2.0;

        
        // ====== ОТРИСОВКА ======
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Передаем параметры в шейдер
        glUniform2f(uResolution, (float)SCR_WIDTH, (float)SCR_HEIGHT);
        glUniform2f(uOffset, (float)offsetX, (float)offsetY);
        glUniform1f(uZoom, (float)zoom);
        glUniform1i(uMaxIter, maxIter);
        glUniform1f(uColorCycles, colorCycles);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    

    


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}