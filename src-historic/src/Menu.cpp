// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include <iostream>
#include "Menu.h"
#include "Manager.h"
#include "BlitBox.h"

Menu::Menu(Manager *manager, Window parent) {
  m_manager = manager;
  m_parent = parent;
  m_window = 0;
}

Menu::~Menu() {
  if (m_window)
    XDestroyWindow(m_manager->getDisplay(), m_window);
}

void Menu::addItem(Item *item) {
  m_itemList.insert(m_itemList.end(), item);
}

int Menu::spawn(XButtonEvent *e) {
  if (!m_window) {
    XSetWindowAttributes attr;
    attr.override_redirect = True;

    m_window = XCreateWindow(m_manager->getDisplay(), m_parent,
      0, 0, 1, 1, 0, CopyFromParent, CopyFromParent, CopyFromParent,
      CWOverrideRedirect, &attr);

    XSelectInput(m_manager->getDisplay(), m_window, ButtonPressMask |
      ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | ExposureMask);
  }

  XGlyphInfo xgi;
  // TODO: Fix screen(second argument to menuBlit).
  BlitBox menuBlit(m_manager, 0);
  menuBlit.getTextExtents(&xgi, "AaKkGgFfPp", 10);

  int itemHeight = xgi.height + 4;
  int totalHeight = itemHeight * m_itemList.size();

  XMoveResizeWindow(m_manager->getDisplay(), m_window, e->x, e->y, 202,
    totalHeight + 2);
  XMapRaised(m_manager->getDisplay(), m_window);

  BlitBox::Color color = { 0.0, 0.0, 0.0, 0.8 };
  size_t i;

  for (i = 0; i < m_itemList.size(); i++) {
    // TODO: Fix screen(second argument to new BlitBox).
    m_itemList[i]->active = new BlitBox(m_manager, 0);
    m_itemList[i]->active->configure(200, itemHeight);
    m_itemList[i]->active->copyAreaBack(1, 1 + i * itemHeight, 200, itemHeight,
      m_window, 0, 0);
    color.r = 0.72; color.g = 0.29; color.b = 0.08;
    m_itemList[i]->active->drawRectangle(0, 0, 200, itemHeight, &color);
    color.r = 0.90; color.g = 0.90; color.b = 0.90;
    m_itemList[i]->active->drawText(5, xgi.y + 2, m_itemList[i]->title,
      strlen(m_itemList[i]->title), &color);

    // TODO: Fix screen(second argument to new BlitBox).
    m_itemList[i]->inactive = new BlitBox(m_manager, 0);
    m_itemList[i]->inactive->configure(200, itemHeight);
    m_itemList[i]->inactive->copyAreaBack(1, 1 + i * itemHeight, 200,
      itemHeight, m_window, 0, 0);
    color.r = 0.90; color.g = 0.90; color.b = 0.90;
    m_itemList[i]->inactive->drawRectangle(0, 0, 200, itemHeight, &color);
    color.r = 0.0; color.g = 0.0; color.b = 0.0;
    m_itemList[i]->inactive->drawText(5, xgi.y + 2, m_itemList[i]->title,
      strlen(m_itemList[i]->title), &color);
  }

  if (m_manager->attemptGrab(m_window, None, ButtonPressMask |
      ButtonReleaseMask | ButtonMotionMask | PointerMotionMask |
      StructureNotifyMask, e->time) != GrabSuccess)
  {
    XUnmapWindow(m_manager->getDisplay(), m_window);
    return -1;
  }

  XEvent event;
  bool done = false;
  int selection = -1, nextSelection;
  int x, y;

  while (!done) {
    XMaskEvent(m_manager->getDisplay(), ButtonPressMask | ButtonReleaseMask |
      ButtonMotionMask | PointerMotionMask | ExposureMask, &event);

    switch (event.type) {
    case ButtonPress:
      if (selection == -1)
        done = true;
      break;

    case ButtonRelease:
      if (selection != -1)
        done = true;
      break;

    case MotionNotify:
      x = event.xbutton.x;
      y = event.xbutton.y;

      if (x > 0 && y > 0 && x <= 200 && y <= totalHeight)
        nextSelection = (event.xbutton.y - 1) / itemHeight;
      else
        nextSelection = -1;

      if (nextSelection != selection) {
        if (selection != -1) {
          m_itemList[selection]->inactive->copyArea(0, 0, 200, itemHeight,
            m_window, 1, 1 + selection * itemHeight);
        }

        if (nextSelection != -1) {
          m_itemList[nextSelection]->active->copyArea(0, 0, 200, itemHeight,
            m_window, 1, 1 + nextSelection * itemHeight);
        }

        selection = nextSelection;
      }
      break;

    case Expose:
      for (i = 0; i < m_itemList.size(); i++) {
        if (selection != -1 && i == (size_t) selection) {
          m_itemList[i]->active->copyArea(0, 0, 200, itemHeight, m_window, 1,
            1 + i * itemHeight);
        } else {
          m_itemList[i]->inactive->copyArea(0, 0, 200, itemHeight, m_window, 1,
            1 + i * itemHeight);
        }
      }
    }
  }

  m_manager->releaseGrab(&event.xbutton);
  XUnmapWindow(m_manager->getDisplay(), m_window);

  for (i = 0; i < m_itemList.size(); i++) {
    delete m_itemList[i]->active;
    delete m_itemList[i]->inactive;
  }

  if (selection == -1)
    return -1;
  else
    return m_itemList[selection]->id;
}
