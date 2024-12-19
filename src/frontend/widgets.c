#include "widgets.h"

#include <ncurses.h>
#include <string.h>

#include "ui.h"
void terderInitColor() {
  for (int fg = 0; fg < COLORS; fg++)
    for (int bg = 0; bg < COLORS; bg++)
      init_extended_pair(fg * COLORS + bg, fg, bg);
}
int showMenu(const Menudef* def, int y, int x, bool cancellable) {
  int X, Y;
  getmaxyx(stdscr, Y, X);
  if (y == ALIGN_CENTER)
    y = (Y - def->height) / 2 - 1;
  else if (y == ALIGN_BOTTOM)
    y = Y - def->height - 2;
  else if (y < -2)
    y = (int)-(y + 2) / 100.0f * Y;

  if (x == ALIGN_CENTER)
    x = (X - def->width) / 2 - 1;
  else if (x == ALIGN_RIGHT)
    x = X - def->width - 2;
  else if (x < -2)
    x = (int)-(x + 2) / 100.0f * X;

  WINDOW* win = newwin(def->height + 2, def->width + 2, y, x);
  if (!win) return -2;
  curs_set(0);
  keypad(win, true);
  wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
  int ENTRY_COLOR = FG(COLOR_GREEN) + BG(COLOR_BLUE);
  int HEAD_COLOR = FG(COLOR_MAGENTA);

  if (def->title) {
    int titlebeg = (def->width - wcslen(def->title)) / 2 + 1;
    mvwaddwstr(win, 0, titlebeg, def->title);
    mvwchgat(win, 0, titlebeg, wcslen(def->title), A_BOLD, HEAD_COLOR, NULL);
  }

  for (int i = 0; i < def->len; i++) mvwaddwstr(win, i + 1, 1, def->items[i]);
  mvwchgat(win, 1, 1, def->width, A_BOLD, ENTRY_COLOR, NULL);
  wrefresh(win);

  bool runc = true;
  int selected = 0;
  while (runc) {
    chtype a = wgetch(win);
    switch (a) {
      case 'q':
        if (!cancellable) break;
        selected = -1;
      case '\n':
      case KEY_RIGHT:
        runc = false;
        break;
      case 'j':
      case KEY_DOWN:
        selected++;
        selected %= def->len;
        wchgat(win, def->width, A_NORMAL, 0, NULL);
        mvwchgat(win, selected + 1, 1, def->width, A_BOLD, ENTRY_COLOR, NULL);
        wrefresh(win);
        break;
      case 'k':
      case KEY_UP:
        if (selected == 0)
          selected = def->len - 1;
        else
          selected--;
        wchgat(win, def->width, A_NORMAL, 0, NULL);
        mvwchgat(win, selected + 1, 1, def->width, A_BOLD, ENTRY_COLOR, NULL);
        wrefresh(win);
        break;
    }
  }
  werase(win);
  wrefresh(win);
  delwin(win);
  curs_set(1);
  return selected;
}
