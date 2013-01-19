// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#ifndef _BLITBOX_H
#define _BLITBOX_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

class Manager;

class BlitBox {
public:
  BlitBox(Manager *manager, int screen);
  virtual ~BlitBox();

private:
  Pixmap m_pixmap;
  XftDraw *m_xftDraw;
  Manager *m_manager;
  int m_screen;
  static XftFont *m_xftFont;

  static unsigned short symbols[3][6][6];

public:
  struct Color { double r, g, b, a; };

  static Color black;
  static Color dark;
  static Color darkActive;
  static Color edgeLight;
  static Color edgeDark;
  static Color backgroundLight;
  static Color backgroundDark;
  static Color edgeLightActive;
  static Color edgeDarkActive;
  static Color backgroundLightActive;
  static Color backgroundDarkActive;

public:
  void drawBlendH(int x, int y, int w, int h, Color *start, Color *end);
  void drawBlendV(int x, int y, int w, int h, Color *start, Color *end);
  void drawRectangle(int x, int y, int w, int h, Color *color);
  void drawOpenRectangle(int x, int y, int w, int h, Color *color);
  void drawText(int x, int y, const char *text, size_t length, Color *color);

public:
  void getTextExtents(XGlyphInfo *xgi, const char *text, size_t length);

public:
  void copyArea(int sx, int sy, int w, int h, Drawable d, int dx, int dy);
  void copyArea(int sx, int sy, int w, int h, BlitBox *d, int dx, int dy);
  void copyAreaBack(int sx, int sy, int w, int h, Drawable d, int dx, int dy);

public:
  void configure(int w, int h);
  static void renderFrame(BlitBox *left, BlitBox *top, BlitBox *right,
    BlitBox *bottom, int width, int height, int leftWidth, int topWidth,
    int rightWidth, int bottomWidth, char *title, bool active = false);
  static void makeButton(int button, bool pressed, BlitBox *blitBox,
    bool active);

  void renderInfoBox(int x, int y, int w, int h, char *text);
};

#endif
