#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>


#include "NVIDIA/NVIDIA.h"
#include "Shaders/Shader_Compilation.h"



unsigned int SCR_WIDTH = 0;
unsigned int SCR_HEIGHT = 0;









GLFWwindow* Start_Window(){
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }
    
    // === КЛЮЧЕВОЕ ИЗМЕНЕНИЕ ===
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Для macOS (если вдруг) нужно еще:
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSwapInterval(0);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    return window;
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
    // Атрибут 0: позиция (3 float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Атрибут 1: цвет (3 float)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

GLuint Compilation_Compute_Shader_VideoCard_Program(std::string name_file){
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





int frame_count_go_program = 1;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT)) frame_count_go_program++;
    if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT) && frame_count_go_program >= 0) frame_count_go_program--;
}/*
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_1 && (action == GLFW_PRESS || action == GLFW_REPEAT)) zoom /= 1.003;
    if (button == GLFW_MOUSE_BUTTON_2 && (action == GLFW_PRESS || action == GLFW_REPEAT)) zoom *= 1.003; 
}
*/

float lastX = 400.0;
float lastY = 300.0;
float div_pos_cam_x = 0.0;
float div_pos_cam_y = 0.0;
bool firstMouse = true;



void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if(firstMouse){
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    div_pos_cam_x = xpos - lastX;
    div_pos_cam_y = lastY - ypos;

    lastX = xpos;
    lastY = ypos;
}

/*
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        std::cout << "Левая кнопка мыши нажата (например, выстрел)" << std::endl;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        std::cout << "Правая кнопка мыши нажата (например, прицеливание)" << std::endl;
    }
}

// 3. Колесико прокрутки (для зума или смены оружия)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // yoffset обычно равен 1.0 (вверх) или -1.0 (вниз) за один щелчок
    std::cout << "Прокрутка: " << yoffset << std::endl;
    // Например: camera.ProcessMouseScroll(yoffset);
}*/


GLuint Create_Camera_Pos(){
    GLuint Camera_ssbo;
    std::vector<float> veccam(4*4, 0.0);

    veccam[4] = 1.0;
    veccam[9] = 1.0;
    veccam[14] = 1.0;

    glCreateBuffers(1, &Camera_ssbo);
    glNamedBufferStorage(Camera_ssbo, 
                        4 * 4 * sizeof(float),
                        nullptr, 
                        GL_DYNAMIC_STORAGE_BIT);
    
    
    glNamedBufferSubData(Camera_ssbo, 0, 
                        4 * 4 * sizeof(float), 
                        veccam.data());
    
    return Camera_ssbo;
}



int main(){
    GLFWwindow* window = Start_Window();
    if (!window) return -1;
    
    GLuint shaderProgram = Create_Shader_Program();
    if (shaderProgram == 0) return -1;

    glEnable(GL_DEPTH_TEST);


    // Данные для полноэкранного квада
    float vertices[] = {
        // positions            // color
        -0.5f,  0.3f, 0.5f,     1.0f, 0.0f, 0.0f, 
        -0.5f, -0.3f, 0.5f,     0.0f, 0.0f, 0.0f, 
         0.5f, -0.3f, 0.5f,     0.5f, 0.0f, 0.0f, 
         0.5f,  0.3f, 0.5f,     0.5f, 0.5f, 0.0f,

        -0.5f,  0.3f, 1.0f,     1.0f, 0.0f, 0.0f, 
        -0.5f, -0.3f, 1.0f,     0.0f, 0.0f, 0.0f, 
         0.5f, -0.3f, 1.0f,     0.5f, 0.0f, 0.0f, 
         0.5f,  0.3f, 1.0f,     0.5f, 0.5f, 0.0f,
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,

        0, 1, 4,
        1, 4, 5,

        1, 2, 6,
        1, 6, 5,

        2, 3, 7,
        2, 7, 6,

        3, 0, 4,
        3, 4, 7,

        4, 5, 6,
        4, 6, 7

    };

    GLuint VAO, VBO, EBO;
    VAO_VBO_EBO(VAO, VBO, EBO, vertices, indices, sizeof(vertices), sizeof(indices));



    GLuint Camera_Program = Compilation_Compute_Shader_VideoCard_Program("Compute_Shader.glsl");


    // --- Регистрация колбэков ввода ---
    glfwSetKeyCallback(window, keyCallback);/*
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
*/
    glfwSetCursorPosCallback(window, mouse_callback);

    GLuint Camera_ssbo = Create_Camera_Pos();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, Camera_ssbo);




    GLuint render_window = glGetUniformLocation(shaderProgram, "window");

    GLuint Camera_view = glGetUniformLocation(Camera_Program, "Camera_view");
    GLuint Camera_Go_see = glGetUniformLocation(Camera_Program, "Go_see");


    int fly_to_camera[4] = {0, 0, 0, 0};
    
    double lastTime = glfwGetTime();
    int frameCount = 0;
    int total_step = 0;
    while (!glfwWindowShouldClose(window)) {
        for(int i = 0; i < 4; i++)
            fly_to_camera[i] = 0;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) fly_to_camera[0] = 1; 
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) fly_to_camera[0] = -1; 
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) fly_to_camera[1] = -1;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) fly_to_camera[1] = 1;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) fly_to_camera[2] = -1;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) fly_to_camera[2] = 1;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) fly_to_camera[3] = -1;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) fly_to_camera[3] = 1;
        

        //if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) zoom /= 1.003; 
        //if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) zoom *= 1.003; 

        glUseProgram(Camera_Program);

        glUniform4i(Camera_Go_see, fly_to_camera[0], fly_to_camera[1], fly_to_camera[2], fly_to_camera[3]);
        glUniform2f(Camera_view, div_pos_cam_x/SCR_WIDTH, div_pos_cam_y/SCR_HEIGHT);


        glDispatchCompute(1, 1, 1);
        
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);









        // ОТРИСОВКА (Render Shader) --------------------------------------------------------------------------------------  
        glUseProgram(shaderProgram);


        glUniform2i(render_window, SCR_WIDTH, SCR_HEIGHT);



        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(float), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        



        // 3. Подсчет FPS
        frameCount++;
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 5.0) {
            double fps = frameCount / (currentTime - lastTime);
            
            std::cout << "FPS: " << fps << std::endl;

            frameCount = 0;
            lastTime = currentTime;
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
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