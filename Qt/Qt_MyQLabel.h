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

#ifndef QT_MYQLABEL_H
#define QT_MYQLABEL_H

#include <QLabel>
#include <QPainter>

#include "Qt_sequencer_proc.h"


struct MyQLabel : public QLabel {

  bool _use_custom_painter = false;
  bool _vertical_text = false;
  
  MyQLabel(QWidget * parent = 0)
    : QLabel(parent)
  {}
  
  MyQLabel ( const QString & text, QWidget * parent = 0)
    : QLabel(text, parent)
  {}

  void setVerticalText(){
    _vertical_text = true;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    updateGeometry();
    update();
  }

  QSize sizeHint() const override {
    if (_vertical_text)
      return QSize(fontMetrics().height(), 0);
    return QLabel::sizeHint();
  }

  QSize minimumSizeHint() const override {
    if (_vertical_text)
      return sizeHint();
    return QLabel::minimumSizeHint();
  }
  
  void paintEvent ( QPaintEvent * ev ) override {
    TRACK_PAINT();

    if (_vertical_text) {
      QPainter p(this);
      p.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,
                               QPalette::WindowText));
      p.translate(width() / 2.0, height() / 2.0);
      p.rotate(-90);
      const QFontMetrics fm(font());
      const int text_width = fm.horizontalAdvance(text());
      // Fit the rotated text to the existing bar height without adding a
      // vertical size requirement to its layout.
      const qreal scale = qMin(qreal(1), qMin(
        qreal(height()) / qMax(1, text_width),
        qreal(width()) / qMax(1, fm.height())));
      p.scale(scale, scale);
      p.drawText(QRectF(-text_width / 2.0, -fm.height() / 2.0,
                       text_width, fm.height()), Qt::AlignCenter, text());
      return;
    }
      
    if (!_use_custom_painter) {

      QLabel::paintEvent(ev);
      
    } else {
      
      QPainter p(this);
      
      p.setRenderHints(QPainter::Antialiasing,true);
      
      myDrawText(p, rect(), text(), alignment(), false, 0, true, false);

    }
  }
};
  

#endif
