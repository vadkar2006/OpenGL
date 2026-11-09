#version 460 core
out vec4 FragColor;
in vec2 TexCoord;

// Привязываем к точке 0, как в вашем C++ коде (binding = 0)
layout(std430, binding = 0) buffer HistoryBuffer {
    float history[];
};




uniform int u_current_step;
uniform int u_z_slice; // Например, 1

uniform vec3 u_dxyz;
uniform ivec2 window;
uniform ivec3 u_xyz_count;

uniform vec3 position_zoom;


void main() {
    vec2 uv = (TexCoord - vec2(0.5, 0.5)) * 2.0;


    // 5. Применяем ЗУМ и СДВИГ
    uv = uv * position_zoom.z + position_zoom.xy;

    // 6. Возвращаем начало координат обратно в левый нижний угол
    uv += vec2(0.5, 0.5);

    uv.x *= float(window.x) / float(window.y);
    int x = int(uv.x * (u_xyz_count.x - 1) / u_dxyz.x * u_dxyz.y);
    int y = int(uv.y * (u_xyz_count.y - 1));
    
    // Формула линейного индекса, аналогичная вашей в C++
    int index = (u_current_step * u_xyz_count.x * u_xyz_count.y * u_xyz_count.z) + 
                (u_z_slice * u_xyz_count.x * u_xyz_count.y) + 
                (y * u_xyz_count.x) + x;
    
    // Защита от выхода за границы массива
    if (x < 0 || x >= u_xyz_count.x || y < 0 || y >= u_xyz_count.y) {
        FragColor = vec4(0.1, 0.1, 0.1, 1.0);
        return;
    }

    float value = history[index];
    
    float normalized = clamp(value, 0.0, 1.0);
    
    // Выводим как оттенок серого (можно заменить на функцию тепловой карты)
    FragColor = vec4(vec3(normalized), 1.0);
}