// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "Client.h"

#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "Atoms.h"
#include "Cursors.h"
#include "Manager.h"
#include "Menu.h"

BlitBox *Client::m_buttonUpI[3] = { 0, 0, 0 };
BlitBox *Client::m_buttonDownI[3] = { 0, 0, 0 };
BlitBox *Client::m_buttonUpA[3] = { 0, 0, 0 };
BlitBox *Client::m_buttonDownA[3] = { 0, 0, 0 };

// TODO: Aaaaargh! More screen bullshit...

Client::Client(Manager *manager, Window window, bool shouldPlace) :
  m_topPixmapI(manager, 0), m_rightPixmapI(manager, 0),
  m_bottomPixmapI(manager, 0), m_leftPixmapI(manager, 0),
  m_topPixmapA(manager, 0), m_rightPixmapA(manager, 0),
  m_bottomPixmapA(manager, 0), m_leftPixmapA(manager, 0)
{
  m_manager = manager;
  m_window = window;
  m_frame = 0;

  if (m_buttonUpI[0] == 0) {
    for (int i = 0; i < 3; i++) {
      m_buttonUpI[i] = new BlitBox(m_manager, 0);
      m_buttonDownI[i] = new BlitBox(m_manager, 0);
      m_buttonUpA[i] = new BlitBox(m_manager, 0);
      m_buttonDownA[i] = new BlitBox(m_manager, 0);

      BlitBox::makeButton(i, false, m_buttonUpI[i], false);
      BlitBox::makeButton(i, true, m_buttonDownI[i], false);
      BlitBox::makeButton(i, false, m_buttonUpA[i], true);
      BlitBox::makeButton(i, true, m_buttonDownA[i], true);
    }
  }

  m_cursor = Cursors::normalCursor;
  m_shouldPlace = shouldPlace;

  XWindowAttributes attr;
  XGetWindowAttributes(m_manager->getDisplay(), m_window, &attr);

  m_screen = XScreenNumberOfScreen(attr.screen);

  m_title = 0;
  m_transientFor = 0;
  m_transientList = 0;
  m_protocols = 0;

  // TODO: No no no no. This must NOT be hard-coded to 1280.
  m_position.x = m_realPosition.x = attr.x % 1280;
  m_position.y = m_realPosition.y = attr.y;
  m_size.w = attr.width;
  m_size.h = attr.height;
  m_borderWidth = attr.border_width;
  m_invalidWidth = m_invalidHeight = true;

  m_isAdopting = false;
  m_state = WithdrawnState;
  storeState();

  m_isHidden = false;
  m_hasVisibleFrame = true;

  XSelectInput(m_manager->getDisplay(), m_window, PropertyChangeMask |
    FocusChangeMask);

  Atom actualType;
  int actualFormat;
  unsigned long itemsReturned, bytesAfter;
  unsigned char *property;

  if (XGetWindowProperty(m_manager->getDisplay(), m_window,
    Atoms::netWmDesktop, 0, 1, False, XA_CARDINAL, &actualType, &actualFormat,
    &itemsReturned, &bytesAfter, &property) == Success)
  {
    if (actualType == XA_CARDINAL)
      m_workspace = (CARD32) *property;
    else
      m_workspace = m_manager->getActiveWorkspace();

    XFree(property);
  } else
    m_workspace = m_manager->getActiveWorkspace();

  if (attr.map_state == IsViewable)
    changeState(NormalState);

  setWorkspace(m_workspace);
}

Client::~Client() {
  if (m_isHidden && m_window != 0)
    unHide();

  if (m_state != WithdrawnState) {
    if (m_window) {
      XReparentWindow(m_manager->getDisplay(), m_window,
        m_manager->getRootWindow(m_screen), m_position.x, m_position.y);
      XUngrabButton(m_manager->getDisplay(), AnyButton, AnyModifier, m_window);
      XSetWindowBorderWidth(m_manager->getDisplay(), m_window, m_borderWidth);
      XRemoveFromSaveSet(m_manager->getDisplay(), m_window);
    }

    XDestroyWindow(m_manager->getDisplay(), m_frame);
    m_frame = 0;
  }

  if (m_title)
    XFree(m_title);
  if (m_transientFor)
    m_transientFor->removeTransient(this);
}

bool Client::hasWindow(Window window) {
  if (window == 0)
    return false;
  else
    return window == m_window || window == m_frame;
}

void Client::setWorkspace(CARD32 workspace) {
  m_workspace = workspace;

  if (m_workspace == 0xFFFFFFFF)
    unHide();
  else if (m_workspace != m_manager->getActiveWorkspace())
    hide();

  XChangeProperty(m_manager->getDisplay(), m_window, Atoms::netWmDesktop,
    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&m_workspace, 1);
}

