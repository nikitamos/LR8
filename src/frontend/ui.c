#include "ui.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

void walstr(WINDOW* win, int y, int x, wchar_t* str) {
  int X, Y;
  getmaxyx(win, Y, X);

  if (y == ALIGN_CENTER)
    y = Y / 2;
  else if (y == ALIGN_BOTTOM)
    y = Y - 1;
  else if (y < -2)
    y = _ALPERCENT(y, Y);

  if (x == ALIGN_CENTER)
    x = (X - wcslen(str)) / 2;
  else if (x == ALIGN_RIGHT)
    x = X - 1 - wcslen(str);
  else if (x < 2)
    x = _ALPERCENT(x, X);
  mvwaddwstr(win, y, x, str);
}

wchar_t* readwstr(WINDOW* win, wchar_t* prompt) {
  const size_t DELTA_ALLOC = 8;
  size_t allocated = 0;
  wchar_t* res = NULL;

  addwstr(prompt);
  wrefresh(win);
  echo();
  nocbreak();
  curs_set(1);

  do {
    wchar_t* buf =
        (wchar_t*)realloc(res, (allocated + DELTA_ALLOC) * sizeof(wchar_t));
    if (buf == NULL) break;
    wmemset(buf + allocated, L'\0', DELTA_ALLOC);
    res = buf;

    for (size_t i = allocated; i < allocated + DELTA_ALLOC; i++) {
      get_wch((wint_t*)res + i);
      if (res[i] == L'\n') {
        res[i] = L'\0';
        break;
      }
    }
    allocated += DELTA_ALLOC;
  } while (res[allocated - 1] != L'\0');

  curs_set(0);
  noecho();
  cbreak();
  return res;
}

wchar_t* mvreadwstr(WINDOW* win, int y, int x, wchar_t* prompt) {
  int Y, X;
  getmaxyx(win, Y, X);

  if (y == ALIGN_CENTER)
    y = Y / 2;
  else if (y == ALIGN_BOTTOM)
    y = Y - 1;
  else if (y < -2)
    y = _ALPERCENT(y, Y);

  if (x == ALIGN_CENTER)
    x = (X - wcslen(prompt)) / 2;
  else if (x == ALIGN_RIGHT)
    x = X - 1 - wcslen(prompt);
  else if (x < -2)
    x = _ALPERCENT(x, X);
  wmove(win, y, x);
  return readwstr(win, prompt);
}

void terderInitColor() {
  for (int fg = 0; fg < COLORS; fg++)
    for (int bg = 0; bg < COLORS; bg++)
      init_extended_pair(fg * COLORS + bg, fg, bg);
}