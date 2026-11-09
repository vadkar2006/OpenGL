#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import shutil
import sys
from datetime import datetime
from pathlib import Path

BACKUP_PREFIX = "backup_fix_"

FRAG_V4_MARKER = "// PATCH v4: full screen cover aspect + amplitude colormap"
VIEWPORT_V4_MARKER = "PATCH v4: full viewport"
INIT_V4_MARKER = "PATCH v4: circular initial amplitude"

FULL_VIEWPORT_LINE = "    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // PATCH v4: full viewport\n"

NEW_FRAGMENT_SHADER = """#version 460 core
// PATCH v4: full screen cover aspect + amplitude colormap

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

    // blue -> cyan -> green -> yellow -> red
    float hue = mix(0.66, 0.0, t);

    // Минимальная яркость, чтобы ноль не был полностью чёрным.
    float val = max(t, 0.02);

    return hsv2rgb(vec3(hue, 1.0, val));
}

vec3 background_color() {
    // Чтобы window и u_dxyz не были вырезаны оптимизатором.
    return vec3(0.015, 0.015, 0.020) +
           0.000001 * vec3(float(window.x), float(window.y), u_dxyz.x + u_dxyz.y);
}

void main() {
    vec2 ndc = (TexCoord - vec2(0.5, 0.5)) * 2.0;

    // Zoom and pan
    ndc = ndc * position_zoom.z + position_zoom.xy;

    // Convert back to [0..1]
    vec2 uv = ndc * 0.5 + 0.5;

    if (u_z_slice < 0 || u_z_slice >= u_xyz_count.z) {
        FragColor = vec4(0.5, 0.0, 0.5, 1.0);
        return;
    }

    // Aspect ratio окна.
    float window_aspect = 1.0;
    if (window.x > 0 && window.y > 0) {
        window_aspect = float(window.x) / float(window.y);
    }

    // Физический размер сетки по X/Y.
    float grid_w = max(float(u_xyz_count.x - 1) * max(u_dxyz.x, 1e-6), 1e-6);
    float grid_h = max(float(u_xyz_count.y - 1) * max(u_dxyz.y, 1e-6), 1e-6);
    float grid_aspect = grid_w / grid_h;

    float aspect = window_aspect / grid_aspect;

    // Fullscreen cover: без чёрных полос, но с возможной обрезкой.
    // Если экран шире сетки — обрезаем верх/низ.
    // Если экран уже сетки — обрезаем слева/справа.
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

    // Амплитуда = длина вектора смещения.
    float amp = length(p);

    // Если слишком темно/слишком ярко, меняй 32.0.
    float t = clamp(log2(1.0 + amp * 32.0) / log2(33.0), 0.0, 1.0);

    FragColor = vec4(amplitude_color(t), 1.0);
}
"""

CIRCULAR_INIT = """// PATCH v4: circular initial amplitude
    {
        int cx = 250;
        int cy = 250;
        int cz = 1;
        int radius = 18;

        for (int init_y = cy - radius; init_y <= cy + radius; ++init_y) {
            for (int init_x = cx - radius; init_x <= cx + radius; ++init_x) {
                float dx = float(init_x - cx) / float(radius);
                float dy = float(init_y - cy) / float(radius);
                float r2 = dx * dx + dy * dy;

                if (r2 <= 1.0f) {
                    float value = (float)std::exp(-3.0f * r2);
                    grid_temp.Get_Grid(0)[grid_temp(true, init_x, init_y, cz, 0)] = value;
                }
            }
        }
    }"""


def find_root():
    cwd = Path.cwd().resolve()

    candidates = [cwd, *cwd.parents]
    for base in candidates:
        if (base / "main.cpp").is_file() and (base / "Shaders" / "Fragment_Shader.glsl").is_file():
            return base

    for main_cpp in cwd.rglob("main.cpp"):
        if any(part.startswith(BACKUP_PREFIX) for part in main_cpp.parts):
            continue

        base = main_cpp.parent
        if (base / "Shaders" / "Fragment_Shader.glsl").is_file():
            return base

    return None


def read_text(path):
    return path.read_bytes().decode("utf-8-sig")


def write_text(path, text):
    with path.open("w", encoding="utf-8", newline="") as f:
        f.write(text)


def make_backup(path, root, backup_dir, backed_up):
    if path in backed_up:
        return

    rel = path.relative_to(root)
    dst = backup_dir / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, dst)
    backed_up.add(path)


