// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _MANAGER_H
#define _MANAGER_H

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <glib.h>

class Client;
class Workspace;
class BlitBox;

class Manager {
public:
  Manager();
  virtual ~Manager();

private:
  static int m_signal;
  static bool m_hasSignal;
  static bool m_isInitializing;
  static bool m_isLooping;
  static int m_returnCode;

  Display *m_display;
  int m_screenCount;
  GArray *m_rootWindows;

  GList *m_clientList;
  GPtrArray *m_workspaceList;
  CARD32 m_activeWorkspace;

  Client *m_activeClient;

public:
  void setActiveClient(Client* client);
  Client *getActiveClient(void) { return m_activeClient; }


  Display *getDisplay(void) { return m_display; }
  Window getRootWindow(int screen) { return g_array_index(m_rootWindows,
    Window, screen); }
  bool isRootWindow(Window window);

  int getProperty(Window window, Atom atom, Atom type, long length,
    unsigned char **property);

  static void signalHandler(int signum);
  void fatalError(char *message);
  static int xErrorHandler(Display *display, XErrorEvent *errorEvent);

  void installCursor(Cursor cursor, Window window);

  void spawnRootMenu(XButtonEvent *event, Window root);
  void exec(char *command);

  int attemptGrab(Window window, Window constrain, int mask, int time);
  void releaseGrab(XButtonEvent *event);

  void scanInitialWindows(void);
  Client *windowToClient(Window window, bool create = false, bool place = true);

  void nextEvent(XEvent *e);
  int run(int argc, char *argv[]);

  void eventButtonPress(XButtonEvent *event);
  void eventClientMessage(XClientMessageEvent *event);
  void eventConfigureRequest(XConfigureRequestEvent *event);
  void eventCreateNotify(XCreateWindowEvent *event);
  void eventCrossingNotify(XCrossingEvent *event);
  void eventDestroyNotify(XDestroyWindowEvent *event);
  void eventExpose(XExposeEvent *event);
  void eventFocusIn(XFocusInEvent *event);
  void eventKeyPress(XKeyEvent *event);
  void eventMapRequest(XMapRequestEvent *event);
  void eventMotionNotify(XMotionEvent *event);
  void eventPropertyNotify(XPropertyEvent *event);
  void eventReparentNotify(XReparentEvent *event);
  void eventUnmapNotify(XUnmapEvent *event);

  Workspace *getWorkspace(CARD32 workspace);
  CARD32 getActiveWorkspace(void) { return m_activeWorkspace; }
  bool windowExists(Window window);
  static bool m_testing, m_quiet;
  void setQuiet(bool quiet) { XSync(m_display, false); m_quiet = quiet; }
  Window m_sizeTracker;
  BlitBox *m_sizeTrackerPixmap;
  void sizeTracker(int screen, int x, int y, int w, int h, int displayW,
    int displayH);
  void endSizeTracker(void);
  GList *findClientsByWorkspace(CARD32 workspace, Client *exclude);
  void switchToWorkspace(CARD32 workspace);
  void grabKeyWithIgnore(int keycode, unsigned int modifiers, Window window,
    int pointerMode, int keyboardMode);
};


#endif
