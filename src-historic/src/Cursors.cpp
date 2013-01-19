// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "Cursors.h"

#include <X11/cursorfont.h>

Cursor Cursors::normalCursor = 0;
Cursor Cursors::resizeSouthCursor = 0;
Cursor Cursors::resizeWestCursor = 0;
Cursor Cursors::resizeEastCursor = 0;

bool Cursors::m_isInitialized = false;

void Cursors::initializeCursors(Display *display) {
  if (m_isInitialized)
    return;

  m_isInitialized = true;

  normalCursor = XCreateFontCursor(display, XC_left_ptr);
  resizeSouthCursor = XCreateFontCursor(display, XC_bottom_side);
  resizeWestCursor = XCreateFontCursor(display, XC_left_side);
  resizeEastCursor = XCreateFontCursor(display, XC_right_side);
}
