#pragma once

#include <QLineEdit>
#include <QSignalBlocker>
#include <limits>

#include "Qt_MyQSpinBox.h"
#include "../common/undo.h"

class HistorySpinBox : public MyQSpinBox {
  bool _can_redo = false;

  void clearValueBackground() {
    QPalette palette = lineEdit()->palette();
    if (palette.color(QPalette::Base) != QColor(Qt::transparent)) {
      palette.setColor(QPalette::Base, Qt::transparent);
      lineEdit()->setPalette(palette);
    }
  }

public:
  QSize sizeHint() const override {
    QSize hint = MyQSpinBox::sizeHint();
    QStyleOptionSpinBox option;
    initStyleOption(&option);
    const QRect input = style()->subControlRect(QStyle::CC_SpinBox, &option,
                                               QStyle::SC_SpinBoxEditField, this);
    // Always reserve three digits and the unsaved marker, regardless of the
    // current undo/redo range. A possible 1000th redo must not widen 999 edits.
    const QString display = textFromValue(qMax(999, value())) + "*";
    hint.setWidth(fontMetrics().horizontalAdvance(display) + 8 + width() - input.width());
    return hint;
  }

  QSize minimumSizeHint() const override {
    return sizeHint();
  }

  explicit HistorySpinBox(QWidget *parent = nullptr) : MyQSpinBox(parent) {
    // History is an action control, not an editable field. Preserve editor focus
    // for both arrow clicks and clicks on the displayed count.
    setFocusPolicy(Qt::NoFocus);
    lineEdit()->setFocusPolicy(Qt::NoFocus);
    lineEdit()->setReadOnly(true);
    lineEdit()->installEventFilter(this);
    clearValueBackground();
    setArrowHoverHighlight(true);
    setHistoryState(0, false);
  }

  void setHistoryState(int num_undos, bool can_redo, bool has_unsaved_undos = false) {
    const QSignalBlocker blocker(this);
    _can_redo = can_redo;
    setSuffix(has_unsaved_undos ? "*" : "");
    // Only the next redo position is known; the undo callback updates the range.
    setMaximum(num_undos < std::numeric_limits<int>::max() && can_redo
               ? num_undos + 1 : num_undos);
    setValue(num_undos);
    updateGeometry();
    update();
  }

protected:
  bool eventFilter(QObject *watched, QEvent *event) override {
    // Keep the read-only count transparent when the application theme changes.
    if (watched == lineEdit() && event->type() == QEvent::PaletteChange)
      clearValueBackground();
    return MyQSpinBox::eventFilter(watched, event);
  }

  StepEnabled stepEnabled() const override {
    StepEnabled enabled = StepNone;
    if (value() > 0)
      enabled |= StepDownEnabled;
    if (_can_redo)
      enabled |= StepUpEnabled;
    return enabled;
  }

  void stepBy(int steps) override {
    // Undo/Redo notify all bottom bars synchronously. Never change the count
    // independently of the history, including when an operation cannot run.
    while (steps != 0) {
      const int previous = value();
      if (steps > 0 && _can_redo) {
        Redo();
        --steps;
      } else if (steps < 0 && value() > 0) {
        Undo();
        ++steps;
      } else {
        break;
      }
      if (value() == previous)
        break;
    }
  }
};