void Client::updateProperty(Atom atom, bool shouldDelete) {
  if (atom == Atoms::wmProtocols) {
    updateProtocolStatus();
    return;
  }

  if (atom == Atoms::mwmHints) {
    if (shouldDelete) {
      m_hasVisibleFrame = true;
      return;
    }

    MotifWmHints *mwm_hint = 0;
    Atom atom_return;
    int format;
    unsigned long num, len;

    int ret = XGetWindowProperty(m_manager->getDisplay(), m_window,
      Atoms::mwmHints, 0, 5, False, Atoms::mwmHints, &atom_return, &format,
      &num, &len, (unsigned char **)&mwm_hint);

    if (ret != Success || !mwm_hint || num < 3)
      m_hasVisibleFrame = true; // No MWM hints... most likely.
    else {
      if (mwm_hint->decorations == 0 && mwm_hint->flags & MWM_HINTS_DECORATIONS)
        m_hasVisibleFrame = false;
      else
        m_hasVisibleFrame = true;
    }

    if (ret == Success)
      XFree((void *)mwm_hint);

    configure(m_position.x, m_position.y, m_size.w, m_size.h, 0, 0, true);
    return;
  }

  // Check constant atoms...

  switch (atom) {
  case XA_WM_NAME:
    if (m_title)
      XFree(m_title);
    if (shouldDelete)
      m_title = 0;
    else
      m_title = getPropertyString(XA_WM_NAME);
      m_invalidWidth = true;
      eventExpose(0);
    break;

  case XA_WM_TRANSIENT_FOR:
    updateTransientStatus();
    break;

  case XA_WM_NORMAL_HINTS:
    long r;
    if (shouldDelete || !XGetWMNormalHints(m_manager->getDisplay(), m_window,
      &m_sizeHints, &r))
    {
      m_sizeHints.flags = 0;
    }
    break;
  }
}

void Client::updateProtocolStatus(void) {
  Atom *protocols;
  int count;

  m_protocols = 0;

  if (!XGetWMProtocols(m_manager->getDisplay(), m_window, &protocols, &count))
    return;

  for (int i = 0; i < count; i++) {
    if (protocols[i] == Atoms::wmDelete)
      m_protocols |= PROTOCOLS_DELETE;
    else if (protocols[i] == Atoms::wmTakeFocus)
      m_protocols |= PROTOCOLS_TAKEFOCUS;
  }

  XFree((void *)protocols);
}

void Client::updateTransientStatus(void) {
  if (m_transientFor)
    m_transientFor->removeTransient(this);

  Window transientFor = 0;

  if (XGetTransientForHint(m_manager->getDisplay(), m_window, &transientFor)) {
    m_transientFor = m_manager->windowToClient(transientFor);
    if (m_transientFor)
      m_transientFor->addTransient(this);
  } else
    m_transientFor = 0;
}

void Client::addTransient(Client *client) {
  m_transientList = g_list_append(m_transientList, client);
}

void Client::removeTransient(Client *client) {
  m_transientList = g_list_remove(m_transientList, client);
}

void Client::raiseTransients(void) {
  for (GList *l = g_list_first(m_transientList); l != 0; l = g_list_next(l))
    ((Client *)l->data)->raise();
}

char *Client::getPropertyString(Atom atom) {
  unsigned char *property;

  if (m_manager->getProperty(m_window, atom, XA_STRING, 100, &property) <= 0)
    return 0;

  return (char *)property;
}

void Client::sendMessage(Atom atom, long data) {
  XEvent ev;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = m_window;
  ev.xclient.message_type = atom;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = data;
  ev.xclient.data.l[1] = CurrentTime;

  int status = XSendEvent(m_manager->getDisplay(), m_window, False, 0, &ev);

  if (!status)
    fprintf(stderr, "mavosxwm: Client::sendMessage() failed.\n");
}

void Client::sendConfigureNotify(void) {
  XConfigureEvent event;
  event.type = ConfigureNotify;
  event.event = m_window;
  event.window = m_window;
  event.x = m_realPosition.x;
  event.y = m_realPosition.y;
  event.width  = m_size.w;
  event.height = m_size.h;
  event.border_width = m_frame == 0 ? m_borderWidth : 0;
  event.above = None;
  event.override_redirect = False;

  XSendEvent(m_manager->getDisplay(), m_window, False, StructureNotifyMask,
    (XEvent*)&event);
}

