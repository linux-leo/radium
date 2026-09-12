/* Copyright 2012 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

#ifndef QT_MIX_COLORS_H
#define QT_MIX_COLORS_H

#include <math.h>

#include <QColor>

static inline QColor mix_colors(const QColor &c1, const QColor &c2, float how_much){

  float a1 = how_much;
  float a2 = 1.0f-a1;

  float red1 = c1.redF();
  float green1 = c1.greenF();
  float blue1 = c1.blueF();
  float alpha1 = c1.alphaF();

  float red2 = c2.redF();
  float green2 = c2.greenF();
  float blue2 = c2.blueF();
  float alpha2 = c2.alphaF();

  float a = a1 * alpha1 + a2 * alpha2;

  float r = sqrtf(red1*red1*a1 + red2*red2*a2);
  float g = sqrtf(green1*green1*a1 + green2*green2*a2);
  float b = sqrtf(blue1*blue1*a1 + blue2*blue2*a2);

  return QColor::fromRgbF(r,g,b,a);
}

#endif
