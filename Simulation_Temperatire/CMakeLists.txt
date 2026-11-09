cmake_minimum_required(VERSION 3.10)
project(Simulation)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ====== 1. Подключаем GLFW ======
add_subdirectory(external/glfw)

# ====== 2. Создаём исполняемый файл ======
add_executable(${PROJECT_NAME} src/main.cpp)

# ====== 3. Добавляем GLAD как статическую библиотеку ======
add_library(glad STATIC external/glad/src/glad.c)

# ✅ ВАЖНО: Добавляем путь к заголовкам для самой библиотеки GLAD
target_include_directories(glad PRIVATE
    external/glad/include
)

# ====== 4. Добавляем пути к заголовкам для основного проекта ======
target_include_directories(${PROJECT_NAME} PRIVATE
    external/glad/include
)

# ====== 5. Линкуем библиотеки ======
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
)

# ====== 6. Для Windows (MinGW) подключаем OpenGL ======
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32
    )
endif()