void Client::changeState(CARD32 state) {
  if (state == m_state)
    return;

  CARD32 oldState = m_state;
  m_state = state;

  if (oldState == WithdrawnState && m_state != WithdrawnState) {
    // ICCCM: The window manager will examine the contents of these properties
    // when the window makes the transition from the Withdrawn state.

    updateProperty(XA_WM_NAME);
    updateProperty(XA_WM_NORMAL_HINTS);
    updateProperty(Atoms::wmProtocols);
    updateProperty(XA_WM_TRANSIENT_FOR);
    updateProperty(Atoms::mwmHints);

    m_invalidWidth = m_invalidHeight = false;
    m_cursor = Cursors::normalCursor;

    m_frame = XCreateSimpleWindow(m_manager->getDisplay(),
      m_manager->getRootWindow(m_screen), 1, 1, 1, 1, 0, 0, 0);
    XSetWindowBorderWidth(m_manager->getDisplay(), m_window, 0);

    XSetWindowAttributes attributes;
    attributes.background_pixmap = None;
    XChangeWindowAttributes(m_manager->getDisplay(), m_frame, CWBackPixmap,
      &attributes);

    XSelectInput(m_manager->getDisplay(), m_frame, EnterWindowMask |
      LeaveWindowMask | ExposureMask | SubstructureRedirectMask |
      SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
      PointerMotionMask);

    XGrabButton(m_manager->getDisplay(), Button1, Mod1Mask, m_window, True,
      ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
      None);
    XGrabButton(m_manager->getDisplay(), Button2, Mod1Mask, m_window, True,
      ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
      None);
    XGrabButton(m_manager->getDisplay(), Button3, Mod1Mask, m_window, True,
      ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
      None);

    m_isAdopting = true;
    XReparentWindow(m_manager->getDisplay(), m_window, m_frame, getLeftWidth(),
      getTopWidth());

    XAddToSaveSet(m_manager->getDisplay(), m_window);

    configure(m_position.x, m_position.y, m_size.w, m_size.h, m_borderWidth,
      CWX | CWY | CWWidth | CWHeight);
  }

  switch (m_state) {
  case NormalState:
    storeState();

    if (m_transientFor) {
      configure(m_transientFor->m_position.x + m_transientFor->m_size.w / 2 -
        m_size.w / 2, m_transientFor->m_position.y +
        m_transientFor->m_size.h / 2 - m_size.h / 2, m_size.w, m_size.h,
        m_borderWidth, CWX | CWY);
    }

    XMapWindow(m_manager->getDisplay(), m_window);
    map();

    if (m_transientFor == m_manager->getActiveClient())
      activate();
    break;

  case WithdrawnState:
    XUnmapWindow(m_manager->getDisplay(), m_frame);

    XReparentWindow(m_manager->getDisplay(), m_window,
      m_manager->getRootWindow(m_screen), m_position.x, m_position.y);

    XUngrabButton(m_manager->getDisplay(), AnyButton, AnyModifier, m_window);
    XSetWindowBorderWidth(m_manager->getDisplay(), m_window, m_borderWidth);

    XRemoveFromSaveSet(m_manager->getDisplay(), m_window);

    XDestroyWindow(m_manager->getDisplay(), m_frame);
    m_frame = 0;

    storeState();
    break;
  }
}

void Client::storeState(void) {
  CARD32 data[2];
  data[0] = m_state;
  data[1] = (CARD32)None;

  XChangeProperty(m_manager->getDisplay(), m_window, Atoms::wmState,
    Atoms::wmState, 32, PropModeReplace, (unsigned char *)data, 2);
}

void Client::deleteOrKill(bool alreadyDestroyed) {
  if (alreadyDestroyed) {
    m_window = 0;
    return;
  }

// TODO: Why change state? No! Play with _NET_WM_PING and timeouts here...
//  changeState(WithdrawnState);

  if (m_protocols & PROTOCOLS_DELETE)
    sendMessage(Atoms::wmProtocols, Atoms::wmDelete);
  else
    XKillClient(m_manager->getDisplay(), m_window);
}

void Client::configure(int x, int y, int width, int height, int borderWidth,
  unsigned long mask, bool resetFrame)
{
  mask &= CWX | CWY | CWWidth | CWHeight | CWBorderWidth;

  if (mask && CWBorderWidth)
    m_borderWidth = borderWidth;

  XWindowChanges wc;

  if (m_frame) {
    mask &= CWX | CWY | CWWidth | CWHeight;

    m_position.x = mask & CWX ? x : m_position.x;
    m_position.y = mask & CWY ? y : m_position.y;

    gravitate(&x, &y);
    wc.x = x;
    wc.y = y;
    wc.width = width + getLeftWidth() + getRightWidth();
    wc.height = height + getTopWidth() + getBottomWidth();

    m_realPosition.x = mask & CWX ? x + getLeftWidth() : m_realPosition.x;
    m_realPosition.y = mask & CWY ? y + getTopWidth() : m_realPosition.y;
    m_size.w = mask & CWWidth ? width : m_size.w;
    m_size.h = mask & CWHeight ? height : m_size.h;

    if (mask & CWWidth)
      m_invalidWidth = true;
    if (mask & CWHeight)
      m_invalidHeight = true;

    XConfigureWindow(m_manager->getDisplay(), m_frame, mask, &wc);

    mask &= CWWidth | CWHeight;
    wc.width = m_size.w;
    wc.height = m_size.h;

    if (resetFrame) {
      mask |= CWX | CWY;
      wc.x = getLeftWidth();
      wc.y = getTopWidth();
    }

    XConfigureWindow(m_manager->getDisplay(), m_window, mask, &wc);
  } else {
    if (mask & CWX)
      wc.x = m_position.x = m_realPosition.x = x;
    if (mask & CWY)
      wc.y = m_position.y = m_realPosition.y = y;
    if (mask & CWWidth)
      wc.width = m_size.w = width;
    if (mask & CWHeight)
      wc.height = m_size.h = height;
    if (mask & CWBorderWidth)
      wc.border_width = borderWidth;

    XConfigureWindow(m_manager->getDisplay(), m_window, mask, &wc);
  }

  sendConfigureNotify();
}

