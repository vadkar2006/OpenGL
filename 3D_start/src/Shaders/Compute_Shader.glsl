#version 460 core

// Размер рабочей группы (16x16 = 256 потоков на группу)
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;



layout(std430, binding = 0) buffer Buffer_Camera {vec3 history[];};



uniform ivec4 Go_see;


uniform vec2 Camera_view;





void Camera_Gram_Shidt(){
    history[1] = normalize(history[1]);
    history[2] = normalize(history[2] - dot(history[1], history[2]) * history[1]);
    history[3] = normalize(history[3] - dot(history[1], history[3]) * history[1] - dot(history[2], history[3]) * history[2]);
}



void main() {
    history[1] += history[3]*Camera_view.x;
    history[2] += history[3]*Camera_view.y;

    //Camera_Gram_Shidt();

    history[1] += history[2] * Go_see.w / 100;

    Camera_Gram_Shidt();

    history[0] -= history[3] * Go_see.x / 100;
    history[0] -= history[1] * Go_see.y / 100;
    history[0] -= history[2] * Go_see.z / 100;
}