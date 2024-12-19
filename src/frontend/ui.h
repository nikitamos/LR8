#pragma once
#include <ncurses.h>
#include <stddef.h>

#define ALIGN_CENTER -1
#define ALIGN_TOP 1
#define ALIGN_LEFT 1
#define ALIGN_RIGHT -2
#define ALIGN_BOTTOM -2
#define ALIGN_100(x) -2 - x
#define _ALPERCENT(p100, a) (int)-(p100 + 2) / 100.0f * a

#define ALIGN(alignment, _X, _Len)                        \
  alignment == ALIGN_CENTER ? _X / 2                      \
  : alignment == ALIGN_RIGHT || alignment == ALIGN_BOTTOM \
      ? _X - _Len                                         \
      : alignment < ALIGN_BOTTOM : _ALPERCENT(alignment, _X) : alignment

#define FG(x) x* COLORS
#define BG(x) x

/** Similar to waddwstr, but alignment is supportted */
void walstr(WINDOW* win, int y, int x, wchar_t* str);
/** Читаетъ строку изъ окна. `prompt` -- запросъ.
 * Возвращаетъ указатель, который необходимо освободить вручную.
 * По завершеніи устанавливаетъ `curs_set(0)` */
wchar_t* readwstr(WINDOW* win, wchar_t* prompt);
/** Аналогична предыдущей функціи, но позволяетъ задать координаты.
 * Поддерживаетъ выравниваніе. */
wchar_t* mvreadwstr(WINDOW* win, int y, int x, wchar_t* prompt);

void terderInitColor();