void Client::moveInteractive(XButtonEvent *e) {
  struct {
    int x, y;
  } basePosition, newPosition, initialPosition;

  int dx, dy;

  initialPosition.x = m_position.x;
  initialPosition.y = m_position.y;

  Window child = 0, root = m_manager->getRootWindow(m_screen);
  XTranslateCoordinates(m_manager->getDisplay(), e->window, root, e->x, e->y,
    &basePosition.x, &basePosition.y, &child);

  if (m_manager->attemptGrab(m_manager->getRootWindow(m_screen), None,
    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, e->time) !=
    GrabSuccess)
  {
    return;
  }

  XEvent event;
  bool done = false, found = false;
  struct timeval sleepval;

  while (!done) {
    found = false;

    while (XCheckMaskEvent(m_manager->getDisplay(), ButtonPressMask |
           ButtonReleaseMask | ButtonMotionMask | ExposureMask, &event))
    {
      found = true;
      if (event.type != MotionNotify)
        break;
    }

    if (!found) {
      sleepval.tv_sec = 0;
      sleepval.tv_usec = 5000;
      select(0, 0, 0, 0, &sleepval);
      continue;
    }

    switch (event.type) {
    default:
      fprintf(stderr, "mavosxwm: Unknown event type %d in mI.\n", event.type);
      break;

    case NoExpose:
    case GraphicsExpose:
      break;

    case Expose:
      m_manager->eventExpose(&event.xexpose);
      break;

    case ButtonRelease:
      m_manager->releaseGrab(&event.xbutton);
      done = true;
      break;

    case MotionNotify:
      XTranslateCoordinates(m_manager->getDisplay(), event.xmotion.window, root,
        event.xmotion.x, event.xmotion.y, &newPosition.x, &newPosition.y,
        &child);

      dx = newPosition.x - basePosition.x;
      dy = newPosition.y - basePosition.y;
      //adjustPosition(&dx, &dy);

      configure(initialPosition.x + dx, initialPosition.y + dy, m_size.w,
        m_size.h, m_borderWidth, CWX | CWY);
      break;
    }
  }
}

