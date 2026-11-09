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
    glfwSwapInterval(0);
    
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
    // Атрибут 0: позиция (2 float)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Атрибут 1: текстурные координаты (2 float)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}





class Grid_Solving{
    GLuint ssbo; 
    GLuint VideoCard_Program;

    std::vector<float> start_grid;
    
    int count_x;
    int count_y;
    int count_z;

    int count_t;

    float dx;
    float dy;
    float dz;
    float dt;

    int current_step;

public:
    Grid_Solving(std::vector<float>& start_grid1, int count_x1, int count_y1, int count_z1, int count_t1, float dx1, float dy1, float dz1, float dt1){
        start_grid = start_grid1;
        count_x = count_x1;
        count_y = count_y1;
        count_z = count_z1;

        count_t = count_t1;

        dx = dx1;
        dy = dy1;
        dz = dz1;
        dt = dt1;

        current_step = 0;
    }
    Grid_Solving(int count_x1, int count_y1, int count_z1, int count_t1, float dx1, float dy1, float dz1, float dt1){
        count_x = count_x1;
        count_y = count_y1;
        count_z = count_z1;

        count_t = count_t1;

        dx = dx1;
        dy = dy1;
        dz = dz1;
        dt = dt1;

        current_step = 0;

        start_grid = std::vector<float>(count_x * count_y * count_z * count_t, 0.0);
    }

    const float* operator()(int x = 0, int y = 0, int z = 0, int t = 0) const{
        t = ((current_step + t) % count_t + count_t) % count_t;

        x = (x % count_x + count_x) % count_x;
        y = (y % count_y + count_y) % count_y;
        z = (z % count_z + count_z) % count_z;

        return start_grid.data() + (t * count_x * count_y * count_z) + (z * count_x * count_y) + (y * count_x) + x;
    }
    float* operator()(int x = 0, int y = 0, int z = 0, int t = 0){
        t = ((current_step + t) % count_t + count_t) % count_t;

        x = (x % count_x + count_x) % count_x;
        y = (y % count_y + count_y) % count_y;
        z = (z % count_z + count_z) % count_z;

        return start_grid.data() + (t * count_x * count_y * count_z) + (z * count_x * count_y) + (y * count_x) + x;
    }


    GLuint Get_ssbo(){return ssbo;}
    GLuint Get_VideoCard_Program(){return VideoCard_Program;}

    unsigned int Get_All_Grid_Nodes() const{return count_x * count_y * count_z * count_t;}
    const float* Get_Grid() const {return start_grid.data();} 

    int Get_Count_Nodes_X(){return count_x;}
    int Get_Count_Nodes_Y(){return count_y;}
    int Get_Count_Nodes_Z(){return count_z;}
    int Get_Count_Nodes_T(){return count_t;}

    float Get_dx(){return dx;}
    float Get_dy(){return dy;}
    float Get_dz(){return dz;}
    float Get_dt(){return dt;}


    void Insert_Time_Pos(std::vector<float>& start_grid1){
        if(start_grid.size() / count_t != start_grid1.size())
            return;
        
        for(int k = 0; k < count_z; k++)
            for(int j = 0; j < count_y; j++)
                for(int i = 0; i < count_x; i++)
                    start_grid[current_step*count_x*count_y*count_z + k*count_x*count_y + j*count_x + i] = start_grid1[k*count_x*count_y + j*count_x + i];

    }

    void Compilation_VideoCard_Program(std::string name_file){
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

        VideoCard_Program = glCreateProgram();
        glAttachShader(VideoCard_Program, computeShader);
        glLinkProgram(VideoCard_Program);

        // === 2. ПРОВЕРКА ЛИНКОВКИ ПРОГРАММЫ ===
        int linkSuccess;
        glGetProgramiv(VideoCard_Program, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            glGetProgramInfoLog(VideoCard_Program, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(computeShader);
    }
    void Go_To_VideoCard(){
        glCreateBuffers(1, &ssbo);
        glNamedBufferStorage(ssbo, 
                            (this -> Get_All_Grid_Nodes()) * sizeof(float),
                            nullptr, 
                            GL_DYNAMIC_STORAGE_BIT);
        
        
        glNamedBufferSubData(ssbo, 0, 
                            (this -> Get_All_Grid_Nodes()) * sizeof(float), 
                            this -> Get_Grid());
    }


    ~Grid_Solving() {
        glDeleteBuffers(1, &ssbo);
        glDeleteProgram(VideoCard_Program);
    }
};







float x_pos = 0.0;
float y_pos = 0.0;
float zoom = 1.0;

int frame_count_go_program = 1;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT)) frame_count_go_program++;
    if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT) && frame_count_go_program >= 0) frame_count_go_program--;
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_1 && (action == GLFW_PRESS || action == GLFW_REPEAT)) zoom /= 1.003;
    if (button == GLFW_MOUSE_BUTTON_2 && (action == GLFW_PRESS || action == GLFW_REPEAT)) zoom *= 1.003; 
}




