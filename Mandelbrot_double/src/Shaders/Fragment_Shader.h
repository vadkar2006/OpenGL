const char* fragmentShaderSource = R"(
    #version 460 core
    #extension GL_NV_gpu_shader5 : enable

    out vec4 FragColor;
    
    in vec2 TexCoord;
    

    uniform vec2  uResolution;   // Размер экрана
    uniform vec2  uOffset;       // Смещение (движение)
    uniform float uZoom;         // Зум
    uniform int   uMaxIter;      // Максимум итераций
    uniform float uColorCycles;  // Количество цветовых циклов
    

    double mandelbrot(double c_x, double c_y, int maxIter) {
        double z_x = 0.0; 
        double z_y = 0.0;
        double zMod = 0.0;
        
        for (int i = 0; i < 10000; i++) {
            if (i > maxIter) break;
            
            // z = z² + c

            double z_x_new = z_x*z_x - z_y*z_y + c_x;
            double z_y_new = 2.0*z_x*z_y + c_y;
            
            
            z_x = z_x_new;
            z_y = z_y_new;

            zMod = z_x*z_x + z_y*z_y;
            
            if (zMod > 4.0) {
                // Сглаживание
                double smooth_val = double(i) + 1.0 - log2(log2(float(sqrt(zMod))));
                return smooth_val / double(maxIter);
            }
        }
        return 0.0;  // Внутри фрактала
    }
    
    
    vec3 spiralPalette(double t, float cycles) {
        float colorT = float(t * cycles);
        colorT = colorT - floor(colorT);  // 0 → 1
        
        
        float R = sin(colorT * 6.28318 + 0.0) * 0.5 + 0.5;
        float G = sin(colorT * 6.28318 + 2.094) * 0.5 + 0.5;
        float B = sin(colorT * 6.28318 + 4.188) * 0.5 + 0.5;
        
        
        float stripes = sin(float(t * 100.0)) * 0.05;
        R = clamp(R + stripes, 0.0, 1.0);
        G = clamp(G + stripes * 0.7, 0.0, 1.0);
        B = clamp(B + stripes * 0.3, 0.0, 1.0);
        
        
        float glow = exp(float(-t * 30.0)) * 0.3;
        R = clamp(R + glow, 0.0, 1.0);
        G = clamp(G + glow * 0.8, 0.0, 1.0);
        B = clamp(B + glow * 0.5, 0.0, 1.0);
        
        
        float dark = float(1.0 - t * t * 0.5);
        R = R * dark;
        G = G * dark;
        B = B * dark;
        
        return vec3(R, G, B);
    }
    
    void main() {
        vec2 uv = TexCoord;
        
        
        vec2 c = (uv - 0.5) * 2.0;
        c.x *= uResolution.x / uResolution.y;
        
        double c_x = c.x;
        double c_y = c.y;

        c_x = c_x / uZoom + uOffset.x;
        c_y = c_y / uZoom + uOffset.y;
        

        double t = mandelbrot(c_x, c_y, uMaxIter);
        

        vec3 color = spiralPalette(t, uColorCycles);
        FragColor = vec4(color, 1.0);
        
    }
)";