void Client::resizeInteractive(XButtonEvent *e, Direction direction) {
  struct {
    int x, y;
  } basePosition, newPosition, initialPosition;

  struct {
    int w, h;
  } initialSize, newSize;

  initialPosition.x = m_position.x;
  initialPosition.y = m_position.y;
  initialSize.w = m_size.w;
  initialSize.h = m_size.h;

  int dw, dh, dx, dy;

  Window child = 0, root = m_manager->getRootWindow(m_screen);
  XTranslateCoordinates(m_manager->getDisplay(), e->window, root, e->x, e->y,
    &basePosition.x, &basePosition.y, &child);

  if (m_manager->attemptGrab(m_manager->getRootWindow(m_screen), None,
    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, e->time) !=
    GrabSuccess)
  {
    return;
  }

  m_manager->sizeTracker(m_screen, m_realPosition.x, m_realPosition.y,
    m_size.w, m_size.h, (m_size.w - (m_sizeHints.flags & PBaseSize ?
    m_sizeHints.base_width : 0)) / (m_sizeHints.flags & PResizeInc ?
    m_sizeHints.width_inc : 1), (m_size.h - (m_sizeHints.flags &
    PBaseSize ? m_sizeHints.base_height : 0)) / (m_sizeHints.flags &
    PResizeInc ? m_sizeHints.height_inc : 1));

  XEvent event;
  bool done = false, found = false;
  struct timeval sleepval;

  while (!done) {
    found = false;

    while (XCheckMaskEvent(m_manager->getDisplay(), ButtonPressMask |
           ButtonReleaseMask | ButtonMotionMask | ExposureMask, &event))
    {
      found = true;
      if (event.type != MotionNotify)
        break;
    }

    if (!found) {
      sleepval.tv_sec = 0;
      sleepval.tv_usec = 5000;
      select(0, 0, 0, 0, &sleepval);
      continue;
    }

    switch (event.type) {
    default:
      fprintf(stderr, "mavosxwm: Unknown event type %d in mI.\n", event.type);
      break;

    case NoExpose:
    case GraphicsExpose:
      break;

    case Expose:
      m_manager->eventExpose(&event.xexpose);
      break;

    case ButtonRelease:
      m_manager->releaseGrab(&event.xbutton);
      done = true;
      break;

    case MotionNotify:
      XTranslateCoordinates(m_manager->getDisplay(), event.xmotion.window, root,
        event.xmotion.x, event.xmotion.y, &newPosition.x, &newPosition.y,
        &child);

      dw = newPosition.x - basePosition.x;
      dh = newPosition.y - basePosition.y;

      switch (direction) {
      case DIR_NORTHWEST:
        dw = -dw;
        dh = -dh;
        break;

      case DIR_WEST:
        dw = -dw;
        dh = 0;
        break;

      case DIR_SOUTHWEST:
        dw = -dw;
        break;

      case DIR_NORTH:
        dw = 0;
        dh = -dh;
        break;

      case DIR_NORTHEAST:
        dh = -dh;
        break;

      case DIR_SOUTH:
        dw = 0;
        break;

      case DIR_EAST:
        dh = 0;
        break;

      default:
        dw = dw;
        dh = dh;
      }

      newSize.w = initialSize.w + dw;
      newSize.h = initialSize.h + dh;

      adjustSize(&newSize.w, &newSize.h);

      dx = newSize.w - initialSize.w;
      dy = newSize.h - initialSize.h;

      switch (direction) {
      case DIR_EAST:
      case DIR_SOUTH:
      case DIR_SOUTHEAST:
        dx = 0;
        dy = 0;
        break;

      case DIR_WEST:
      case DIR_SOUTHWEST:
        dy = 0;
        break;

      case DIR_NORTH:
      case DIR_NORTHEAST:
        dx = 0;
        break;

      default:
        dx = dx;
        dy = dy;
      }

      configure(initialPosition.x - dx, initialPosition.y - dy, newSize.w,
        newSize.h, m_borderWidth, CWX | CWY | CWWidth | CWHeight);
      m_manager->sizeTracker(m_screen, m_realPosition.x, m_realPosition.y,
        m_size.w, m_size.h, (m_size.w - (m_sizeHints.flags & PBaseSize ?
        m_sizeHints.base_width : 0)) / (m_sizeHints.flags & PResizeInc ?
        m_sizeHints.width_inc : 1), (m_size.h - (m_sizeHints.flags &
        PBaseSize ? m_sizeHints.base_height : 0)) / (m_sizeHints.flags &
        PResizeInc ? m_sizeHints.height_inc : 1));
      break;
    }
  }

  m_manager->endSizeTracker();
}

void Client::gravitate(int *x, int *y) {
  if (~m_sizeHints.flags & PWinGravity)
    m_sizeHints.win_gravity = NorthWestGravity;

  switch (m_sizeHints.win_gravity) {
  case NorthWestGravity:
    // Coordinates are unchanged.
    break;

  case NorthGravity:
    *x -= (getLeftWidth() + getRightWidth()) / 2;
    break;

  case NorthEastGravity:
    *x -= getLeftWidth() + getRightWidth();
    break;

  case WestGravity:
    *y -= (getTopWidth() + getBottomWidth()) / 2;
    break;

  case CenterGravity:
    *x -= (getLeftWidth() + getRightWidth()) / 2;
    *y -= (getTopWidth() + getBottomWidth()) / 2;
    break;

  case EastGravity:
    *x -= getRightWidth();
    *y -= (getTopWidth() + getBottomWidth()) / 2;
    break;

  case SouthWestGravity:
    *y -= getTopWidth() + getBottomWidth();
    break;

  case SouthGravity:
    *x -= (getLeftWidth() + getRightWidth()) / 2;
    *y -= getTopWidth() + getBottomWidth();
    break;

  case SouthEastGravity:
    *x -= getLeftWidth() + getRightWidth();
    *y -= getTopWidth() + getBottomWidth();
    break;

  default:
  case StaticGravity:
    *x -= getLeftWidth();
    *y -= getTopWidth();
  }
}

