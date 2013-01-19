// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "Atoms.h"

Atom Atoms::mwmHints = 0;
Atom Atoms::wmProtocols = 0;
Atom Atoms::wmDelete = 0;
Atom Atoms::wmState = 0;
Atom Atoms::wmTakeFocus = 0;
Atom Atoms::netWmDesktop = 0;

bool Atoms::m_isInitialized = false;

void Atoms::initializeAtoms(Display *display) {
  if (m_isInitialized)
    return;

  m_isInitialized = true;

  mwmHints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  wmProtocols = XInternAtom(display, "WM_PROTOCOLS", False);
  wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
  wmState = XInternAtom(display, "WM_STATE", False);
  wmTakeFocus = XInternAtom(display, "WM_TAKE_FOCUS", False);
  netWmDesktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
}
