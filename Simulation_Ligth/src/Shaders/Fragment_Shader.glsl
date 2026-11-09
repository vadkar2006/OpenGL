#version 460 core


out vec4 FragColor;
in vec2 TexCoord;

layout(std430, binding = 0) buffer History_Buffer_Pos {
    vec3 history_pos[];
};


uniform int u_current_step;
uniform int u_z_slice;
uniform vec3 u_dxyz;
uniform ivec2 window;
uniform ivec3 u_xyz_count;
uniform vec3 position_zoom;


vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 amplitude_color(float t) {
    t = clamp(t, 0.0, 1.0);

    float hue = mix(0.66, 0.0, t);

    float val = max(t, 0.02);

    return hsv2rgb(vec3(hue, 1.0, val));
}

vec3 background_color() {

    return vec3(0.015, 0.015, 0.020) +
           0.000001 * vec3(float(window.x), float(window.y), u_dxyz.x + u_dxyz.y);
}

void main() {
    vec2 ndc = (TexCoord - vec2(0.5, 0.5)) * 2.0;

    ndc = ndc * position_zoom.z + position_zoom.xy;

    vec2 uv = ndc * 0.5 + 0.5;

    if (u_z_slice < 0 || u_z_slice >= u_xyz_count.z) {
        FragColor = vec4(0.5, 0.0, 0.5, 1.0);
        return;
    }


    float window_aspect = 1.0;
    if (window.x > 0 && window.y > 0) {
        window_aspect = float(window.x) / float(window.y);
    }


    float grid_w = max(float(u_xyz_count.x - 1) * max(u_dxyz.x, 1e-6), 1e-6);
    float grid_h = max(float(u_xyz_count.y - 1) * max(u_dxyz.y, 1e-6), 1e-6);
    float grid_aspect = grid_w / grid_h;

    float aspect = window_aspect / grid_aspect;


    if (aspect > 1.0) {
        uv.y = (uv.y - 0.5) / aspect + 0.5;
    } else {
        uv.x = (uv.x - 0.5) * aspect + 0.5;
    }

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(background_color(), 1.0);
        return;
    }

    vec2 grid_f = uv * vec2(float(u_xyz_count.x - 1), float(u_xyz_count.y - 1));
    ivec2 cell = ivec2(floor(grid_f));

    if (cell.x < 0 || cell.x >= u_xyz_count.x || cell.y < 0 || cell.y >= u_xyz_count.y) {
        FragColor = vec4(background_color(), 1.0);
        return;
    }

    int index = (u_current_step * u_xyz_count.x * u_xyz_count.y * u_xyz_count.z) +
                (u_z_slice * u_xyz_count.x * u_xyz_count.y) +
                (cell.y * u_xyz_count.x) +
                cell.x;

    vec3 p = history_pos[index];

    if (isnan(p.x) || isnan(p.y) || isnan(p.z)) {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }


    float amp = length(p);


    float t = clamp(log2(1.0 + amp * 32.0) / log2(33.0), 0.0, 1.0);

    FragColor = vec4(amplitude_color(t), 1.0);
}