void Client::adjustSize(int *w, int *h) {
  int xInc, yInc;
  int xBase, yBase;
  int xMin, yMin;
  int xMax, yMax;

  if (m_sizeHints.flags & PResizeInc) {
    xInc = m_sizeHints.width_inc;
    yInc = m_sizeHints.height_inc;
  } else
    xInc = yInc = 1;

  if (m_sizeHints.flags & PBaseSize) {
    xBase = m_sizeHints.base_width;
    yBase = m_sizeHints.base_height;
  } else
    xBase = yBase = 0;

  if (m_sizeHints.flags & PMinSize) {
    xMin = m_sizeHints.min_width;
    yMin = m_sizeHints.min_height;
  } else
    xMin = yMin = 2;

  if (m_sizeHints.flags & PMaxSize) {
    xMax = m_sizeHints.max_width;
    yMax = m_sizeHints.max_height;
  } else
    xMax = yMax = 10240;

  // TODO: Disabled for incs?
  if (m_sizeHints.flags & PAspect && xInc == 1 && yInc == 1) {
    // TODO: This is scary stuff... loops may run wild.

    while ((double) *w / (double) *h >
      (double) m_sizeHints.min_aspect.x / (double) m_sizeHints.min_aspect.y)
    {
      *w -= xInc;
    }
    while ((double) *w / (double) *h <
      (double) m_sizeHints.max_aspect.x / (double) m_sizeHints.max_aspect.y)
    {
      *h -= yInc;
    }
  }

  *w -= (*w - xBase) % xInc;
  *h -= (*h - yBase) % yInc;

  while (*w < xMin)
    *w += xInc;
  while (*h < yMin)
    *h += yInc;

  while (*w > xMax)
    *w -= xInc;
  while (*h > yMax)
    *h -= yInc;
}

void Client::adjustPosition(int *dx, int *dy) {
  // TODO: Uhu... fix the workspace thing.
  // TODO: Optimize by moving client finder outside this function.
  int left, top, right, bottom, localLeft, localTop, localRight, localBottom;
  getEdges(&localLeft, &localTop, &localRight, &localBottom);
  GList *clients = m_manager->findClientsByWorkspace(0, this);

  for (GList *l = g_list_first(clients); l != 0; l = g_list_next(l)) {
    ((Client *)l->data)->getEdges(&left, &top, &right, &bottom);

    if (!((localLeft + *dx < left && localRight + *dx < left) ||
      (localLeft + *dx >= right && localRight + *dx >= right)))
    {
      if (localTop + *dy < bottom && localTop + *dy > bottom - 6)
        *dy = bottom - localTop;
    }

/*    if (localLeft + *dx < right && localLeft + *dx > right - 6)
      *dx += right - (localLeft + *dx);*/
/*    if (localRight + *dx > left && localRight + *dx < left + 6)
      *dx += left - (localRight + *dx);
    if (localBottom + *dy > top && localBottom + *dy < top + 6)
      *dy += top - (localBottom + *dy);*/
  }

  g_list_free(clients);
}

int Client::pingButton(int x, int y) {
  for (int i = 0; i < 3; i++) {
    if (x >= m_size.w + getLeftWidth() - 11 - 14 * i &&
      x < m_size.w + getLeftWidth() + 1 - 14 * i && y >= 3 && y < 15)
    {
      return i;
    }
  }

  return -1;
}

bool Client::trackButton(int button, int x, int y) {
  int newX = 0, newY = 0;
  int newButton;
  bool onButton = true;

  m_buttonDownA[button]->copyArea(0, 0, 12, 12, m_frame,
    m_size.w + getLeftWidth() - 11 - 14 * button, 3);

  if (m_manager->attemptGrab(m_manager->getRootWindow(m_screen), None,
    ButtonReleaseMask | ButtonMotionMask, CurrentTime) != GrabSuccess)
  {
    return false;
  }

  XEvent event;
  bool done = false, found = false;
  struct timeval sleepval;
  Window dummy;

  while (!done) {
    found = false;

    while (XCheckMaskEvent(m_manager->getDisplay(), ButtonReleaseMask |
      ButtonMotionMask, &event))
    {
      found = true;
      if (event.type != MotionNotify)
        break;
    }

    if (!found) {
      sleepval.tv_sec = 0;
      sleepval.tv_usec = 5000;
      select(0, 0, 0, 0, &sleepval);
      continue;
    }

    switch (event.type) {
    default:
      fprintf(stderr, "mavosxwm: Unknown event type.\n");
      break;

    case ButtonRelease:
      m_manager->releaseGrab(&event.xbutton);
      m_buttonUpA[button]->copyArea(0, 0, 12, 12, m_frame,
        m_size.w + getLeftWidth() - 11 - 14 * button, 3);
      return onButton;

    case MotionNotify:
      XTranslateCoordinates(m_manager->getDisplay(),
        m_manager->getRootWindow(m_screen), m_frame, event.xbutton.x,
        event.xbutton.y, &newX, &newY, &dummy);

      newButton = pingButton(newX, newY);
      if (newButton == button && !onButton) {
        onButton = true;
        m_buttonDownA[button]->copyArea(0, 0, 12, 12, m_frame,
          m_size.w + getLeftWidth() - 11 - 14 * button, 3);
      } else if (newButton != button && onButton) {
        onButton = false;
        m_buttonUpA[button]->copyArea(0, 0, 12, 12, m_frame,
          m_size.w + getLeftWidth() - 11 - 14 * button, 3);
      }
      break;
    }
  }

  return false;
}

void Client::activate(void) {
  // TODO: Extend this crap.
  if (m_manager->getActiveClient() != this) {
    XSetInputFocus(m_manager->getDisplay(), m_window, RevertToPointerRoot,
      CurrentTime);
    m_manager->setActiveClient(this);
  }
}