def replace_square_viewport_block(text):
    marker = "PATCH v3: square viewport"
    idx = text.find(marker)

    if idx == -1:
        return text, False

    line_start = text.rfind("\n", 0, idx)
    line_start = 0 if line_start == -1 else line_start + 1

    brace_start = text.find("{", idx)
    if brace_start == -1:
        return text, False

    depth = 0
    pos = brace_start

    while pos < len(text):
        ch = text[pos]

        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                end = pos + 1

                if end < len(text) and text[end] == "\r":
                    end += 1
                if end < len(text) and text[end] == "\n":
                    end += 1

                return text[:line_start] + FULL_VIEWPORT_LINE + text[end:], True

        pos += 1

    return text, False


def ensure_full_viewport(text):
    changed = False

    # Если вдруг остался вызов квадратного viewport.
    square_call = re.compile(r"glViewport\(vp_x,\s*vp_y,\s*side,\s*side\);")
    if square_call.search(text):
        text = square_call.sub(
            "glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // " + VIEWPORT_V4_MARKER,
            text
        )
        changed = True

    # Если полного viewport нет вообще, добавляем перед рендером.
    if not re.search(r"glViewport\(0,\s*0,\s*SCR_WIDTH,\s*SCR_HEIGHT\)", text):
        use_pat = re.compile(r"^([ \t]*)glUseProgram\(shaderProgram\);", re.M)

        def use_repl(m):
            indent = m.group(1)
            return (
                indent + "glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // " + VIEWPORT_V4_MARKER + "\n" +
                indent + "glClear(GL_COLOR_BUFFER_BIT);\n" +
                m.group(0)
            )

        new_text = use_pat.sub(use_repl, text, count=1)
        if new_text != text:
            text = new_text
            changed = True

    # Если нет очистки экрана, добавляем после viewport.
    if not re.search(r"glClear\(GL_COLOR_BUFFER_BIT\)", text):
        vp_pat = re.compile(r"^([ \t]*)glViewport\(0,\s*0,\s*SCR_WIDTH,\s*SCR_HEIGHT\);.*$", re.M)

        if vp_pat.search(text):
            def vp_repl(m):
                full = m.group(0)
                indent = re.match(r"[ \t]*", full).group(0)
                return full + "\n" + indent + "glClear(GL_COLOR_BUFFER_BIT);"

            new_text = vp_pat.sub(vp_repl, text, count=1)
            if new_text != text:
                text = new_text
                changed = True

    return text, changed


def patch_initial_condition(text):
    if INIT_V4_MARKER in text:
        return text, False

    square_init_re = re.compile(
        r"for \(int init_y = 245; init_y <= 254; \+\+init_y\) \{.*?"
        r"grid_temp\.Get_Grid\(0\)\[grid_temp\(true, init_x, init_y, 1, 0\)\] = 1\.0f;.*?"
        r"\}\s*\}",
        re.S
    )

    if square_init_re.search(text):
        return square_init_re.sub(lambda _: CIRCULAR_INIT, text, count=1), True

    original_init_re = re.compile(
        r"grid_temp\.Get_Grid\(0\)\[grid_temp\(true,\s*250,\s*250,\s*1,\s*0\)\]\s*=\s*0\.1f?;"
    )

    if original_init_re.search(text):
        return original_init_re.sub(lambda _: CIRCULAR_INIT, text, count=1), True

    return text, False


def patch_fragment_shader(path):
    text = read_text(path)

    if FRAG_V4_MARKER in text:
        return False

    write_text(path, NEW_FRAGMENT_SHADER)
    return True


def patch_main_cpp(path):
    text = read_text(path)
    changed = False

    new_text, replaced_square = replace_square_viewport_block(text)
    if replaced_square:
        text = new_text
        changed = True

    new_text, viewport_changed = ensure_full_viewport(text)
    if viewport_changed:
        text = new_text
        changed = True

    new_text, init_changed = patch_initial_condition(text)
    if init_changed:
        text = new_text
        changed = True

    if changed:
        write_text(path, text)

    return changed


def main():
    root = find_root()
    if not root:
        print("Не найдена папка проекта с файлами main.cpp и Shaders/Fragment_Shader.glsl")
        sys.exit(1)

    print(f"Корень проекта: {root}")

    backup_dir = root / (BACKUP_PREFIX + datetime.now().strftime("%Y-%m-%d_%H-%M-%S"))
    backup_dir.mkdir(parents=True, exist_ok=True)
    backed_up = set()

    main_cpp = root / "main.cpp"
    frag = root / "Shaders" / "Fragment_Shader.glsl"

    make_backup(main_cpp, root, backup_dir, backed_up)
    make_backup(frag, root, backup_dir, backed_up)

    frag_changed = patch_fragment_shader(frag)
    main_changed = patch_main_cpp(main_cpp)

    print(f"Резервные копии: {backup_dir}")
    print(f"Fragment Shader обновлён: {frag_changed}")
    print(f"main.cpp обновлён: {main_changed}")
    print("Готово. Теперь пересобери проект.")


if __name__ == "__main__":
    main()