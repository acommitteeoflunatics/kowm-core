// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _CURSORS_H
#define _CURSORS_H

#include <X11/Xlib.h>

class Cursors {
public:
  static Cursor normalCursor;
  static Cursor resizeSouthCursor;
  static Cursor resizeWestCursor;
  static Cursor resizeEastCursor;

  static bool m_isInitialized;
  static void initializeCursors(Display *display);
};

#endif
