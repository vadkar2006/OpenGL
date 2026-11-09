#version 460 core

// Размер рабочей группы (16x16 = 256 потоков на группу)
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;



layout(std430, binding = 0) buffer Buffer_Camera {vec3 camera[];};



uniform ivec4 Camera_navigation;


uniform vec2 Camera_look;





void Camera_Gram_Shidt(){
    camera[1] = normalize(camera[1]);
    camera[2] = normalize(camera[2] - dot(camera[1], camera[2]) * camera[1]);
    camera[3] = normalize(camera[3] - dot(camera[1], camera[3]) * camera[1] - dot(camera[2], camera[3]) * camera[2]);
}



void main() {
    camera[1] += camera[3]*Camera_look.x;
    camera[2] += camera[3]*Camera_look.y;

    Camera_Gram_Shidt();

    camera[1] += camera[2] * Camera_navigation.w / 100;

    Camera_Gram_Shidt();

    camera[0] -= camera[3] * Camera_navigation.x / 100;
    camera[0] -= camera[1] * Camera_navigation.y / 100;
    camera[0] -= camera[2] * Camera_navigation.z / 100;
}