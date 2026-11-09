#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 Color;


layout(std430, binding = 0) buffer Buffer_Camera {vec3 history[];};

uniform ivec2 window;




void main() {
    mat3 A = mat3(history[1], history[2], history[3]);


    vec3 Pos_In_Camera = transpose(A) * (aPos - history[0]);


    float aspect = 1.0;

    mat4 projection = mat4(
        2.4142 / aspect,  0.0,      0.0,       0.0,    // Столбец 0
        0.0,                2.4142,   0.0,       0.0,    // Столбец 1
        0.0,                0.0,     -1.002,    -1.0,    // Столбец 2
        0.0,                0.0,     -0.2002,    0.0     // Столбец 3
    );

    float a = 1.0 * window.y / window.x;
    gl_Position = projection * vec4(Pos_In_Camera.x * a, Pos_In_Camera.yz, 1.0);
    Color = aColor;
}
