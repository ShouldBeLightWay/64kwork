# AGENTS.md — правила проекта для AI-агентов

## Что это

Фреймворк для создания 64k intro (размер итогового бинарника ≤ 65536 байт со сжатым видом).
Только Linux (x86-64). Компилятор: GCC/Clang. Ассемблер: NASM (elf64). Сборка: CMake.

## Железные правила

1. **Никакого libc/CRT в нашем коде.** Компиляция `-ffreestanding`, линковка `-nostdlib`.
   libc подгружается только как транзитивная зависимость libX11/libGL во время выполнения —
   это допустимо, но в нашем коде запрещены любые вызовы libc-символов (printf, malloc, strlen, ...).
   Точка входа — `_start` в `src/asm/entry.asm`, она вызывает
   `int intro_main(int argv, char *argvc)` и возвращает её результат в `SYS_exit`.
2. **Размер — метрика №1.** Любое изменение проверяется размером MinSizeRel-бинарника:
   `stat -c%s build-minsz/intro`. Ожидаемый размер текущей базы: ~5.5 КБ до упаковщика.
   Перед добавлением кода спрашивай: сколько байт это стоит?
3. **Заголовки системных библиотек не include-ить** (только `<stddef.h>`). Все типы — свои
   opaque-определения, все функции — ручные `extern`-прототипы (см. `src/c/intro_main.c`).
4. **Не трогай `link.ld` без понимания последствий** (см. «Ловушки линковки» ниже).

## Сборка и проверка

```sh
cmake -S . -B build-minsz -DCMAKE_BUILD_TYPE=MinSizeRel && cmake --build build-minsz
stat -c%s build-minsz/intro                      # размер
readelf -SW build-minsz/intro                    # секции: нет ли мусора/паддинга
readelf -lW build-minsz/intro                    # сегменты: один PT_LOAD, offset 0
```

Ручная проверка рантайма (нужен X/Wayland-сессия, DISPLAY=:0):

```sh
./build-minsz/intro &                                    # окно 1024x640, фон анимируется
xdotool search --name "64kwork intro"                    # id окна
xdotool windowactivate --sync <id> key Escape            # выход, exit code 0
xdotool windowclose <id>                                 # тоже выход, exit code 0
```

Время жизни процесса в тестах ограничивай явно (`sleep` + `kill` цикл), `wait` без таймаута
допускает зависание. Ожидаемый exit code: 0.

## Структура

```
CMakeLists.txt      флаги freestanding, линковка X11+GL, link.ld
link.ld             intro-раскладка ELF (см. ловушки)
src/asm/entry.asm   _start → intro_main → SYS_exit (rax=60)
src/c/intro_main.c  окно Xlib + контекст GL 4.1 core, кадр-цикл
doc/                документация (architecture, roadmap)
```

## Ловушки линковки (проверено на GCC 16 / binutils / glibc-ld.so)

- Один PT_LOAD c `p_offset=0` обязан покрывать ELF-заголовки, иначе ld.so крэшится при старте.
  В `link.ld` начало `. = 0x400000 + SIZEOF_HEADERS`.
- Уменьшать `max-page-size` (< 4K) нельзя: ядро требует `p_offset ≡ p_vaddr (mod PAGE_SIZE)`,
  иначе SIGSEGV сразу при exec.
- Флаги `-N` / `-n` в новых binutils эквивалентны `--static` — не использовать.
- При `-Wl,-T<script>` линкер не находит транзитивные зависимости libX11/libGL
  (libc, libxcb, libGLX, libGLdispatch) — обязателен `-Wl,-rpath-link,/usr/lib`.
- Стрипка: `-Wl,-s` включена для Release и MinSizeRel; без неё symtab/strtab добавляют ~1.4 КБ.
- Отключены: PIE, stack-protector, unwind-таблицы (`.eh_frame` пуст), `.comment` (`-fno-ident`),
  CFI (`-fcf-protection=none`, иначе Arch-GCC вставляет `endbr64` в каждую функцию).

## Ловушки X11 (проверено)

- `XCheckWindowEvent` НЕ читает сокет — события не приходят при прямом DRI-рендерере.
  Использовать только `while (XPending(dpy)) XNextEvent(dpy, ev)`.
- Выход: `WM_DELETE_WINDOW` (ClientMessage + `WM_PROTOCOLS`), Esc/`q` (KeyPress),
  а также `UnmapNotify`/`DestroyNotify` (закрытие композитором без WM_DELETE, типично для XWayland).
- XEvent — union из 24 longs; события принимаются в буфер `long ev[24]` и читаются
  через ручные структуры (`XAnyEventMin` и т.п.).
- Свой `syscall`-хелпер через inline asm (`sys_write`, `nanosleep`) — libc нет.

## Согласованность кода

- Имена в стиле нижнего регистра (`intro_main`, `glXMakeCurrent` — чужие как в ABI).
- Константы — `#define` со значениями ABI/X/GLX, числа из спецификаций, не выдумывать.
- Комментариев минимум; только там, где нужен контекст ABI/размера.
