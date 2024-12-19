#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

typedef struct Menudef {
  wchar_t* title;
  /** Height of inside space of the menu */
  int height;
  /** Amount of characters in the longest string or any number greater or equal
   * it */
  int width;
  wchar_t** items;
  /** Length of an `items` array */
  int len;

} Menudef;

/**
 * `x` and `y` are coordinates relatively to `stdscr`. `ALIGN_*` macros are
 * accepted
 *
 * Returns the index of selected option.
 * -1 if selection is cancelled
 * -2 if window creation failed
 * UB: wrong or incorrect data
 */
int showMenu(const Menudef* def, int y, int x, bool cancellable);
