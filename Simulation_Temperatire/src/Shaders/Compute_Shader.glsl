#version 460 core

// Размер рабочей группы (16x16 = 256 потоков на группу)
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) buffer HistoryBuffer {
    float history[];
};

// Физические параметры
uniform float u_Alpha;

uniform ivec4 u_count_xyzt;

uniform vec4 u_dxyzt;

uniform int u_current_step;




float Get_History(int x, int y, int z, int t) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history[idx];
}
        
void Set_History(int x, int y, int z, int t, float value) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history[idx] = value;
}




void main() {
    // Получаем координаты текущего потока
    ivec3 pixel = ivec3(gl_GlobalInvocationID.xyz);

    bool flag_X = pixel.x <= 0 || pixel.x >= (u_count_xyzt.x - 1);
    bool flag_Y = pixel.y <= 0 || pixel.y >= (u_count_xyzt.y - 1);
    bool flag_Z = pixel.z <= 0 || pixel.z >= (u_count_xyzt.z - 1);

    if(flag_X || flag_Y || flag_Z) return;
    
    
    // Читаем температуру центра и 4 соседей
    float center = Get_History(pixel.x, pixel.y, pixel.z, 0);
    float left   = Get_History(pixel.x-1, pixel.y, pixel.z, 0);;
    float right  = Get_History(pixel.x+1, pixel.y, pixel.z, 0);;
    float up     = Get_History(pixel.x, pixel.y+1, pixel.z, 0);;
    float down   = Get_History(pixel.x, pixel.y-1, pixel.z, 0);;
    
    // Лапласиан
    float laplacian = (left - 2.0 * center + right) / (u_dxyzt.x * u_dxyzt.x) + (up - 2.0 * center + down) / (u_dxyzt.y * u_dxyzt.y);
    
    // Явный метод Эйлера
    float newTemp = center + u_Alpha * u_dxyzt.w * laplacian;
    
    Set_History(pixel.x, pixel.y, pixel.z, 1, newTemp);
}