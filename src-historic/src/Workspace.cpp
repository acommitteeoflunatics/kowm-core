// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "Workspace.h"

#include <string.h>

Workspace::Workspace(Manager *manager, const char *name) {
  m_manager = manager;
  if (name) {
    m_name = new char[strlen(name) + 1];
    strcpy(m_name, name);
  } else
    m_name = 0;
}

Workspace::~Workspace() {
  if (m_name)
    delete[] m_name;
}