int main(){
    GLFWwindow* window = Start_Window();
    if (!window) return -1;
    
    GLuint shaderProgram = Create_Shader_Program();
    if (shaderProgram == 0) return -1;

    // Данные для полноэкранного квада
    float vertices[] = {
        // positions      // texcoords
        -1.0f,  1.0f,     0.0f, 1.0f, // top left
        -1.0f, -1.0f,     0.0f, 0.0f, // bottom left
         1.0f, -1.0f,     1.0f, 0.0f, // bottom right
         1.0f,  1.0f,     1.0f, 1.0f  // top right
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint VAO, VBO, EBO;
    VAO_VBO_EBO(VAO, VBO, EBO, vertices, indices, sizeof(vertices), sizeof(indices));


    float alfa = 0.2;
    Grid_Solving grid_temp(500, 500, 3, 3, 0.1, 0.1, 0.1, 0.001);

    *(grid_temp(200, 300, 1, 0)) = 5000;




    grid_temp.Compilation_VideoCard_Program("Compute_Shader.glsl");

    grid_temp.Go_To_VideoCard();



    // --- Регистрация колбэков ввода ---
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);





    GLuint u_Alpha = glGetUniformLocation(grid_temp.Get_VideoCard_Program(), "u_Alpha");
    GLuint u_count_xyzt = glGetUniformLocation(grid_temp.Get_VideoCard_Program(), "u_count_xyzt");
    GLuint u_dxyzt = glGetUniformLocation(grid_temp.Get_VideoCard_Program(), "u_dxyzt");
    GLuint u_current_step = glGetUniformLocation(grid_temp.Get_VideoCard_Program(), "u_current_step");





    GLuint render_u_z_slice = glGetUniformLocation(shaderProgram, "u_z_slice");
    GLuint render_u_current_step = glGetUniformLocation(shaderProgram, "u_current_step");

    GLuint render_u_count_xyz = glGetUniformLocation(shaderProgram, "u_xyz_count");
    GLuint render_u_dxyz = glGetUniformLocation(shaderProgram, "u_dxyz");

    GLuint render_window = glGetUniformLocation(shaderProgram, "window");

    GLuint render_position_zoom = glGetUniformLocation(shaderProgram, "position_zoom");


    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, grid_temp.Get_ssbo());
    double lastTime = glfwGetTime();
    int frameCount = 0;
    int total_step = 0;
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) y_pos += 0.001 * zoom;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) y_pos -= 0.001 * zoom;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) x_pos += 0.001 * zoom;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) x_pos -= 0.001 * zoom;

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) zoom /= 1.003; 
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) zoom *= 1.003; 


        glUseProgram(grid_temp.Get_VideoCard_Program());
    
    
        
        
        
        glUniform1f(u_Alpha, alfa);
        glUniform4i(u_count_xyzt, grid_temp.Get_Count_Nodes_X(), grid_temp.Get_Count_Nodes_Y(), grid_temp.Get_Count_Nodes_Z(), grid_temp.Get_Count_Nodes_T());
        glUniform4f(u_dxyzt, grid_temp.Get_dx(), grid_temp.Get_dy(), grid_temp.Get_dz(), grid_temp.Get_dt());
        
        int groupsX = (grid_temp.Get_Count_Nodes_X() + 15) / 16;
        int groupsY = (grid_temp.Get_Count_Nodes_Y() + 15) / 16;
        int groupsZ = (grid_temp.Get_Count_Nodes_Z() + 0) / 1;
        
        for (int i = 0; i < frame_count_go_program; i++) {
            glUniform1i(u_current_step, total_step);
            
            glDispatchCompute(groupsX, groupsY, groupsZ);
            
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            
            total_step = (total_step + 1) % grid_temp.Get_Count_Nodes_T();
        }


        





        // ОТРИСОВКА (Render Shader) --------------------------------------------------------------------------------------  
        glUseProgram(shaderProgram);


        glUniform1i(render_u_z_slice, 1); // Смотрим срез Z=1, где вы поставили 1000
        glUniform1i(render_u_current_step, total_step);
        glUniform2i(render_window, SCR_WIDTH, SCR_HEIGHT);
        glUniform3i(render_u_count_xyz, grid_temp.Get_Count_Nodes_X(), grid_temp.Get_Count_Nodes_Y(), grid_temp.Get_Count_Nodes_Z());

        glUniform3f(render_u_dxyz, grid_temp.Get_dx(), grid_temp.Get_dy(), grid_temp.Get_dz());
        glUniform3f(render_position_zoom, x_pos, y_pos, zoom);


        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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