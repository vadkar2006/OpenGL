#pragma once


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    //if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT)) frame_count_go_program++;
    //if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT) && frame_count_go_program >= 0) frame_count_go_program--;
}
class Camera{
    GLFWwindow* window;

    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;

    GLuint VertexFragment_Program;
    GLuint Camera_Program;
    GLuint Camera_ssbo;
    GLuint Camera_look;
    GLuint Camera_navigation;
    GLuint Window_WIDTH_HEIGHT;

    int navigation_buttons[4];
    double mouse_pos[6];

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

public:
    Camera(){
        window = Start_Window();
        VertexFragment_Program = Compilation_VertexFragment_Shader_Program();
        Camera_Program = Compilation_Compute_Shader_Program("Camera_Shader.glsl");

        glEnable(GL_DEPTH_TEST);


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
            
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, Camera_ssbo);




        Camera_navigation = glGetUniformLocation(Camera_Program, "Camera_navigation");
        Camera_look = glGetUniformLocation(Camera_Program, "Camera_look");
        Window_WIDTH_HEIGHT = glGetUniformLocation(VertexFragment_Program, "window");


        for(int i = 0; i < 4; i++)
            navigation_buttons[i] = 0;
        
        glfwGetCursorPos(window, mouse_pos + 4, mouse_pos + 5);

        glfwSetKeyCallback(window, keyCallback);
    }

    void Step_Frame(const Figure_Conveyor& fig_con){
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        for(int i = 0; i < 4; i++)
            navigation_buttons[i] = 0;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) navigation_buttons[0] = 1; 
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) navigation_buttons[0] = -1; 
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) navigation_buttons[1] = -1;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) navigation_buttons[1] = 1;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) navigation_buttons[2] = -1;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) navigation_buttons[2] = 1;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) navigation_buttons[3] = -1;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) navigation_buttons[3] = 1;

        glfwGetCursorPos(window, mouse_pos + 2, mouse_pos + 3);
        mouse_pos[0] = mouse_pos[2] - mouse_pos[4];
        mouse_pos[1] = mouse_pos[5] - mouse_pos[3];
        mouse_pos[4] = mouse_pos[2];
        mouse_pos[5] = mouse_pos[3];




        glUseProgram(Camera_Program);

        glUniform4i(Camera_navigation, this -> Get_navigation_buttons(0), this -> Get_navigation_buttons(1), this -> Get_navigation_buttons(2), this -> Get_navigation_buttons(3));
        glUniform2f(Camera_look, this -> Get_mouse_pos(0)/SCR_WIDTH, this -> Get_mouse_pos(1)/SCR_HEIGHT);

        glDispatchCompute(1, 1, 1);
        
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


        // ОТРИСОВКА (Render Shader) --------------------------------------------------------------------------------------  
        glUseProgram(VertexFragment_Program);

        glUniform2i(Window_WIDTH_HEIGHT, SCR_WIDTH, SCR_HEIGHT);


        glBindVertexArray(fig_con.Get_VAO());
        glDrawElements(GL_TRIANGLES, fig_con.Get_count_triangles_in_mas()*3, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    int Get_navigation_buttons(int i) const{i = (i % 4 + 4) % 4; return navigation_buttons[i];}
    double Get_mouse_pos(int i) const{i = (i % 2 + 2) % 2; return mouse_pos[i];}

    GLFWwindow* Get_Window(){return window;}

    ~Camera(){
        glDeleteBuffers(1, &Camera_ssbo);
        glDeleteProgram(Camera_Program);
        glDeleteProgram(VertexFragment_Program);
        glfwDestroyWindow(window);
    }
};


