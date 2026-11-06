#pragma once

#include <QDoubleValidator>
#include <QApplication>
#include "Qt_MyQSpinBox.h"
#include "Qt_colors_proc.h"

// Keep the existing 0..10000 velocity units while editing percentages.
class VelocitySpinBox : public MyQSpinBox {
  bool _drag_pending = false;
  bool _dragging = false;
  QPointF _drag_origin;
  int _drag_value = 0;
  bool _slider_hovered = false;

  void clearValueBackground() {
    QPalette colors = lineEdit()->palette();
    if (colors.color(QPalette::Base) != QColor(Qt::transparent)) {
      colors.setColor(QPalette::Base, Qt::transparent);
      lineEdit()->setPalette(colors);
    }
  }

  QString percentageText(QString text) const {
    if (text.startsWith(prefix()))
      text.remove(0, prefix().size());
    if (text.endsWith(suffix()))
      text.chop(suffix().size());
    return text.trimmed();
  }

protected:
  virtual void beginSliderDrag() {}

  virtual int valueForDrag(qreal scaled_value) const {
    return qRound(scaled_value / 100.0) * 100;
  }

  QString textFromValue(int value) const override {
    return locale().toString(qRound(value / 100.0));
  }

  int valueFromText(const QString &text) const override {
    return qRound(locale().toDouble(percentageText(text))) * 100;
  }

  QValidator::State validate(QString &text, int &position) const override {
    QString number = percentageText(text);
    int number_position = qBound(0, position - int(prefix().size()), int(number.size()));
    QDoubleValidator validator(minimum() / 100.0, maximum() / 100.0, 2);
    validator.setLocale(locale());
    validator.setNotation(QDoubleValidator::StandardNotation);
    return validator.validate(number, number_position);
  }

  bool eventFilter(QObject *watched, QEvent *event) override {
    if (watched == lineEdit()) {
      if (event->type() == QEvent::PaletteChange)
        clearValueBackground();
      if (isEnabled() && !lineEdit()->isReadOnly()) {
        if (event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::MouseButtonDblClick) {
          auto *mouse = static_cast<QMouseEvent *>(event);
          _drag_pending = mouse->button() == Qt::LeftButton;
          _dragging = false;
          if (_drag_pending) {
            lineEdit()->setCursor(Qt::ArrowCursor);
            clearFocus();
            lineEdit()->deselect();
            _drag_origin = mouse->globalPosition();
            _drag_value = value();
            // Wait for release before giving the number field focus.
            return true;
          }
        } else if (event->type() == QEvent::MouseMove && _drag_pending) {
          auto *mouse = static_cast<QMouseEvent *>(event);
          if (mouse->buttons() & Qt::LeftButton) {
            const qreal distance = mouse->globalPosition().x() - _drag_origin.x();
            if (!_dragging && qAbs(distance) >= QApplication::startDragDistance()) {
              _dragging = true;
              beginSliderDrag();
            }
            if (_dragging) {
              lineEdit()->deselect();
              // The fill always represents 0..10000, even when DV narrows
              // the editable range. A full field-width drag spans that scale.
              const qreal next = _drag_value + distance * 10000.0 / qMax(1, lineEdit()->width());
              setValue(qBound(minimum(), valueForDrag(
                qBound(qreal(minimum()), next, qreal(maximum()))), maximum()));
              return true;
            }
            return true;
          } else {
            _drag_pending = _dragging = false;
          }
        } else if (event->type() == QEvent::MouseButtonRelease) {
          auto *mouse = static_cast<QMouseEvent *>(event);
          if (mouse->button() == Qt::LeftButton && _drag_pending) {
            if (_dragging) {
              set_editor_focus();
              _drag_pending = _dragging = false;
              return true;
            }
            _drag_pending = _dragging = false;
            setFocus(Qt::MouseFocusReason);
            lineEdit()->setCursor(Qt::IBeamCursor);
            lineEdit()->selectAll();
            return true;
          }
        }
      }
    }
    return MyQSpinBox::eventFilter(watched, event);
  }

  void focusOutEvent(QFocusEvent *event) override {
    MyQSpinBox::focusOutEvent(event);
    lineEdit()->deselect();
    lineEdit()->setCursor(Qt::ArrowCursor);
  }

  void wheelEvent(QWheelEvent *event) override {
    MyQSpinBox::wheelEvent(event);
    // Stepping can select the value even when the editor already has focus,
    // so there may be no focus-out event to clear it afterward.
    lineEdit()->deselect();
    lineEdit()->setCursor(Qt::ArrowCursor);
  }

  void enterEvent(QEnterEvent *event) override {
    MyQSpinBox::enterEvent(event);
    _slider_hovered = true;
    update();
  }

  void leaveEvent(QEvent *event) override {
    MyQSpinBox::leaveEvent(event);
    _slider_hovered = false;
    update();
  }

  void paintEvent(QPaintEvent *event) override {
    MyQSpinBox::paintEvent(event);
    QStyleOptionSpinBox option;
    initStyleOption(&option);
    const QRect input = style()->subControlRect(QStyle::CC_SpinBox, &option,
                                               QStyle::SC_SpinBoxEditField, this);
    QPainter painter(this);
    const bool highlighted = _slider_hovered && isEnabled();
    painter.fillRect(input, highlighted
      ? palette().color(backgroundRole()).lighter(110)
      : get_qcolor(SLIDER_BACKGROUND_COLOR_NUM));
    QLinearGradient gradient(input.topLeft(),
                             QPointF(input.right(), input.top() + input.height() * 0.75));
    QColor first = get_qcolor(isEnabled() ? SLIDER1_COLOR_NUM : SLIDER_DISABLED_COLOR_NUM);
    QColor second = get_qcolor(isEnabled() ? SLIDER2_COLOR_NUM : SLIDER_DISABLED_COLOR_NUM);
    if (highlighted) {
      first = first.lighter(150);
      second = second.lighter(150);
    }
    gradient.setColorAt(0, first);
    gradient.setColorAt(1, second);
    QRect level = input;
    level.setWidth(qRound(input.width() * value() / 10000.0));
    painter.fillRect(level, gradient);
  }

public:
  explicit VelocitySpinBox(QWidget *parent = nullptr) : MyQSpinBox(parent) {
    setRange(0, 10000);
    setSingleStep(100);
    setFrame(false);
    setAlignment(Qt::AlignCenter);
    setArrowHoverHighlight(true);
    setArrowFocusEnabled(false);
    lineEdit()->setCursor(Qt::ArrowCursor);
    clearValueBackground();
    connect(lineEdit(), &QLineEdit::selectionChanged, this, [this] {
      if (!hasFocus() && !lineEdit()->hasFocus() && lineEdit()->hasSelectedText())
        lineEdit()->deselect();
    });
    connect(this, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int){ update(); });
  }
};
