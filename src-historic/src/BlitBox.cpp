// Copyright (C) 2002 Martin Vollrathson
// See COPYING for boring license stuff.

#include "BlitBox.h"

#include "Manager.h"
#include "Client.h"

XftFont *BlitBox::m_xftFont = 0;

BlitBox::Color BlitBox::black = { 0.0, 0.0, 0.0, 1.0 };
BlitBox::Color BlitBox::dark = { 0.49, 0.49, 0.49, 1.0 };
BlitBox::Color BlitBox::darkActive = { 0.49, 0.48, 0.41, 1.0 };
BlitBox::Color BlitBox::edgeLight = { 0.96, 0.96, 0.96, 1.0 };
BlitBox::Color BlitBox::edgeDark = { 0.85, 0.85, 0.85, 1.0 };
BlitBox::Color BlitBox::backgroundLight = { 0.95, 0.95, 0.95, 1.0 };
BlitBox::Color BlitBox::backgroundDark = { 0.86, 0.86, 0.86, 1.0 };
BlitBox::Color BlitBox::edgeLightActive = { 0.96, 0.95, 0.88, 1.0 };
BlitBox::Color BlitBox::edgeDarkActive = { 0.85, 0.81, 0.64, 1.0 };
BlitBox::Color BlitBox::backgroundLightActive = { 0.95, 0.93, 0.85, 1.0 };
BlitBox::Color BlitBox::backgroundDarkActive = { 0.86, 0.82, 0.65, 1.0 };

unsigned short BlitBox::symbols[3][6][6] = {
  {{ 1, 1, 0, 0, 1, 1 },
   { 1, 1, 1, 1, 1, 1 },
   { 0, 1, 1, 1, 1, 0 },
   { 0, 1, 1, 1, 1, 0 },
   { 1, 1, 1, 1, 1, 1 },
   { 1, 1, 0, 0, 1, 1 }},

  {{ 1, 1, 1, 1, 1, 1 },
   { 1, 1, 1, 1, 1, 1 },
   { 1, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 1 },
   { 1, 1, 1, 1, 1, 1 }},

  {{ 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0 },
   { 1, 1, 1, 1, 1, 0 },
   { 1, 1, 1, 1, 1, 0 }}
};

BlitBox::BlitBox(Manager *manager, int screen) {
  m_pixmap = 0;
  m_xftDraw = 0;
  m_manager = manager;
  m_screen = screen;
}

BlitBox::~BlitBox() {
  if (m_pixmap)
    XFreePixmap(m_manager->getDisplay(), m_pixmap);
}

void BlitBox::drawBlendH(int x, int y, int w, int h, Color *start, Color *end) {
  Color blend;
  for (int pos = 0; pos < w; pos++) {
    blend.r = start->r + (end->r - start->r) * (double)pos / w;
    blend.g = start->g + (end->g - start->g) * (double)pos / w;
    blend.b = start->b + (end->b - start->b) * (double)pos / w;
    blend.a = start->a + (end->a - start->a) * (double)pos / w;
    drawRectangle(x + pos, y, 1, h, &blend);
  }
}

void BlitBox::drawBlendV(int x, int y, int w, int h, Color *start, Color *end) {
  Color blend;
  for (int pos = 0; pos < h; pos++) {
    blend.r = start->r + (end->r - start->r) * (double)pos / h;
    blend.g = start->g + (end->g - start->g) * (double)pos / h;
    blend.b = start->b + (end->b - start->b) * (double)pos / h;
    blend.a = start->a + (end->a - start->a) * (double)pos / h;
    drawRectangle(x, y + pos, w, 1, &blend);
  }
}

void BlitBox::drawRectangle(int x, int y, int w, int h, Color *color) {
  XRenderColor renderColor;
  renderColor.red   = (unsigned short) (color->r * 65535.0);
  renderColor.green = (unsigned short) (color->g * 65535.0);
  renderColor.blue  = (unsigned short) (color->b * 65535.0);
  renderColor.alpha = (unsigned short) (color->a * 65535.0);

  XftColor xftColor;
  XftColorAllocValue(m_manager->getDisplay(),
    DefaultVisual(m_manager->getDisplay(), m_screen),
    DefaultColormap(m_manager->getDisplay(), m_screen), &renderColor,
    &xftColor);

  XftDrawRect(m_xftDraw, &xftColor, x, y, w, h);

  XftColorFree(m_manager->getDisplay(), DefaultVisual(m_manager->getDisplay(),
    m_screen), DefaultColormap(m_manager->getDisplay(), m_screen), &xftColor);
}

void BlitBox::drawOpenRectangle(int x, int y, int w, int h, Color *color) {
  drawRectangle(x, y, w, 1, color);
  drawRectangle(x, y, 1, h, color);
  drawRectangle(x + 1, y + h - 1, w - 1, 1, color);
  drawRectangle(x + w - 1, y + 1, 1, h - 2, color);
}

