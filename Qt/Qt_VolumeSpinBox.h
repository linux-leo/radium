#pragma once

#include "Qt_VelocitySpinBox.h"
#include "Qt_MyQSlider.h"
#include <QScopedValueRollback>
#include <QTimer>
#include "../common/undo.h"

// The spinbox edits the same scaled volume as the former slider. Its hidden
// slider supplies the existing peak and automation painter behind the input.
class VolumeSpinBox : public VelocitySpinBox {
  MyQSlider *_meter;
  QTimer _wheel_idle;
  bool _handling_wheel = false;
  bool _wheel_undo_recorded = false;
  int64_t _wheel_undo_generation = -1;
  int _wheel_undo_position = -1;
  struct Patch *_wheel_patch = nullptr;

  void endWheelGesture() {
    _wheel_idle.stop();
    _wheel_undo_recorded = false;
  }

  void addVolumeUndo() {
    if (_patch.data())
      ADD_UNDO(AudioEffect_CurrPos(_patch.data(), _effect_num, AE_NO_FLAGS));
  }

  QString numberText(QString text) const {
    if (!prefix().isEmpty() && text.startsWith(prefix()))
      text.remove(0, prefix().size());
    return text.trimmed();
  }

protected:
  void beginSliderDrag() override {
    endWheelGesture();
    addVolumeUndo();
  }

  int valueForDrag(qreal scaled_value) const override {
    return qRound(scaled_value);
  }

  QString textFromValue(int value) const override {
    if (value == 0)
      return "-inf";
    const double db = MIN_DB + value * (MAX_DB - MIN_DB) / 10000.0;
    return locale().toString(qAbs(db) < 0.05 ? 0.0 : db, 'f', 1);
  }

  int valueFromText(const QString &text) const override {
    if (numberText(text) == "-inf")
      return 0;
    return qRound((locale().toDouble(numberText(text)) - MIN_DB) * 10000.0 / (MAX_DB - MIN_DB));
  }

  QValidator::State validate(QString &text, int &position) const override {
    QString number = numberText(text);
    if (number == "-inf")
      return QValidator::Acceptable;
    if (!number.isEmpty() && QString("-inf").startsWith(number))
      return QValidator::Intermediate;
    QDoubleValidator validator(MIN_DB, MAX_DB, 1);
    validator.setLocale(locale());
    validator.setNotation(QDoubleValidator::StandardNotation);
    int number_position = qBound(0, position - int(prefix().size()), int(number.size()));
    return validator.validate(number, number_position);
  }

  void stepBy(int steps) override {
    const int next = qBound(minimum(),
      value() + qRound(steps * 10000.0 / (MAX_DB - MIN_DB)), maximum());
    if (next == value())
      return;

    if (_handling_wheel) {
      // Start another entry if an unrelated edit, undo, or plugin switch
      // occurred between scroll events; never fold it into the old gesture.
      if (!_wheel_undo_recorded || _wheel_undo_generation != g_curr_undo_generation
          || _wheel_undo_position != Undo_num_undos() || _wheel_patch != _patch.data()) {
        addVolumeUndo();
        _wheel_undo_recorded = true;
        _wheel_undo_generation = g_curr_undo_generation;
        _wheel_undo_position = Undo_num_undos();
        _wheel_patch = _patch.data();
      }
    } else {
      endWheelGesture();
      addVolumeUndo();
    }
    setValue(next);
  }

  void wheelEvent(QWheelEvent *event) override {
    if (event->phase() == Qt::ScrollBegin)
      endWheelGesture();
    if (event->phase() != Qt::NoScrollPhase)
      _wheel_idle.stop();
    {
      QScopedValueRollback<bool> handling(_handling_wheel, true);
      VelocitySpinBox::wheelEvent(event);
    }
    if (event->phase() == Qt::ScrollEnd)
      endWheelGesture();
    else if (event->phase() == Qt::NoScrollPhase)
      _wheel_idle.start();
  }

  void focusInEvent(QFocusEvent *event) override {
    endWheelGesture();
    addVolumeUndo();
    VelocitySpinBox::focusInEvent(event);
  }

  void paintEvent(QPaintEvent *event) override {
    VelocitySpinBox::paintEvent(event);
    QStyleOptionSpinBox option;
    initStyleOption(&option);
    const QRect input = style()->subControlRect(QStyle::CC_SpinBox, &option,
                                               QStyle::SC_SpinBoxEditField, this);
    _meter->resize(input.size());
    _meter->setValue(value());
    QPainter painter(this);
    painter.setClipRect(input);
    painter.translate(input.topLeft());
    SLIDERPAINTER_paint_indicators(_painter, &painter);
  }

public:
  SliderPainter *_painter;

  explicit VolumeSpinBox(QWidget *parent = nullptr) : VelocitySpinBox(parent) {
    _wheel_idle.setSingleShot(true);
    _wheel_idle.setInterval(400);
    connect(&_wheel_idle, &QTimer::timeout, this, [this]{ endWheelGesture(); });
    _meter = new MyQSlider(Qt::Horizontal, this);
    _meter->setRange(0, 10000);
    _meter->_minimum_size_set = true;
    _meter->hide();
    _painter = _meter->_painter;
    SLIDERPAINTER_set_string(_painter, "");
    _show_popup_menu = [this]{
      if (_patch.data() && _patch->patchdata) {
        auto *plugin = static_cast<SoundPlugin *>(_patch->patchdata);
        S7CALL2(dyn_dyn_charpointer, "FROM_C-show-effect-popup-menu",
                DYN_create_instrument(_patch->id), PLUGIN_get_effect_name(plugin, _effect_num));
      }
    };
    setAccessibleName("System volume (dB)");
    _statusbar_text = "System volume (dB). Click to edit; drag horizontally to adjust. Right-click for MIDI learn and automation options.";
  }

  void calledRegularlyByParent() {
    _meter->_patch.set(_patch.data());
    _meter->_effect_num = _effect_num;
    _meter->calledRegularlyByParent();
    update();
  }
};