void Client::place(void) {
/*  m_x = WidthOfScreen(ScreenOfDisplay(m_manager->getDisplay(), m_screen)) / 2 -
    m_width / 2;
  m_y = HeightOfScreen(ScreenOfDisplay(m_manager->getDisplay(), m_screen)) / 2 -
    m_height / 2;
  m_frame->configure(m_x, m_y, m_width, m_height, CWX | CWY);
  sendConfigureNotify();*/
}

void Client::raise(void) {
  if (m_frame)
    XRaiseWindow(m_manager->getDisplay(), m_frame);
  raiseTransients();
}

void Client::lower(void) {
  if (m_frame)
    XLowerWindow(m_manager->getDisplay(), m_frame);
}

void Client::map(void) {
  if (m_frame)
    XMapWindow(m_manager->getDisplay(), m_frame);
}

void Client::mapRaised(void) {
  if (m_frame)
    XMapRaised(m_manager->getDisplay(), m_frame);
}

void Client::unmap(void) {
  if (m_frame)
    XUnmapWindow(m_manager->getDisplay(), m_frame);
}

void Client::eventButtonPress(XButtonEvent *e) {
  if (e->window == m_window) {
    if (e->button == Button1) {
      raise();
      moveInteractive(e);
    } else if (e->button == Button2) {
      raise();
      resizeInteractive(e, positionToDirection(e));
    } else {
      e->x += m_realPosition.x;
      e->y += m_realPosition.y;
      spawnMenu(e);
    }

    return;
  }

  int b = pingButton(e->x, e->y);
  if (b != -1) {
    if (trackButton(b, e->x, e->y) && b == 0)
      deleteOrKill();
    return;
  }

  switch (e->button) {
  case Button1:
    raise();

    if (m_cursor == Cursors::normalCursor)
      moveInteractive(e);
    else
      resizeInteractive(e, positionToDirection(e));
    break;

  case Button2:
    if (m_cursor == Cursors::normalCursor)
      resizeInteractive(e, positionToDirection(e));
    else
      moveInteractive(e);
    break;

  case Button3:
    lower();
    break;

  default:
    fprintf(stderr, "mavosxwm: No binding for button %d.\n", e->button);
  }
}

void Client::eventConfigureRequest(XConfigureRequestEvent *event) {
  configure(event->x, event->y, event->width, event->height,
    event->border_width, event->value_mask);
}

void Client::eventCrossingNotify(XCrossingEvent *e) {
  Cursor cursor;

  if (e->type == LeaveNotify)
    cursor = Cursors::normalCursor;
  else {
    activate();
    cursor = positionToCursor(e->x, e->y);
  }

  if (cursor != m_cursor) {
    m_cursor = cursor;
    m_manager->installCursor(m_cursor, m_frame);
  }
}

void Client::eventExpose(XExposeEvent *e) {
  if (e && e->count != 0)
    return; // Maybe we should care about subareas... to make it faster.

  // Be careful. eventExpose can actually be called with e == 0.
  // Never expect e to contain useable data without testing.

  if (!m_frame || !m_hasVisibleFrame)
    return; // This window doesn't need painting.

  int totalWidth = m_size.w + getLeftWidth() + getRightWidth();
  int totalHeight = m_size.h + getTopWidth() + getBottomWidth();

  if (m_invalidWidth || m_invalidHeight) {
    BlitBox::renderFrame(&m_leftPixmapI, &m_topPixmapI, &m_rightPixmapI,
      &m_bottomPixmapI, totalWidth, totalHeight, getLeftWidth(), getTopWidth(),
      getRightWidth(), getBottomWidth(), m_title);
    BlitBox::renderFrame(&m_leftPixmapA, &m_topPixmapA, &m_rightPixmapA,
      &m_bottomPixmapA, totalWidth, totalHeight, getLeftWidth(), getTopWidth(),
      getRightWidth(), getBottomWidth(), m_title, true);

    m_invalidWidth = m_invalidHeight = false;
  }

  if (m_manager->getActiveClient() == this) {
    m_topPixmapA.copyArea(0, 0, totalWidth, getTopWidth(), m_frame, 0, 0);
    m_bottomPixmapA.copyArea(0, 0, totalWidth, getBottomWidth(), m_frame, 0,
      totalHeight - getBottomWidth());

    m_leftPixmapA.copyArea(0, 0, getLeftWidth(), totalHeight - getTopWidth() -
      getBottomWidth(), m_frame, 0, getTopWidth());
    m_rightPixmapA.copyArea(0, 0, getRightWidth(), totalHeight - getTopWidth() -
      getBottomWidth(), m_frame, totalWidth - getRightWidth(), getTopWidth());
  } else {
    m_topPixmapI.copyArea(0, 0, totalWidth, getTopWidth(), m_frame, 0, 0);
    m_bottomPixmapI.copyArea(0, 0, totalWidth, getBottomWidth(), m_frame, 0,
      totalHeight - getBottomWidth());

    m_leftPixmapI.copyArea(0, 0, getLeftWidth(), totalHeight - getTopWidth() -
      getBottomWidth(), m_frame, 0, getTopWidth());
    m_rightPixmapI.copyArea(0, 0, getRightWidth(), totalHeight - getTopWidth() -
      getBottomWidth(), m_frame, totalWidth - getRightWidth(), getTopWidth());
  }
}