void BlitBox::drawText(int x, int y, const char *text, size_t length,
  Color *color)
{
  if (!m_xftFont) {
    m_xftFont = XftFontOpen(m_manager->getDisplay(), 0, XFT_FAMILY,
      XftTypeString, "Verdana", XFT_SIZE, XftTypeInteger, 8, 0);
  }

  XRenderColor renderColor;
  renderColor.red   = (unsigned short) (color->r * 65535.0);
  renderColor.green = (unsigned short) (color->g * 65535.0);
  renderColor.blue  = (unsigned short) (color->b * 65535.0);
  renderColor.alpha = (unsigned short) (color->a * 65535.0);

  XftColor xftColor;
  XftColorAllocValue(m_manager->getDisplay(),
    DefaultVisual(m_manager->getDisplay(), m_screen),
    DefaultColormap(m_manager->getDisplay(), m_screen), &renderColor,
    &xftColor);

  XftDrawString8(m_xftDraw, &xftColor, m_xftFont, x, y, (XftChar8 *)text,
    length);

  XftColorFree(m_manager->getDisplay(), DefaultVisual(m_manager->getDisplay(),
    m_screen), DefaultColormap(m_manager->getDisplay(), m_screen), &xftColor);
}

void BlitBox::getTextExtents(XGlyphInfo *xgi, const char *text, size_t length) {
  XftTextExtents8(m_manager->getDisplay(), m_xftFont, (XftChar8 *)text, length,
    xgi);
}

void BlitBox::copyArea(int sx, int sy, int w, int h, Drawable d, int dx,
  int dy)
{
  XCopyArea(m_manager->getDisplay(), m_pixmap, d,
    DefaultGC(m_manager->getDisplay(), m_screen), sx, sy, w, h, dx, dy);
}

void BlitBox::copyArea(int sx, int sy, int w, int h, BlitBox *d, int dx,
  int dy)
{
  copyArea(sx, sy, w, h, d->m_pixmap, dx, dy);
}

void BlitBox::copyAreaBack(int sx, int sy, int w, int h, Drawable d, int dx,
  int dy)
{
  XCopyArea(m_manager->getDisplay(), d, m_pixmap,
    DefaultGC(m_manager->getDisplay(), m_screen), sx, sy, w, h, dx, dy);
}

void BlitBox::configure(int w, int h) {
  if (m_pixmap) {
    XftDrawDestroy(m_xftDraw);
    XFreePixmap(m_manager->getDisplay(), m_pixmap);
  }

  if (w <= 0 || h <= 0) {
    m_pixmap = 0;
    return;
  }

  m_pixmap = XCreatePixmap(m_manager->getDisplay(),
    m_manager->getRootWindow(m_screen), w, h,
    DefaultDepth(m_manager->getDisplay(), m_screen));

  m_xftDraw = XftDrawCreate(m_manager->getDisplay(), m_pixmap,
    DefaultVisual(m_manager->getDisplay(), m_screen),
    DefaultColormap(m_manager->getDisplay(), m_screen));
}

