// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "Manager.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <X11/Xproto.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Atoms.h"
#include "Cursors.h"
#include "Client.h"
#include "Menu.h"
#include "Workspace.h"
#include "BlitBox.h"

#include "../config.h"

#define MAXVAL(a,b) ((a) > (b) ? (a) : (b))

int Manager::m_signal = 0;
bool Manager::m_hasSignal = false;
bool Manager::m_isInitializing = false;
bool Manager::m_isLooping = false;
int Manager::m_returnCode = 0;
bool Manager::m_testing = false;
bool Manager::m_quiet = true;

Manager::Manager() {
  m_isInitializing = true;

  m_sizeTracker = 0;
  m_sizeTrackerPixmap = 0;

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGHUP, signalHandler);

  m_display = XOpenDisplay(0);
  if (!m_display)
    fatalError("Cannot open display.");

//  XSynchronize(m_display, True);
  XSetErrorHandler(xErrorHandler);

  Atoms::initializeAtoms(m_display);
  Cursors::initializeCursors(m_display);

  // Initialize screens.
  m_screenCount = ScreenCount(m_display);
  m_rootWindows = g_array_sized_new(FALSE, FALSE, sizeof(Window),
    m_screenCount);

  m_clientList = 0;
  m_workspaceList = 0;
  m_activeWorkspace = 0;

  m_workspaceList = g_ptr_array_sized_new(4);
  g_ptr_array_add(m_workspaceList, (void *)(new Workspace(this, 0)));
  g_ptr_array_add(m_workspaceList, (void *)(new Workspace(this, 0)));
  g_ptr_array_add(m_workspaceList, (void *)(new Workspace(this, 0)));
  g_ptr_array_add(m_workspaceList, (void *)(new Workspace(this, 0)));

  for (int i = 0; i < m_screenCount; i++) {
    m_rootWindows = g_array_insert_val(m_rootWindows, i,
      RootWindow(m_display, i));
    XSelectInput(m_display, g_array_index(m_rootWindows, Window, i),
      SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask |
      ButtonReleaseMask | PropertyChangeMask);
  }

  // We sync because syncing makes us look cool.
  XSync(m_display, False);
  m_isInitializing = true;
}

Manager::~Manager() {
  g_array_free(m_rootWindows, FALSE);
  if (m_clientList)
    g_list_free(m_clientList);
  if (m_workspaceList) {
    for (guint i = 0; i < 4; i++)
      delete (Workspace *)g_ptr_array_index(m_workspaceList, i);
    g_ptr_array_free(m_workspaceList, FALSE);
  }
  XCloseDisplay(m_display);
}

bool Manager::isRootWindow(Window window) {
  for (int i = 0; i < m_screenCount; i++) {
    if (getRootWindow(i) == window)
      return true;
  }

  return false;
}

