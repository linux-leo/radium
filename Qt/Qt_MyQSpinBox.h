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

#pragma once

#include <QSpinBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionSpinBox>

#include "../audio/undo_audio_effect_proc.h"
#include "../common/undo_patchvoice_proc.h"

#include "helpers.h"

#include "../OpenGL/Widget_proc.h"


extern void set_editor_focus(void);

  
struct MyQSpinBox : public GL_PauseCaller, public QSpinBox{
  bool _arrow_focus_enabled = true;

  void setArrowFocusEnabled(bool enabled) {
    _arrow_focus_enabled = enabled;
    setFocusPolicy(enabled ? Qt::StrongFocus : Qt::TabFocus);
    lineEdit()->installEventFilter(this);
  }

  bool eventFilter(QObject *watched, QEvent *event) override {
    // TabFocus prevents the arrow buttons from taking keyboard focus. Allow
    // explicit clicks in the number field to start editing as usual.
    if (!_arrow_focus_enabled && watched == lineEdit()
        && event->type() == QEvent::MouseButtonPress && !lineEdit()->isReadOnly())
      setFocus(Qt::MouseFocusReason);
    return QSpinBox::eventFilter(watched, event);
  }

  bool _highlight_arrow_hover = false;

  void setArrowHoverHighlight(bool enabled) {
    _highlight_arrow_hover = enabled;
    setAttribute(Qt::WA_Hover, enabled);
    update();
  }

  void initStyleOption(QStyleOptionSpinBox *option) const override {
    QSpinBox::initStyleOption(option);
    // Keep the flat arrows borderless while pressed; Qt still handles stepping.
    if (_highlight_arrow_hover && !hasFrame())
      option->state &= ~QStyle::State_Sunken;
  }

  void paintEvent(QPaintEvent *event) override {
    if (_highlight_arrow_hover && isEnabled()) {
      QStyleOptionSpinBox option;
      initStyleOption(&option);
      const auto control = option.activeSubControls;
      const bool up = control == QStyle::SC_SpinBoxUp;
      const bool down = control == QStyle::SC_SpinBoxDown;
      if ((option.state & QStyle::State_MouseOver) && (up || down)
          && (option.stepEnabled & (up ? StepUpEnabled : StepDownEnabled))) {
        QPainter painter(this);
        // Match draw-button's 1.25 hover lightening in bin/scheme/gui.scm.
        painter.fillRect(style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                up ? QStyle::SC_SpinBoxUp : QStyle::SC_SpinBoxDown, this),
                         palette().color(QPalette::Window).lighter(125));
      }
    }
    QSpinBox::paintEvent(event);
  }

  bool _has_mouse;
  radium::GcHolder<struct Patch> _patch;
  int _effect_num;
  bool _is_patchvoice_spinbox;

  QString _statusbar_text;
  
  void init(){
    _has_mouse=false;
    _patch.set(NULL);
    _effect_num = 0;
    _is_patchvoice_spinbox = false;
  }

  std::function<void(void)> _show_popup_menu;
  
  MyQSpinBox ( QWidget * parent = 0 ) : QSpinBox(parent) {init();}

  void focusInEvent ( QFocusEvent *e ) override {                                 
    printf("Got focusInEvent\n");
    obtain_keyboard_focus();
    GL_lock();
    QSpinBox::focusInEvent(e);                                             
    GL_unlock();
  }                                                                     
  void focusOutEvent ( QFocusEvent *e ) override {                                
    printf("Got focusOutEvent\n");
    release_keyboard_focus();
    GL_lock();
    QSpinBox::focusOutEvent(e);                                            
    GL_unlock();
  }                                                                     
  void hideEvent ( QHideEvent *e ) override {                                
    //printf("Got hideEvent\n");
    release_keyboard_focus();
    QSpinBox::hideEvent(e);                                            
  }                                                                     
  void 	wheelEvent ( QWheelEvent * event ) override {
    printf("Got wheel event\n");
    QSpinBox::wheelEvent(event);
    set_editor_focus();
  }

  bool _is_hovered = false;
      
  bool _popup_menu_is_visible = false;

  void enterEvent(QEnterEvent *event) override {
    if (_statusbar_text != "")
      GFX_SetStatusBar(_statusbar_text.toUtf8().constData());
  }

  void leaveEvent(QEvent *event) override {
    if (_statusbar_text != "")
      GFX_SetStatusBar("");
  }

  /*
  void enterEvent(QEvent *event) override {
    if (_patch.data() != NULL){
      _is_hovered = true;
      update();
      GFX_SetStatusBar(talloc_format("\"%s\" (right-click for options)", getInstrumentEffectName(_effect_num, _patch->id)));
    }
  }

  void leaveEvent(QEvent *event) override {
    if (_patch.data() != NULL){
      if(_popup_menu_is_visible==false){
        _is_hovered = false;
        update();
      }
      GFX_SetStatusBar("");
    }
  }
  */
  
  void contextMenuEvent(QContextMenuEvent *event) override {
    if (_show_popup_menu) {
      _show_popup_menu();
    } else
      QSpinBox::contextMenuEvent(event);
  }
  
  // Is this method ever called? It doesn't seems so. Maybe it's only used if the arrows are clicked?
#if 0
  void mousePressEvent ( QMouseEvent * event ) override
  {
    if (event->button() == Qt::LeftButton){      
      //setSliderDown(true);    
      if(_is_patchvoice_spinbox==true)
        ADD_UNDO(PatchVoice_CurrPos(_patch.data(),_effect_num)); // <- BUG (don't send _effect_num)

      printf("Got it %p %d\n",_patch.data(),_effect_num);
      //setChecked(!isChecked());
    }

    QSpinBox::mousePressEvent(event);
  }
#endif
  
#if 0
  void paintEvent ( QPaintEvent * ev ) override {
    TRACK_PAINT();
    
    EditorWidget *editor = static_cast<EditorWidget*>(root->song->tracker_windows->os_visual.widget);

    QColor col1; // off
    QColor col2; // on

    int col1num = 11;
    int col2num = 10;

    if(isEnabled()){
      col1 = editor->colors[col1num].light(90);
      col2 = editor->colors[col2num].light(82);
    }else{
      col1 = editor->colors[col1num].light(116);
      col2 = editor->colors[col2num].light(110);
    }

    //col2 = QColor(106, 104, 100, 255);
    //col2 = QColor(0, 107, 156, 255);

    QPainter p(this);

    if(isChecked()){
      p.fillRect(1,1,width()-2,height()-2,col2);
      p.setPen(editor->colors[1]);
      //p.drawRect(0,0,width()-1,height()-1);
      //p.drawRect(1,1,width()-3,height()-3);
    }else{
      p.setPen(col2);
      p.drawRect(1,1,width()-3,height()-3);
      p.drawRect(2,2,width()-5,height()-5);
      //p.fillRect(0,0,width(),height(),col1);
    }
  }
#endif
};
