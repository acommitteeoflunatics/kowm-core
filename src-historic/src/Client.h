// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _CLIENT_H
#define _CLIENT_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/Xft/Xft.h>
#include <glib.h>

#include "BlitBox.h"

class Manager;

const int PROTOCOLS_DELETE = 1 << 0;
const int PROTOCOLS_TAKEFOCUS = 1 << 1;

const long STATE_HIDDEN = 1 << 0;
const long STATE_WITHDRAWN = 1 << 1;
const long STATE_ICONIC = 1 << 2;
const long STATE_EXPANDED = 1 << 3;

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

struct MotifWmHints {
  CARD32 flags;
  CARD32 functions;
  CARD32 decorations;
  INT32 inputMode;
  CARD32 status;
};

class Client {
public:
  Client(Manager *manager, Window window, bool shouldPlace);
  virtual ~Client();

private:
  Manager *m_manager;
  int m_screen;

  Window m_window;
  Window m_frame;

  BlitBox m_topPixmapI, m_rightPixmapI, m_bottomPixmapI, m_leftPixmapI;
  BlitBox m_topPixmapA, m_rightPixmapA, m_bottomPixmapA, m_leftPixmapA;
  Cursor m_cursor;

  char *m_title;

  struct { int x, y; } m_position, m_realPosition, m_edgePosition;
  struct { int w, h; } m_size;
  int m_borderWidth;

  XSizeHints m_sizeHints;
  unsigned long m_protocols;
  Client *m_transientFor;
  GList *m_transientList;

  CARD32 m_state;
  bool m_isAdopting;

  bool m_shouldPlace;

  CARD32 m_workspace;
  bool m_isHidden;

  bool m_hasVisibleFrame;

  // TODO: This is evil...
  enum Direction { DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST,
    DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST };

public:
  bool hasWindow(Window window);
  Window getFrame(void) const { return m_frame; }

  CARD32 getWorkspace(void) const { return m_workspace; }
  void setWorkspace(CARD32 workspace);

  void updateProperty(Atom atom, bool shouldDelete = false);
  void updateProtocolStatus(void);
  void updateTransientStatus(void);

  void addTransient(Client *client);
  void removeTransient(Client *client);
  void raiseTransients(void);

  char *getPropertyString(Atom atom);
  void sendMessage(Atom atom, long data);
  void sendConfigureNotify(void);

  void changeState(CARD32 state);
  void storeState(void);
  void deleteOrKill(bool alreadyDestroyed = false);

  void configure(int x, int y, int width, int height, int borderWidth,
    unsigned long mask, bool resetFrame = false);

  void moveInteractive(XButtonEvent *e);
  void resizeInteractive(XButtonEvent *e, Direction direction);

  void gravitate(int *x, int *y);
  void adjustSize(int *w, int *h);
  void adjustPosition(int *dx, int *dy);

  int pingButton(int x, int y);
  bool trackButton(int button, int x, int y);

  void activate(void);
  void place(void);

  void raise(void);
  void lower(void);

  void map(void);
  void mapRaised(void);
  void unmap(void);

  void eventButtonPress(XButtonEvent *e);
  void eventConfigureRequest(XConfigureRequestEvent *event);
  void eventCrossingNotify(XCrossingEvent *e);
  void eventExpose(XExposeEvent *e);
  void eventFocusIn(XFocusInEvent *event);
  void eventMapRequest(XMapRequestEvent *event);
  void eventMotionNotify(XMotionEvent *event);
  void eventPropertyNotify(XPropertyEvent *event);
  void eventUnmapNotify(XUnmapEvent *event);

  void hide(void);
  void unHide(void);

  void spawnMenu(XButtonEvent *event);

  int getTopWidth(void) { return m_hasVisibleFrame ? 18 : 0; }
  int getRightWidth(void) { return m_hasVisibleFrame ? 6 : 0; }
  int getBottomWidth(void) { return m_hasVisibleFrame ? 6 : 0; }
  int getLeftWidth(void) { return m_hasVisibleFrame ? 6 : 0; }

  void getEdges(int *left, int *top, int *right, int *bottom);

// TODO: Here be monsters...
private:
  Cursor positionToCursor(int x, int y);
  Direction positionToDirection(XButtonEvent *e);

  bool m_invalidWidth, m_invalidHeight;

public:
  static BlitBox *m_buttonUpI[3], *m_buttonDownI[3], *m_buttonUpA[3],
    *m_buttonDownA[3];
};

#endif