void Client::eventFocusIn(XFocusInEvent *event) {
  activate();
}

void Client::eventMapRequest(XMapRequestEvent *event) {
  switch(m_state) {
  case WithdrawnState:
    changeState(NormalState);
    break;

  case NormalState:
    XMapWindow(m_manager->getDisplay(), m_window);
    mapRaised();
    break;

  // TODO: Hmm... iconic? Noooo...
/*  case IconicState:
    unhide(True);
    break;*/
  }
}

void Client::eventMotionNotify(XMotionEvent *event) {
  Cursor cursor = positionToCursor(event->x, event->y);

  if (cursor != m_cursor) {
    m_cursor = cursor;
    m_manager->installCursor(m_cursor, m_frame);
  }
}

void Client::eventPropertyNotify(XPropertyEvent *event) {
  updateProperty(event->atom, event->state == PropertyDelete);
}

void Client::eventUnmapNotify(XUnmapEvent *event) {
  if (m_isAdopting) {
    m_isAdopting = false;
    return;
  }

  changeState(WithdrawnState);
}

void Client::hide(void) {
  if (m_isHidden)
    return;

  if (m_frame) {
    XWindowAttributes attr;
    XGetWindowAttributes(m_manager->getDisplay(), m_frame, &attr);
    m_edgePosition.x = attr.x;
    m_edgePosition.y = attr.y;
    // TODO: No no no no. This must NOT be hard-coded to 1280.
    XMoveWindow(m_manager->getDisplay(), m_frame, 1280 * 2 + attr.x, attr.y);

    m_isHidden = true;
  }
}

void Client::unHide(void) {
  if (!m_isHidden)
    return;

  if (m_frame) {
    XMoveWindow(m_manager->getDisplay(), m_frame, m_edgePosition.x,
      m_edgePosition.y);
    m_isHidden = false;
  }
}

void Client::spawnMenu(XButtonEvent *event) {
  Menu apor(m_manager, m_manager->getRootWindow(m_screen));
  Menu::Item sticky, close;

  strcpy(sticky.title, "Toggle sticky"); sticky.id = 23;
  strcpy(close.title, "Close"); close.id = 31;

  apor.addItem(&sticky);
  apor.addItem(&close);

  switch(apor.spawn(event)) {
  case 23:
    if (m_workspace == 0xFFFFFFFF)
      setWorkspace(m_manager->getActiveWorkspace());
    else
      setWorkspace(0xFFFFFFFF);
    break;

  case 31:
    deleteOrKill();
    break;
  }
}

void Client::getEdges(int *left, int *top, int *right, int *bottom) {
  *left = m_realPosition.x - getLeftWidth();
  *top = m_realPosition.y - getTopWidth();
  *right = m_realPosition.x + m_size.w + getRightWidth();
  *bottom = m_realPosition.y + m_size.h + getBottomWidth();
}

Cursor Client::positionToCursor(int x, int y) {
  if (y < getTopWidth())
    return Cursors::normalCursor;

  if (y >= getTopWidth() + m_size.h)
    return Cursors::resizeSouthCursor;

  if (x < getLeftWidth())
    return Cursors::resizeWestCursor;

  if (x >= getLeftWidth() + m_size.w)
    return Cursors::resizeEastCursor;

  return Cursors::normalCursor;
}

Client::Direction Client::positionToDirection(XButtonEvent *e) {
  bool north, east, south, west;

  if (e->window == m_window) {
    north = e->y < m_size.h / 3;
    south = e->y > m_size.h / 3 * 2;
    west = e->x < m_size.w / 3;
    east = e->x > m_size.w / 3 * 2;
  } else {
    north = e->y < getTopWidth();
    south = e->y > m_size.h - 20;
    west = e->x < 20;
    east = e->x > m_size.w - 20;
  }

  if (north) {
    if (east)
      return DIR_NORTHEAST;
    else if (west)
      return DIR_NORTHWEST;
    else
      return DIR_NORTH;
  } else if (south) {
    if (east)
      return DIR_SOUTHEAST;
    else if (west)
      return DIR_SOUTHWEST;
    else
      return DIR_SOUTH;
  } else {
    if (east)
      return DIR_EAST;
    else if (west)
      return DIR_WEST;
    else
      return DIR_SOUTHEAST; // This is the amazing freak.
  }
}
