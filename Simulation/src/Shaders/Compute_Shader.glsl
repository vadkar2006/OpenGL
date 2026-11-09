#version 460 core

// Размер рабочей группы (16x16 = 256 потоков на группу)
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;



layout(std430, binding = 1) buffer Buffer_Local_Coordinates {vec3 local_coordinates[];};






void main() {
    
}