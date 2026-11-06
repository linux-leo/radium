#pragma once

#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QTextLayout>

// Rotate only the text surface, retaining the original editor's validation,
// keyboard handling, signals, wheel handling and context menu.
class VerticalLineEdit : public QObject {
  QLineEdit *_edit;
  int _selection_anchor = 0;
  int _horizontal_above = 0;
  QWidget *_width_owner = nullptr;
  int _vertical_width = 0;
  bool _horizontal = false;

  void updatePresentation() {
    if (_width_owner) {
      _horizontal = _edit->text().toLongLong() > _horizontal_above;
      _width_owner->setFixedWidth(_horizontal
        ? qMax(_vertical_width, _edit->fontMetrics().horizontalAdvance(_edit->text()) + 12)
        : _vertical_width);
    }
    _edit->update();
  }

  qreal textScale(qreal text_width, qreal text_height) const {
    return qMin(qreal(1), qMin(qMax(1, _edit->height() - 4) / qMax(qreal(1), text_width),
                              qMax(1, _edit->width() - 4) / qMax(qreal(1), text_height)));
  }

  bool eventFilter(QObject *object, QEvent *event) override {
    if (object != _edit || _horizontal)
      return false;

    const auto type = event->type();
    if (type == QEvent::FocusIn || type == QEvent::FocusOut) {
      _edit->update();
      return false;
    }
    if (type != QEvent::Paint && type != QEvent::MouseButtonPress
        && type != QEvent::MouseMove && type != QEvent::MouseButtonDblClick)
      return false;

    QTextLayout layout(_edit->displayText(), _edit->font());
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(100000);
    layout.endLayout();
    const qreal text_width = line.naturalTextWidth();
    const qreal text_height = line.height();
    const qreal scale = textScale(text_width + 2, text_height);

    if (type != QEvent::Paint) {
      auto *mouse = static_cast<QMouseEvent *>(event);
      if (mouse->button() != Qt::LeftButton)
        return false;
      _edit->setFocus(Qt::MouseFocusReason);
      _edit->selectAll();
      _edit->update();
      return true;
    }
    QPainter painter(_edit);
    const auto group = !_edit->isEnabled() ? QPalette::Disabled
                     : _edit->isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    painter.fillRect(_edit->rect(), _edit->palette().brush(group, QPalette::Base));
    painter.setPen(_edit->palette().color(group, QPalette::Text));
    painter.translate(_edit->width() / 2.0, _edit->height() / 2.0);
    painter.rotate(-90);
    painter.scale(scale, scale);

    QList<QTextLayout::FormatRange> selections;
    if (_edit->hasSelectedText()) {
      QTextLayout::FormatRange selection;
      selection.start = _edit->selectionStart();
      selection.length = _edit->selectedText().size();
      selection.format.setBackground(_edit->palette().brush(group, QPalette::Highlight));
      selection.format.setForeground(_edit->palette().brush(group, QPalette::HighlightedText));
      selections.append(selection);
    }
    const QPointF origin(-text_width / 2.0, -text_height / 2.0);
    layout.draw(&painter, origin, selections);
    if (_edit->hasFocus() && !_edit->isReadOnly())
      layout.drawCursor(&painter, origin, _edit->cursorPosition());
    return true;
  }

public:
  void refreshPresentation() {
    updatePresentation();
  }

  void setHorizontalAbove(int threshold, QWidget *width_owner) {
    _horizontal_above = threshold;
    _width_owner = width_owner;
    _vertical_width = width_owner->width();
    updatePresentation();
  }

  explicit VerticalLineEdit(QLineEdit *edit) : QObject(edit), _edit(edit) {
    edit->installEventFilter(this);
    connect(edit, &QLineEdit::textChanged, this, [this]{ updatePresentation(); });
    connect(edit, &QLineEdit::selectionChanged, this, [this]{ _edit->update(); });
    connect(edit, &QLineEdit::cursorPositionChanged, this, [this]{ _edit->update(); });
  }
};
