#version 460 core

// Размер рабочей группы (16x16 = 256 потоков на группу)
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;


layout(std430, binding = 0) buffer History_Buffer_Pos {vec3 history_pos[];};
layout(std430, binding = 1) buffer History_Buffer_Speed {vec3 history_speed[];};
layout(std430, binding = 2) buffer History_Buffer_Acceleration {vec3 history_acceleration[];};

layout(std430, binding = 3) buffer History_Buffer_Mass {float history_mass[];};
layout(std430, binding = 4) buffer History_Buffer_Stiffness {float history_stiffness[];};


// Физические параметры

uniform ivec4 u_count_xyzt;

uniform vec2 u_dlt;

uniform int u_current_step;




vec3 Get_History_Pos(int x, int y, int z, int t){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history_pos[idx];
}
vec3 Get_History_Speed(int x, int y, int z, int t){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history_speed[idx];
}
vec3 Get_History_Acceleration(int x, int y, int z, int t){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history_acceleration[idx];
}

float Get_History_Mass(int x, int y, int z, int t) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history_mass[idx];
}
float Get_History_Stiffness(int x, int y, int z, int t) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    return history_stiffness[idx];
}




void Set_History_Pos(int x, int y, int z, int t, vec3 value){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history_pos[idx] = value;
}
void Set_History_Speed(int x, int y, int z, int t, vec3 value){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history_speed[idx] = value;
}
void Set_History_Acceleration(int x, int y, int z, int t, vec3 value){
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history_acceleration[idx] = value;
}
void Set_History_Mass(int x, int y, int z, int t, float value) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history_mass[idx] = value;
}
void Set_History_Stiffness(int x, int y, int z, int t, float value) {
    int absolute_step = ((u_current_step + t) % u_count_xyzt.w + u_count_xyzt.w) % u_count_xyzt.w;

    int idx = (absolute_step * u_count_xyzt.x *u_count_xyzt.y *u_count_xyzt.z) + (z * u_count_xyzt.x *u_count_xyzt.y) + (y * u_count_xyzt.x) + x;
    history_stiffness[idx] = value;
}





// PATCH: safe spring normalize
vec3 Safe_Spring_Deformation(vec3 d, float rest_length) {
    float len = length(d);
    if (len < 1e-8) return vec3(0.0);
    return d - (d / len) * rest_length;
}

void main() {
    // Получаем координаты текущего потока
    ivec3 pixel = ivec3(gl_GlobalInvocationID.xyz);

    bool flag_X = pixel.x <= 0 || pixel.x >= (u_count_xyzt.x - 1);
    bool flag_Y = pixel.y <= 0 || pixel.y >= (u_count_xyzt.y - 1);
    bool flag_Z = pixel.z <= 0 || pixel.z >= (u_count_xyzt.z - 1);

    if(flag_X || flag_Y || flag_Z) return;
    


    vec3 pos_center = Get_History_Pos(pixel.x, pixel.y, pixel.z, 0);
    float k_center = Get_History_Stiffness(pixel.x, pixel.y, pixel.z, 0);
    float m_center = Get_History_Mass(pixel.x, pixel.y, pixel.z, 0);

    //векторы до грузов от центрального
    vec3 pos_x_plus = Get_History_Pos(pixel.x + 1, pixel.y, pixel.z, 0) + vec3(1.0, 0.0, 0.0)*u_dlt.x - pos_center;
    vec3 pos_x_minus = Get_History_Pos(pixel.x - 1, pixel.y, pixel.z, 0) - vec3(1.0, 0.0, 0.0)*u_dlt.x - pos_center;
    vec3 pos_y_plus = Get_History_Pos(pixel.x, pixel.y + 1, pixel.z, 0) + vec3(0.0, 1.0, 0.0)*u_dlt.x - pos_center;
    vec3 pos_y_minus = Get_History_Pos(pixel.x, pixel.y - 1, pixel.z, 0) - vec3(0.0, 1.0, 0.0)*u_dlt.x - pos_center;
    vec3 pos_z_plus = Get_History_Pos(pixel.x, pixel.y, pixel.z + 1, 0) + vec3(0.0, 0.0, 1.0)*u_dlt.x - pos_center;
    vec3 pos_z_minus = Get_History_Pos(pixel.x, pixel.y, pixel.z - 1, 0) - vec3(0.0, 0.0, 1.0)*u_dlt.x - pos_center;

    pos_x_plus = Safe_Spring_Deformation(pos_x_plus, u_dlt.x);
    pos_x_minus = Safe_Spring_Deformation(pos_x_minus, u_dlt.x);
    pos_y_plus = Safe_Spring_Deformation(pos_y_plus, u_dlt.x);
    pos_y_minus = Safe_Spring_Deformation(pos_y_minus, u_dlt.x);
    pos_z_plus = Safe_Spring_Deformation(pos_z_plus, u_dlt.x);
    pos_z_minus = Safe_Spring_Deformation(pos_z_minus, u_dlt.x);
    
    //жесткости
    float k_x_plus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x + 1, pixel.y, pixel.z, 0)); 
    float k_x_minus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x - 1, pixel.y, pixel.z, 0)); 
    float k_y_plus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x, pixel.y + 1, pixel.z, 0)); 
    float k_y_minus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x, pixel.y - 1, pixel.z, 0)); 
    float k_z_plus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x, pixel.y, pixel.z + 1, 0)); 
    float k_z_minus = 1 / (1 / k_center  +  1 / Get_History_Stiffness(pixel.x, pixel.y, pixel.z - 1, 0));

    //итоговое ускорение
    vec3 a_center = (pos_x_plus*k_x_plus + pos_x_minus*k_x_minus + pos_y_plus*k_y_plus + pos_y_minus*k_y_minus + pos_z_plus*k_z_plus + pos_z_minus*k_z_minus) / m_center;

    vec3 v_center = Get_History_Speed(pixel.x, pixel.y, pixel.z, 0) + a_center*u_dlt.y;
    vec3 x_center = pos_center + v_center*u_dlt.y;


    
    Set_History_Pos(pixel.x, pixel.y, pixel.z, 1, x_center);
    Set_History_Speed(pixel.x, pixel.y, pixel.z, 1, v_center);
    Set_History_Acceleration(pixel.x, pixel.y, pixel.z, 1, a_center);
}