// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _ATOMS_H
#define _ATOMS_H

#include <X11/Xlib.h>

class Atoms {
public:
  static Atom mwmHints;
  static Atom wmProtocols;
  static Atom wmDelete;
  static Atom wmState;
  static Atom wmTakeFocus;
  static Atom netWmDesktop;

  static bool m_isInitialized;
  static void initializeAtoms(Display *display);
};

#endif
