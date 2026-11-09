#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pack_code.py

Собирает исходный код проекта из подкаталогов в один TXT-файл.

По умолчанию собирает файлы:
    .cpp .h .glsl

Примеры:
    python pack_code.py .
    python pack_code.py . -o project_code.txt
    python pack_code.py src -o project_code.txt
    python pack_code.py . --extra-exclude external
    python pack_code.py . -e .cpp .h .c .glsl
"""

from __future__ import annotations

import argparse
import sys
from datetime import datetime
from pathlib import Path


DEFAULT_EXTENSIONS = {
    ".cpp",
    ".h",
    ".glsl",
}

# Каталоги, которые обычно не нужны в дампе кода.
# Внешние библиотеки сюда специально не включены по умолчанию,
# чтобы можно было решить, нужны ли external/, glfw/, glad/ и т.п.
DEFAULT_EXCLUDE_DIRS = {
    ".git",
    ".hg",
    ".svn",
    "build",
    "cmake-build-debug",
    "cmake-build-release",
    "out",
    "bin",
    "obj",
    ".vs",
    ".idea",
    ".cache",
    "__pycache__",
}


def normalize_extensions(values: list[str]) -> set[str]:
    """
    Приводит расширения к виду .cpp, .h, .glsl и т.д.
    """
    result = set()

    for value in values:
        value = value.strip().lower()
        if not value:
            continue

        if not value.startswith("."):
            value = "." + value

        result.add(value)

    return result


def iter_source_files(
    root: Path,
    extensions: set[str],
    exclude_dirs: set[str],
):
    """
    Рекурсивно возвращает файлы из root, пропуская исключённые каталоги.
    """
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue

        if path.suffix.lower() not in extensions:
            continue

        rel = path.relative_to(root)

        # Проверяем только каталоги, имя самого файла не проверяем.
        if any(part in exclude_dirs for part in rel.parts[:-1]):
            continue

        yield path


def make_short_labels(file_paths: list[Path], root: Path) -> list[dict]:
    """
    Делает короткие уникальные подписи для файлов.

    Например:
        src/main.cpp -> main.cpp
        src/render/shader.cpp -> render/shader.cpp
        src/ui/shader.cpp -> ui/shader.cpp

    Если имена совпадают, скрипт добавляет нужную часть пути.
    """
    entries = []

    for path in file_paths:
        rel = path.relative_to(root)
        parts = rel.parts

        entries.append(
            {
                "path": path,
                "rel": rel,
                "parts": parts,
                "level": 0,
                "label": parts[-1],
            }
        )

    # Расширяем короткие имена, пока есть совпадения.
    while True:
        counts = {}

        for entry in entries:
            counts[entry["label"]] = counts.get(entry["label"], 0) + 1

        duplicates = {
            label
            for label, count in counts.items()
            if count > 1
        }

        if not duplicates:
            break

        expanded = False

        for entry in entries:
            if entry["label"] in duplicates:
                max_level = len(entry["parts"]) - 1

                if entry["level"] < max_level:
                    entry["level"] += 1
                    take = entry["level"] + 1
                    entry["label"] = "/".join(entry["parts"][-take:])
                    expanded = True

        # Если расширять больше нечего, делаем уникальность номером.
        if not expanded:
            counters = {}

            for entry in entries:
                base = entry["label"]

                if counts.get(base, 0) > 1:
                    counters[base] = counters.get(base, 0) + 1
                    entry["label"] = f"{base} ({counters[base]})"

            break

    entries.sort(key=lambda entry: entry["rel"].as_posix())
    return entries


def read_text_smart(path: Path) -> tuple[str, str]:
    """
    Читает текст файла, пробуя несколько кодировок.
    Полезно, если встречаются русские комментарии в cp1251.
    """
    raw = path.read_bytes()

    for encoding in ("utf-8-sig", "utf-8", "cp1251", "latin-1"):
        try:
            return raw.decode(encoding), encoding
        except UnicodeDecodeError:
            continue

    return raw.decode("utf-8", errors="replace"), "utf-8-replace"


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Собирает исходники проекта в один TXT-файл. "
            "По умолчанию: .cpp, .h, .glsl."
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )

    parser.add_argument(
        "root",
        nargs="?",
        default=Path("."),
        type=Path,
        help="Корневая папка проекта. По умолчанию текущая папка.",
    )

    parser.add_argument(
        "-o",
        "--output",
        default=Path("project_code.txt"),
        type=Path,
        help="Имя выходного TXT-файла.",
    )

    parser.add_argument(
        "-e",
        "--extensions",
        nargs="*",
        default=sorted(DEFAULT_EXTENSIONS),
        help=(
            "Расширения файлов.\n"
            "Пример: -e .cpp .h .hpp .c .glsl"
        ),
    )

    parser.add_argument(
        "--exclude",
        nargs="*",
        default=None,
        help=(
            "Полностью заменить список исключаемых каталогов.\n"
            "Пример: --exclude build external .git"
        ),
    )

    parser.add_argument(
        "--extra-exclude",
        nargs="*",
        default=[],
        help=(
            "Добавить каталоги к стандартным исключениям.\n"
            "Пример: --extra-exclude external glfw"
        ),
    )

    parser.add_argument(
        "--no-exclude",
        action="store_true",
        help="Не использовать стандартные исключения каталогов.",
    )

    args = parser.parse_args()

    root = args.root.resolve()

    if not root.exists():
        print(f"Ошибка: путь не найден: {root}")
        return 1

    if not root.is_dir():
        print(f"Ошибка: путь не является папкой: {root}")
        return 1

    extensions = normalize_extensions(args.extensions)

    if args.no_exclude:
        exclude_dirs = set()
    elif args.exclude is not None:
        exclude_dirs = {name.strip() for name in args.exclude if name.strip()}
    else:
        exclude_dirs = set(DEFAULT_EXCLUDE_DIRS)

    exclude_dirs.update(
        name.strip()
        for name in args.extra_exclude
        if name.strip()
    )

    files = list(
        iter_source_files(
            root=root,
            extensions=extensions,
            exclude_dirs=exclude_dirs,
        )
    )

    if not files:
        print(
            "Файлы не найдены.\n"
            f"Root: {root}\n"
            f"Extensions: {', '.join(sorted(extensions))}\n"
            f"Exclude dirs: {', '.join(sorted(exclude_dirs)) if exclude_dirs else 'нет'}"
        )
        return 0

    entries = make_short_labels(files, root)

    output_path = args.output
    if output_path.parent and not output_path.parent.exists():
        output_path.parent.mkdir(parents=True, exist_ok=True)

    line = "=" * 100
    small_line = "-" * 100

    with output_path.open("w", encoding="utf-8", newline="\n") as out:
        out.write(f"{line}\n")
        out.write("PROJECT SOURCE EXPORT\n")
        out.write(f"{line}\n")
        out.write(f"Root       : {root}\n")
        out.write(f"Generated  : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        out.write(f"Files      : {len(entries)}\n")
        out.write(f"Extensions : {', '.join(sorted(extensions))}\n")

        if exclude_dirs:
            out.write(f"Excluded   : {', '.join(sorted(exclude_dirs))}\n")
        else:
            out.write("Excluded   : нет\n")

        out.write("\n")
        out.write("Список файлов:\n")
        out.write(f"{small_line}\n")

        for index, entry in enumerate(entries, start=1):
            label = entry["label"]
            rel_path = entry["rel"].as_posix()
            out.write(f"{index:4}. {label:<40} -> {rel_path}\n")

        for index, entry in enumerate(entries, start=1):
            text, encoding = read_text_smart(entry["path"])

            out.write("\n")
            out.write(f"{line}\n")
            out.write(f"[{index}/{len(entries)}] {entry['label']}\n")
            out.write(f"Path: {entry['rel'].as_posix()}\n")
            out.write(f"Encoding: {encoding}\n")
            out.write(f"{line}\n\n")

            out.write(text.rstrip("\n"))
            out.write("\n")

    print(f"Готово: {len(entries)} файлов упаковано -> {output_path.resolve()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())