// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _WORKSPACE_H
#define _WORKSPACE_H

class Manager;

class Workspace {
public:
  Workspace(Manager *manager, const char *name);
  virtual ~Workspace();

private:
  Manager *m_manager;
  char *m_name;

public:
  char *getName(void) const { return m_name; }
};

#endif
