// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include <vector>
#include <X11/Xlib.h>

class Manager;
class BlitBox;

class Menu {
public:
  Menu(Manager *manager, Window parent);
  virtual ~Menu();

public:
  struct Item {
    char title[40];
    int id;
    BlitBox *active, *inactive;
  };

private:
  std::vector<Item *> m_itemList;
  Manager *m_manager;
  int m_parent;
  Window m_window;

public:
  void addItem(Item *item);
  int spawn(XButtonEvent *e);
};