void BlitBox::renderFrame(BlitBox *left, BlitBox *top, BlitBox *right,
  BlitBox *bottom, int width, int height, int leftWidth, int topWidth,
  int rightWidth, int bottomWidth, char *title, bool active)
{
  top->configure(width, topWidth);
  bottom->configure(width, bottomWidth);

  top->drawOpenRectangle(0, 0, width, topWidth, &black);
  top->drawBlendV(2, 2, width - 4, topWidth - 2,
    active ? &backgroundDarkActive : &backgroundDark,
    active ? &backgroundLightActive : &backgroundLight);
  top->drawRectangle(1, 1, 1, topWidth - 1,
    active ? &edgeLightActive : &edgeLight);
  top->drawRectangle(2, 1, width - 3, 1,
    active ? &edgeLightActive : &edgeLight);
  top->drawRectangle(width - 2, 2, 1, topWidth - 2,
    active ? &edgeDarkActive : &edgeDark);
  top->drawRectangle(leftWidth - 2, topWidth - 1, 1, 1,
    active ? &edgeDarkActive : &edgeDark);
  top->drawRectangle(leftWidth - 2, topWidth - 2,
    width - leftWidth - rightWidth + 4, 1,
    active ? &edgeDarkActive : &edgeDark);
  top->drawRectangle(width - rightWidth + 1, topWidth - 1, 1, 1,
    active ? &edgeLightActive : &edgeLight);
  top->drawRectangle(leftWidth - 1, topWidth - 1,
    width - leftWidth - rightWidth + 2, 1, &black);

  if (title)
    top->drawText(19, 13, title, strlen(title), &black);

  for (int b = 0; b < 3; b++) {
    if (active) {
      Client::m_buttonUpA[b]->copyArea(0, 0, 12, 12, top,
        width - rightWidth - 11 - 14 * b, 3);
    } else {
      Client::m_buttonUpI[b]->copyArea(0, 0, 12, 12, top,
        width - rightWidth - 11 - 14 * b, 3);
    }
  }

  bottom->drawOpenRectangle(0, 0, width, bottomWidth, &black);
  bottom->drawRectangle(2, 0, width - 4, bottomWidth - 2,
    active ? &backgroundLightActive : &backgroundLight);
  bottom->drawRectangle(1, 0, 1, bottomWidth - 1,
    active ? &edgeLightActive : &edgeLight);
  bottom->drawRectangle(2, bottomWidth - 2, width - 3, 1,
    active ? &edgeDarkActive : &edgeDark);
  bottom->drawRectangle(width - 2, 0, 1, bottomWidth - 2,
    active ? &edgeDarkActive : &edgeDark);
  bottom->drawRectangle(leftWidth - 2, 0, 1, 2,
    active ? &edgeDarkActive : &edgeDark);
  bottom->drawRectangle(leftWidth - 1, 1, width - leftWidth - rightWidth + 2,
    1, active ? &edgeLightActive : &edgeLight);
  bottom->drawRectangle(width - rightWidth + 1, 0, 1, 2,
    active ? &edgeLightActive : &edgeLight);
  bottom->drawRectangle(leftWidth - 1, 0, width - leftWidth - rightWidth + 2,
    1, &black);

  left->configure(leftWidth, height - topWidth - bottomWidth);
  right->configure(rightWidth, height - topWidth - bottomWidth);

  left->drawRectangle(0, 0, 1, height - topWidth - bottomWidth, &black);
  left->drawRectangle(leftWidth - 1, 0, 1, height - topWidth - bottomWidth,
    &black);
  left->drawRectangle(leftWidth - 2, 0, 1, height - topWidth - bottomWidth,
    active ? &edgeDarkActive : &edgeDark);
  left->drawRectangle(1, 0, 1, height - topWidth - bottomWidth,
    active ? &edgeLightActive : &edgeLight);
  left->drawRectangle(2, 0, leftWidth - 4, height - topWidth - bottomWidth,
    active ? &backgroundLightActive : &backgroundLight);

  right->drawRectangle(0, 0, 1, height - topWidth - bottomWidth, &black);
  right->drawRectangle(rightWidth - 1, 0, 1, height - topWidth - bottomWidth,
    &black);
  right->drawRectangle(rightWidth - 2, 0, 1, height - topWidth - bottomWidth,
    active ? &edgeDarkActive : &edgeDark);
  right->drawRectangle(1, 0, 1, height - topWidth - bottomWidth,
    active ? &edgeLightActive : &edgeLight);
  right->drawRectangle(2, 0, rightWidth - 4, height - topWidth - bottomWidth,
    active ? &backgroundLightActive : &backgroundLight);
}

void BlitBox::makeButton(int button, bool pressed, BlitBox *blitBox,
  bool active)
{
  blitBox->configure(12, 12);
  blitBox->drawOpenRectangle(0, 0, 12, 12, active ? &darkActive : &dark);
  blitBox->drawOpenRectangle(1, 1, 10, 10, pressed ?
    (active ? &edgeLightActive : &edgeLight) :
    (active ? &edgeDarkActive : &edgeDark));
  blitBox->drawRectangle(1, 1, 10, 1, pressed ?
    (active ? &edgeDarkActive : &edgeDark) :
    (active ? &edgeLightActive : &edgeLight));
  blitBox->drawRectangle(1, 2, 1, 9, pressed ?
    (active ? &edgeDarkActive : &edgeDark) :
    (active ? &edgeLightActive : &edgeLight));
  if (pressed) {
    blitBox->drawBlendV(2, 2, 8, 8,
      active ? &backgroundLightActive : &backgroundLight,
      active ? &backgroundDarkActive : &backgroundDark);
  } else {
    blitBox->drawBlendV(2, 2, 8, 8,
      active ? &backgroundDarkActive : &backgroundDark,
      active ? &backgroundLightActive : &backgroundLight);
  }

  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < 6; x++) {
      if (symbols[button][y][x] > 0) {
        blitBox->drawRectangle(3 + x, 3 + y, 1, 1, &black);
      }
    }
  }
}

void BlitBox::renderInfoBox(int x, int y, int w, int h, char *text) {
  drawOpenRectangle(0, 0, w, h, &black);
  drawOpenRectangle(1, 1, w - 2, h - 2, &edgeDarkActive);
  drawRectangle(1, 1, 1, h - 2, &edgeLightActive);
  drawRectangle(2, 1, w - 3, 1, &edgeLightActive);
  drawBlendV(2, 2, w - 4, h - 4, &backgroundDarkActive, &backgroundLightActive);
  drawText(5 + x, 5 + y, text, strlen(text), &black);
}