void Manager::exec(char *command) {
  // This function is fashionably bizarre.
  // Forks are cool.

  char *envDisplay = DisplayString(m_display);

  if (fork() == 0) {
    if (fork() == 0) {
      setsid();

      close(ConnectionNumber(m_display));

      if (envDisplay && envDisplay[0] != 0) {
        char *envString = (char *)malloc(strlen(envDisplay) + 10);
        sprintf(envString, "DISPLAY=%s", envDisplay);
        putenv(envString);
      }

      execlp(command, command, 0);
      fprintf(stderr, "mavosxwm: exec %s", command);
      perror(" failed");

      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
  }

  wait(0);
}

int Manager::getProperty(Window window, Atom atom, Atom type, long length,
  unsigned char **property)
{
  Atom realType;
  int format;
  unsigned long n, extra;
  int status;

  status = XGetWindowProperty(m_display, window, atom, 0, length, False, type,
    &realType, &format, &n, &extra, property);

  if (status != Success || *property == 0)
    return -1;
  if (n == 0)
    XFree((void *)*property);

  return n;
}

void Manager::signalHandler(int signum) {
  m_signal = signum;
  m_hasSignal = true;
}

void Manager::fatalError(char *message) {
  fprintf(stderr, "mavosxwm: Fatal error - %s\n", message);
  exit(EXIT_FAILURE);
}

int Manager::xErrorHandler(Display *display, XErrorEvent *errorEvent) {
  if (m_testing) {
    m_testing = false;
    return 0;
  }

  if (m_quiet && errorEvent->error_code == BadWindow)
    return 0;

  if (m_isInitializing && errorEvent->request_code ==
    X_ChangeWindowAttributes && errorEvent->error_code == BadAccess)
  {
    fprintf(stderr, "mavosxwm: There is another window manager running.\n");
    exit(EXIT_FAILURE);
  }

  char message[256];
  XGetErrorText(display, errorEvent->error_code, message, 256);
  fprintf(stderr, "mavosxwm: %s\n", message);

  return 0;
}

void Manager::installCursor(Cursor cursor, Window window) {
  XSetWindowAttributes attr;
  attr.cursor = cursor;
  XChangeWindowAttributes(m_display, window, CWCursor, &attr);
}

void Manager::spawnRootMenu(XButtonEvent *event, Window root) {
  Menu rootMenu(this, root);
  Menu::Item xterm, blaj1, blaj2, blaj3;

  strcpy(xterm.title, "xterm"); xterm.id = 32;
  rootMenu.addItem(&xterm);
  strcpy(blaj1.title, "blaj1"); blaj1.id = 64;
  rootMenu.addItem(&blaj1);
  strcpy(blaj2.title, "blaj2"); blaj2.id = 65;
  rootMenu.addItem(&blaj2);
  strcpy(blaj3.title, "blaj3"); blaj3.id = 66;
  rootMenu.addItem(&blaj3);

  switch(rootMenu.spawn(event)) {
  case 32:
    exec("xterm");
    break;
  }
}

int Manager::attemptGrab(Window window, Window constrain, int mask, int time) {
  if (time == 0)
    time = CurrentTime;
  return XGrabPointer(m_display, window, False, mask, GrabModeAsync,
    GrabModeAsync, constrain, None, time);
}

void Manager::releaseGrab(XButtonEvent *event) {
  XUngrabPointer(m_display, event->time);
}

void Manager::scanInitialWindows(void) {
  Window *windows, root, parent;
  unsigned int windowCount, j;

  for (int i = 0; i < m_screenCount; i++) {
    XQueryTree(m_display, g_array_index(m_rootWindows, Window, i), &root,
      &parent, &windows, &windowCount);

    for (j = 0; j < windowCount; j++)
      windowToClient(windows[j], true, false);

    if (windows)
      XFree((void *)windows);
  }
}

Client *Manager::windowToClient(Window window, bool create, bool place) {
  for (GList *l = g_list_first(m_clientList); l != 0; l = g_list_next(l)) {
    if (((Client *)l->data)->hasWindow(window))
      return (Client *)l->data;
  }

  if (create) {
    Window *windows, root, parent;
    unsigned int windowCount, j;
    bool found = false;

    // A bizarre hack to prevent creation of windows that don't exist... (!?)

    for (int i = 0; i < m_screenCount; i++) {
      XQueryTree(m_display, g_array_index(m_rootWindows, Window, i), &root,
        &parent, &windows, &windowCount);

      for (j = 0; j < windowCount; j++) {
        if (windows[j] == window)
          found = true;
      }

      if (windows)
        XFree((void *)windows);
    }

    if (!found)
      return 0;

    XWindowAttributes attributes;
    XGetWindowAttributes(m_display, window, &attributes);
    if (attributes.override_redirect)
      return 0;

    Client *client = new Client(this, window, place);
    m_clientList = g_list_append(m_clientList, client);
    return client;
  }

  // A dull ending. This never happens... I think.
  return 0;
}

// This is borrowed from wm2. I don't like wm2.
void Manager::nextEvent(XEvent *e) {
  int fd;
  fd_set rfds;
  struct timeval t;
  int r;

  if (!m_hasSignal) {
  waiting:
    if (QLength(m_display) > 0) {
      XNextEvent(m_display, e);
      return;
    }

    fd = ConnectionNumber(m_display);
    memset((void *)&rfds, 0, sizeof(fd_set)); // SGI's FD_ZERO is fucked
    FD_SET(fd, &rfds);
    t.tv_sec = t.tv_usec = 0;

    if (select(fd + 1, &rfds, NULL, NULL, &t) == 1) {
      XNextEvent(m_display, e);
      return;
    }

    XFlush(m_display);
    FD_SET(fd, &rfds);
    t.tv_sec = 0; t.tv_usec = 20000;

    if ((r = select(fd + 1, &rfds, NULL, NULL,
        (struct timeval *)NULL)) == 1)
    {
      XNextEvent(m_display, e);
      return;
    }

    if (r == 0)
      goto waiting;

    if (errno != EINTR || !m_hasSignal) {
      perror("mavosx: select failed");
      m_isLooping = false;
    }
  }

  fprintf(stderr, "mavosxwm: Signal caught, exiting.\n");
  m_isLooping = false;
  m_returnCode = 0;
}

int Manager::run(int argc, char *argv[]) {
  printf("\nmavosxwm-%s\nCopyright (c) 2002\nMartin Vollrathson\n\n", VERSION);

  scanInitialWindows();

  grabKeyWithIgnore(100, ControlMask | Mod1Mask, getRootWindow(0),
    GrabModeAsync, GrabModeAsync);
  grabKeyWithIgnore(102, ControlMask | Mod1Mask, getRootWindow(0),
    GrabModeAsync, GrabModeAsync);

  XEvent event;
  m_isLooping = true;

  while (m_isLooping) {
    nextEvent(&event);

    switch (event.type) {
    case ButtonPress:
      eventButtonPress(&event.xbutton);
      break;

    case ClientMessage:
      eventClientMessage(&event.xclient);
      break;

    case ConfigureRequest:
      eventConfigureRequest(&event.xconfigurerequest);
      break;

    case CreateNotify:
      eventCreateNotify(&event.xcreatewindow);
      break;

    case DestroyNotify:
      eventDestroyNotify(&event.xdestroywindow);
      break;

    case EnterNotify:
    case LeaveNotify:
      eventCrossingNotify(&event.xcrossing);
      break;

    case Expose:
      eventExpose(&event.xexpose);
      break;

    case FocusIn:
      eventFocusIn(&event.xfocus);
      break;

    case KeyPress:
      eventKeyPress(&event.xkey);

    case MapRequest:
      eventMapRequest(&event.xmaprequest);
      break;

    case MotionNotify:
      eventMotionNotify(&event.xmotion);
      break;

    case PropertyNotify:
      eventPropertyNotify(&event.xproperty);
      break;

    case ReparentNotify:
      eventReparentNotify(&event.xreparent);
      break;

    case UnmapNotify:
      eventUnmapNotify(&event.xunmap);
      break;
    }
  }

  for (GList *l = g_list_first(m_clientList); l != 0; l = g_list_next(l))
    delete (Client *)l->data;

  return m_returnCode;
}

void Manager::eventButtonPress(XButtonEvent *event) {
  if (isRootWindow(event->window) && event->button == Button3) {
    spawnRootMenu(event, event->window);
    return;
  }

  Client *client = windowToClient(event->window);
  if (client)
    client->eventButtonPress(event);
}

void Manager::eventClientMessage(XClientMessageEvent *event) {
//  Client *client = windowToClient(event->window);

  // TODO: Do something like this if the client wants to be Iconic.
  // Currently, there's no iconic state.
/*  if (event->message_type == Atoms::wmChangeState) {
    if (c && e->format == 32 && e->data.l[0] == IconicState && c != 0) {
      if (c->isNormal())
        c->hide();
      return;
    }
  }*/

  fprintf(stderr, "mavosxwm: Unexpected XClientMessageEvent %s.\n",
    XGetAtomName(getDisplay(), event->message_type));
}

void Manager::eventConfigureRequest(XConfigureRequestEvent *event) {
  // TODO: This is almost identical to wm2, make it better.

  XWindowChanges wc;
  Client *client = windowToClient(event->window);
  event->value_mask &= ~CWSibling;

  if (client)
    client->eventConfigureRequest(event);
  else {
    wc.x = event->x;
    wc.y = event->y;
    wc.width  = event->width;
    wc.height = event->height;
    // TODO: I don't think we should set border_width to 0 here...
    // Who cares anyway... it'll work for now.
    wc.border_width = 0;
    wc.sibling = None;
    wc.stack_mode = Above;
    event->value_mask &= ~CWStackMode;
    event->value_mask |= CWBorderWidth;

    XConfigureWindow(m_display, event->window, event->value_mask, &wc);
  }
}

void Manager::eventCreateNotify(XCreateWindowEvent *event) {
  windowToClient(event->window, true);
}

void Manager::eventCrossingNotify(XCrossingEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventCrossingNotify(event);
}

void Manager::eventDestroyNotify(XDestroyWindowEvent *event) {
  Client *client = windowToClient(event->window);

  if (client) {
    client->deleteOrKill(true);
    m_clientList = g_list_remove(m_clientList, client);
    delete client;
  }
}

void Manager::eventExpose(XExposeEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventExpose(event);
}

void Manager::eventFocusIn(XFocusInEvent *event) {
  if (event->detail != NotifyNonlinearVirtual)
    return;

  Client *client = windowToClient(event->window);
  if (client)
    client->eventFocusIn(event);
}

void Manager::eventKeyPress(XKeyEvent *event) {
  if (event->state & ControlMask && event->state & Mod1Mask &&
    event->keycode == 100)
  {
    if (m_activeWorkspace > 0)
      switchToWorkspace(m_activeWorkspace - 1);
  }

  if (event->state & ControlMask && event->state & Mod1Mask &&
    event->keycode == 102)
  {
    if (m_activeWorkspace < 3)
      switchToWorkspace(m_activeWorkspace + 1);
  }
}

void Manager::eventMapRequest(XMapRequestEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventMapRequest(event);
}

void Manager::eventMotionNotify(XMotionEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventMotionNotify(event);
}

void Manager::eventPropertyNotify(XPropertyEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventPropertyNotify(event);
}

void Manager::eventReparentNotify(XReparentEvent *event) {
  if (!isRootWindow(event->parent)) {
    Client *client = windowToClient(event->window);
    if (client && event->parent != client->getFrame()) {
      m_clientList = g_list_remove(m_clientList, client);
      delete client;
    }
  } else
    windowToClient(event->window, true);
}

void Manager::eventUnmapNotify(XUnmapEvent *event) {
  Client *client = windowToClient(event->window);
  if (client)
    client->eventUnmapNotify(event);
}

Workspace *Manager::getWorkspace(CARD32 workspace) {
  return (Workspace *)g_ptr_array_index(m_workspaceList, (guint) workspace);
}

bool Manager::windowExists(Window window) {
  int a;
  unsigned int b;
  Window root;

  XSync(m_display, False);
  m_testing = m_quiet = true;
  XGetGeometry(m_display, window, &root, &a, &a, &b, &b, &b, &b);
  XSync(m_display, False);
  m_quiet = false;

  if (m_testing) {
    m_testing = false;
    return true;
  } else
    return false;
}

void Manager::setActiveClient(Client* client) {
  Client *temp = m_activeClient;
  m_activeClient = client;
  if (g_list_find(m_clientList, temp))
    temp->eventExpose(0);
  client->eventExpose(0);
}

void Manager::sizeTracker(int screen, int x, int y, int w, int h, int displayW,
  int displayH)
{
  if (!m_sizeTracker) {
    XSetWindowAttributes attr;
    attr.override_redirect = True;

    m_sizeTracker = XCreateWindow(m_display, getRootWindow(screen), x, y, 100,
      50, 0, CopyFromParent, CopyFromParent, CopyFromParent,
      CWOverrideRedirect, &attr);
  }

  if (!m_sizeTrackerPixmap)
    m_sizeTrackerPixmap = new BlitBox(this, screen);

  char text[32];
  sprintf(text, "%d x %d", displayW, displayH);

  XGlyphInfo xgi;
  m_sizeTrackerPixmap->getTextExtents(&xgi, text, strlen(text));

  int tw = xgi.width + 10;
  int th = xgi.height + 10;

  m_sizeTrackerPixmap->configure(tw, th);
  m_sizeTrackerPixmap->renderInfoBox(xgi.x, xgi.y, tw, th, text);

  XMoveResizeWindow(m_display, m_sizeTracker, MAXVAL(x + w / 2 - tw / 2, 0),
    MAXVAL(y + h / 2 - th / 2, 0), tw, th);
  XMapRaised(m_display, m_sizeTracker);
  m_sizeTrackerPixmap->copyArea(0, 0, tw, th, m_sizeTracker, 0, 0);
}

void Manager::endSizeTracker(void) {
  if (m_sizeTracker) {
    XDestroyWindow(m_display, m_sizeTracker);
    m_sizeTracker = 0;
  }

  if (m_sizeTrackerPixmap) {
    delete m_sizeTrackerPixmap;
    m_sizeTrackerPixmap = 0;
  }
}

GList *Manager::findClientsByWorkspace(CARD32 workspace, Client *exclude) {
  GList *clients = 0;

  for (GList *l = g_list_first(m_clientList); l != 0; l = g_list_next(l)) {
    if ((Client *)l->data != exclude && ((Client *)l->data)->getFrame() != 0)
      clients = g_list_append(clients, l->data);
  }

  return clients;
}

void Manager::switchToWorkspace(CARD32 workspace) {
  Client *client;

  for (GList *l = g_list_first(m_clientList); l != 0; l = g_list_next(l)) {
    client = (Client *)l->data;
    if (client->getWorkspace() == m_activeWorkspace)
      client->hide();
    else if (client->getWorkspace() == workspace)
      client->unHide();
  }

  m_activeWorkspace = workspace;
}

void Manager::grabKeyWithIgnore(int keycode, unsigned int modifiers,
  Window window, int pointerMode, int keyboardMode)
{
  XGrabKey(m_display, keycode, modifiers | Mod2Mask | LockMask, window, False,
    pointerMode, keyboardMode);
  XGrabKey(m_display, keycode, modifiers | Mod2Mask, window, False,
    pointerMode, keyboardMode);
  XGrabKey(m_display, keycode, modifiers | LockMask, window, False,
    pointerMode, keyboardMode);
  XGrabKey(m_display, keycode, modifiers, window, False,
    pointerMode, keyboardMode);
}
