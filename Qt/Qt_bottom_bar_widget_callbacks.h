#pragma once
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


#include "../common/patch_proc.h"
#include "../common/instruments_proc.h"
#include "../common/undo.h"
#include "../common/OS_visual_input.h"
#include "../common/sequencer_proc.h"

#include "../audio/SoundPlugin.h"
#include "../audio/SoundPlugin_proc.h"
#include "../audio/SoundProducer_proc.h"
#include "../audio/CpuUsage.hpp"

#include "../Qt/Qt_MyQButton.h"
#include "../Qt/Qt_MyQSpinBox.h"
#include "Qt_VerticalLineEdit.h"
#include <QTimer>
#include <QToolTip>
#include <QStackedLayout>
#include <QButtonGroup>
#include <QSignalBlocker>

#ifdef USE_QT5
#include <QStyleFactory>
#else
#include <QCleanlooksStyle>
#endif

#include "FocusSniffers.h"

#include "Qt_instruments_proc.h"

#include "Qt_audio_instrument_widget.h"

#include "Qt_bottom_bar_widget.h"

#include "Qt_bottom_bar_widget_proc.h"

#include "Qt_PopupMenu_proc.h"



class Bottom_bar_widget;

//static QVector<Bottom_bar_widget*> g_bottom_bars;
static Ui::Audio_instrument_widget *g_system_audio_instrument_widget = NULL;
static struct Patch *g_system_out_patch = NULL;
static struct SoundPlugin *g_system_out_plugin = NULL;

extern bool drunk_velocity;

extern int scrolls_per_second;
extern int default_scrolls_per_second;

class Bottom_bar_widget : public QWidget, public Ui::Bottom_bar_widget {
  Q_OBJECT

  bool has_midi_learn = false;

  void frequent_timer_callback(void){

    if (_include_navigator && _navigator_widget==NULL) {
      
      _navigator_widget = SEQUENCER_create_navigator_widget();
      
      if (_navigator_widget !=NULL) {
        overview_group_layout->addWidget(_navigator_widget);
        _navigator_widget->show();
      }
      
    }


    if (this->edit_onoff->isChecked() != ATOMIC_GET(root->editonoff))
      this->edit_onoff->setChecked(ATOMIC_GET(root->editonoff));

    bool clickonoff = ATOMIC_GET_RELAXED(root->clickonoff);
    if (this->click_onoff->isChecked() != clickonoff)
      this->click_onoff->setChecked(clickonoff);
    
    update_playback_mode();

    if (g_system_out_plugin != NULL){
      const SoundPluginType *type = g_system_out_plugin->type;
      int effect_num = type->num_effects+EFFNUM_INPUT_VOLUME;
      float val = PLUGIN_get_effect_value(g_system_out_plugin, effect_num, VALUE_FROM_STORAGE);
      this->_triggered_by_user = false;
      this->system_volume_slider->setValue(val * 10000);
      this->_triggered_by_user = true;
        
      bool has_midi_learn_now = PLUGIN_has_midi_learn(g_system_out_plugin, effect_num);
      if (has_midi_learn_now != has_midi_learn){
        //printf("hepp %d\n",has_midi_learn_now);
        has_midi_learn = has_midi_learn_now;
        this->set_volume_slider_string();
        this->system_volume_slider->update();
      }
      
      //SLIDERPAINTER_set_recording_color(this->system_volume_slider->_painter, PLUGIN_is_recording_automation(g_system_out_plugin, effect_num));
    }

    this->system_volume_slider->_patch.set(g_system_out_patch);
    
    //SLIDERPAINTER_call_regularly(this->system_volume_slider->_painter);
  }
  
  struct Timer2 : public QTimer{
    
    void timerEvent(QTimerEvent * e) override {

      RETURN_IF_DATA_IS_INACCESSIBLE();

      if (g_system_out_patch==NULL && g_system_out_plugin != NULL)
        g_system_out_patch = (struct Patch*)g_system_out_plugin->patch;
      
      if (g_system_audio_instrument_widget == NULL && g_system_out_patch != NULL) {
        
        g_system_audio_instrument_widget = InstrumentWidget_get_audio_instrument_widget(g_system_out_patch);
        
        // Commented out. We poll the plugin instead. (see below)
        //if (g_system_audio_instrument_widget != NULL)
        //  this->system_volume_slider->setValue(g_system_audio_instrument_widget->input_volume_slider->value());
      }
    
      for(auto *bottom_bar_widget : g_bottom_bars)
        bottom_bar_widget->frequent_timer_callback();
    }
  };

 public:
  bool _initing;
  Timer2 _timer2;

  bool _triggered_by_user;
  bool _include_navigator;
  
  Bottom_bar_widget(QWidget *parent, bool include_navigator)
    : QWidget(parent)
    , _triggered_by_user(true)
    , _include_navigator(include_navigator)
  {
    _initing = true;
    setupUi(this);
#ifdef USE_QT5
#else
    setStyle(new QCleanlooksStyle());
#endif

    QFontMetrics fm(QApplication::font());
        
    setMinimumHeight(fm.height()*3/2);
    setMaximumHeight(fm.height()*3/2);
    for (auto *label : {signature_label, lpb_label, bpm_label, octave_label, volume_label})
      label->setVerticalText();
    // A control-relative gap, fixed across window resizes. Internal group
    // layouts retain their existing spacing and have no added margins.
    horizontalLayout_2->setSpacing(qRound(maximumHeight() / 4.0));
    timing_group_layout->setSpacing(horizontalLayout_2->spacing());
    for (auto *layout : {horizontalLayout_8, horizontalLayout_3,
                         horizontalLayout_4, octave_group_layout, editlines_group_layout})
      layout->setSpacing(qMax(2, qRound(maximumHeight() / 12.0)));
    horizontalLayout_2->setStretchFactor(overview_group, 1);
    // Share one stable area beside the menu for monitoring and hover help.
    horizontalLayout_6->removeWidget(status_label);
    horizontalLayout_2->removeWidget(cpu_group);
    auto *status_area = new QWidget(widget_2);
    _status_stack = new QStackedLayout(status_area);
    _status_stack->setContentsMargins(0, 0, 0, 0);
    _status_stack->addWidget(cpu_group);
    _status_stack->addWidget(status_label);
    horizontalLayout_6->insertWidget(0, status_area, 1);
    cpu_group_layout->setAlignment(Qt::AlignLeft);
    for (auto *toggle : {edit_onoff, drunk_velocity_onoff}) {
      toggle->vertical_text = toggle->text();
      toggle->_unscaled_vertical_text = true;
      toggle->setText("");
      toggle->_show_enabled_marker = false;
      toggle->_always_gradient_when_checked = true;
      toggle->setFixedWidth(fm.height());
      toggle->setMinimumHeight(0);
      toggle->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
      toggle->setFocusPolicy(Qt::NoFocus);
    }
    edit_onoff->setAccessibleName("Edit mode");
    edit_onoff->setToolTip("Toggle Edit mode: record played notes into the editor. Right-click for options.");
    drunk_velocity_onoff->setAccessibleName("Drunk velocity");
    drunk_velocity_onoff->setToolTip("Toggle Drunk velocity: vary input velocity between Min and Max. Right-click for options.");
    for (auto *toggle : {edit_onoff, drunk_velocity_onoff}) {
      toggle->_hovered_callback = [toggle](bool hovered) {
        GFX_SetStatusBar(hovered ? toggle->toolTip().toUtf8().constData() : "");
      };
    }
    horizontalLayout_2->removeWidget(editlines_group);
    horizontalLayout_2->insertWidget(horizontalLayout_2->indexOf(octave_group) + 1,
                                     editlines_group);
    auto *volume_group = new QWidget(widget);
    auto *volume_layout = new QHBoxLayout(volume_group);
    volume_layout->setContentsMargins(0, 0, 0, 0);
    volume_layout->setSpacing(horizontalLayout_8->spacing());
    volume_layout->addWidget(volume_label);
    volume_layout->addWidget(system_volume_slider);
    horizontalLayout_2->addWidget(volume_group);

    _playback_modes = new QButtonGroup(this);
    _scroll_mode = new MyQCheckBox("S", editing_group);
    _scroll_mode->setObjectName("scroll_mode");
    editing_group_layout->insertWidget(0, _scroll_mode);
    play_cursor_onoff->setText("C");
    editor_follows_play_cursor_onoff->setText("F");
    _scroll_mode->setToolTip("Scroll the editor during playback (default).");
    play_cursor_onoff->setToolTip("Show the play cursor during playback.");
    editor_follows_play_cursor_onoff->setToolTip("Show the play cursor and follow it during playback.");
    int mode = 0;
    for (auto *button : {_scroll_mode, play_cursor_onoff, editor_follows_play_cursor_onoff}) {
      _playback_modes->addButton(button, mode++);
      button->_show_enabled_marker = false;
      button->_always_gradient_when_checked = true;
      button->setFixedSize(maximumHeight(), maximumHeight());
      button->setFocusPolicy(Qt::NoFocus);
      button->setAccessibleName(button->toolTip());
      button->_hovered_callback = [button](bool hovered) {
        GFX_SetStatusBar(hovered ? button->toolTip().toUtf8().constData() : "");
      };
    }
    click_onoff->_show_enabled_marker = false;
    click_onoff->_always_gradient_when_checked = true;
    click_onoff->setFixedSize(maximumHeight(), maximumHeight());
    click_onoff->setFocusPolicy(Qt::NoFocus);
    click_onoff->_hovered_callback = [this](bool hovered) {
      GFX_SetStatusBar(hovered ? click_onoff->toolTip().toUtf8().constData() : "");
    };
    editing_group_layout->insertSpacing(3, horizontalLayout_8->spacing());
    connect(_playback_modes, &QButtonGroup::idToggled, this, [this](int mode, bool checked) {
      if (!checked || _initing)
        return;
      enablePlayCursor(mode != 0);
      enableEditorFollowsPlayCursor(mode == 2);
    });
    update_playback_mode();
    update_timing_group_visibility();
    //frame->setMinimumHeight(fm.height()*3/2);
    //frame->setMaximumHeight(fm.height()*3/2);

    velocity_layout->setSpacing(horizontalLayout_2->spacing());
    for (auto *spinbox : {min_velocity, velocity}) {
      spinbox->setMinimumHeight(0);
      spinbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
      spinbox->setFixedWidth(history->sizeHint().width());
    }
    history->installEventFilter(this);
    velocity_group_layout->setSpacing(horizontalLayout_8->spacing());
    min_velocity->setAccessibleName("Minimum velocity (%)");
    min_velocity->setToolTip("Minimum input velocity (%). Click to edit; drag horizontally to adjust.");

    min_velocity->setValue(4000);
    velocity->setValue(8000);
    update_velocity_controls();

    drunk_velocity_onoff->setChecked(drunk_velocity);

    if(scrolls_per_second==-1)
      scrolls_per_second = SETTINGS_read_int32("scrolls_per_second", default_scrolls_per_second);

    octave->setArrowHoverHighlight(true);
    editlines->setArrowHoverHighlight(true);
    octave->setArrowFocusEnabled(false);
    editlines->setArrowFocusEnabled(false);
    if (!g_is_starting_up)
      history->setHistoryState(Undo_num_undos(), CanRedo(), Undo_num_undos_since_last_save() != 0);

    octave->blockSignals(true);
    // The main bottom bar is constructed before root is allocated.
    octave->setValue(root != NULL ? root->keyoct / 12 : 3);
    octave->blockSignals(false);

    editlines->setValue(1);
    
    sps->setValue(scrolls_per_second);
    
    // For now, I haven't tried any machine where smooth scrolling doesn't work better than non-smooth scrolling. Anyay, just hide the sps options for now.
    scrollrate_group->hide();

    // cpu / xruns
    {
      int64_t guinum = S7CALL2(int_void,"FROM_C-create-cpu-usage-widget");
      QWidget *cpuusage = API_gui_get_widget(guinum);
      if (cpuusage != NULL)
        cpu_group_layout->addWidget(cpuusage);
      else{
        cpu_group->hide();
        R_ASSERT_NON_RELEASE(false);
      }
    }

    
    _initing = false;

    
    // set up timers, but only in the first created bottom bar.
    if (g_bottom_bars.size() == 0){

      _timer2.setInterval(50);
      _timer2.start();
    }

    g_bottom_bars.push_back(this);

    if (g_bottom_bars.size() > 1 && g_system_out_plugin!=NULL)
      GFX_OS_set_system_volume_plugin(g_system_out_plugin);

    // Set up custom popup menues for the time widgets
    {
      bpm->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(bpm, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowBPMPopup(const QPoint&)));

      bpm_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(bpm_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowBPMPopup(const QPoint&)));

      lpb->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(lpb, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowLPBPopup(const QPoint&)));
      
      lpb_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(lpb_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowLPBPopup(const QPoint&)));
      
      signature->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(signature, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowSignaturePopup(const QPoint&)));

      signature_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(signature_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowSignaturePopup(const QPoint&)));
    }

    
    {
      QColor system_color = get_qcolor(HIGH_BACKGROUND_COLOR_NUM);
      QPalette pal(palette());
      pal.setColor( QPalette::Active, QPalette::Dark, system_color);
      pal.setColor( QPalette::Active, QPalette::Light, system_color);
      pal.setColor( QPalette::Inactive, QPalette::Dark, system_color);
      pal.setColor( QPalette::Inactive, QPalette::Light, system_color);
      pal.setColor( QPalette::Disabled, QPalette::Dark, system_color);
      pal.setColor( QPalette::Disabled, QPalette::Light, system_color);
      setPalette(pal);
    }

    
    octave->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-octave-popup-menu");
    };

    history->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-history-popup-menu");
    };
    
    drunk_velocity_onoff->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-switch-drunk_velocity-popup-menu");
    };
    
    edit_onoff->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-switch-edit-popup-menu");
    };
    
    click_onoff->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-switch-click-popup-menu");
    };
    
    play_cursor_onoff->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-switch-play-cursor-popup-menu");
    };
    _scroll_mode->_show_popup_menu = play_cursor_onoff->_show_popup_menu;

    editor_follows_play_cursor_onoff->_show_popup_menu = [](){
      S7CALL2(void_void,"FROM_C-show-bottom-bar-switch-editor-follows-play-cursor-popup-menu");
    };

    const int value_width = fm.height() + 4;
    signature->setFixedWidth(value_width);
    signature->setMinimumHeight(0);
    signature->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    new VerticalLineEdit(signature);
    for (auto *spinbox : {lpb, bpm, octave, editlines}) {
      auto *vertical_edit = new VerticalLineEdit(spinbox->findChild<QLineEdit *>());
      QStyleOptionSpinBox option;
      spinbox->initStyleOption(&option);
      const QRect value_rect = spinbox->style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                               QStyle::SC_SpinBoxEditField, spinbox);
      // Match the visible text areas; reserve arrow space only where needed.
      spinbox->setFixedWidth(value_width + spinbox->width() - value_rect.width());
      spinbox->setMinimumHeight(0);
      spinbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
      if (spinbox == bpm) {
        vertical_edit->setHorizontalAbove(999, bpm);
        // QSpinBox suppresses its editor's textChanged when setValue is used.
        connect(bpm, qOverload<int>(&QSpinBox::valueChanged), vertical_edit,
                [vertical_edit](int){ vertical_edit->refreshPresentation(); });
      }
    }
    system_volume_slider->setMinimumHeight(0);
    system_volume_slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    // Reserve space for signed dB values and the MIDI-learn marker once.
    QStyleOptionSpinBox volume_option;
    system_volume_slider->initStyleOption(&volume_option);
    const QRect volume_input = system_volume_slider->style()->subControlRect(
      QStyle::CC_SpinBox, &volume_option, QStyle::SC_SpinBoxEditField, system_volume_slider);
    system_volume_slider->setFixedWidth(qMax(history->sizeHint().width() * 3 / 2,
      fm.horizontalAdvance("*-40.0") + 12 + system_volume_slider->width() - volume_input.width()));
  }

  QStackedLayout *_status_stack = nullptr;
  QButtonGroup *_playback_modes = nullptr;
  MyQCheckBox *_scroll_mode = nullptr;

  void update_playback_mode() {
    const int mode = root && ATOMIC_GET(root->play_cursor_onoff)
      ? (ATOMIC_GET(root->editor_follows_play_cursor_onoff) ? 2 : 1) : 0;
    const QSignalBlocker blocker(_playback_modes);
    _playback_modes->button(mode)->setChecked(true);
  }

  void set_status_text(const QString &text) {
    status_label->setText(text);
    _status_stack->setCurrentWidget(text.isEmpty() ? cpu_group : status_label);
  }

  bool eventFilter(QObject *watched, QEvent *event) override {
    if (watched == history && event->type() == QEvent::Resize) {
      min_velocity->setFixedWidth(history->width());
      velocity->setFixedWidth(history->width());
    }
    return QWidget::eventFilter(watched, event);
  }

  ~Bottom_bar_widget(){
    g_bottom_bars.removeOne(this);
    delete _navigator_widget;
  }

  QWidget *_navigator_widget = NULL;

  MyQButton *_hamburger_button = NULL;
      
  void update_timing_group_visibility() {
    timing_group->setVisible(!SignatureWidget->isHidden()
                             || (!LPBWidget->isHidden() && !lpb->isHidden())
                             || !BPMWidget->isHidden());
  }

  void remove_editor_elements(void){

    velocity_group->hide();
    
    edit_onoff->hide();
    
    octave_group->hide();
    
    lpb->hide();
    lpb_label->hide();

    _scroll_mode->hide();
    play_cursor_onoff->hide();
    editor_follows_play_cursor_onoff->hide();

    editlines_group->hide();
    update_timing_group_visibility();

  }

  /*
  void resizeEvent( QResizeEvent *qresizeevent) override {
    if(_navigator_widget != NULL){
      _navigator_widget->setMinimumHeight(frame->height());
      _navigator_widget->setMaximumHeight(frame->height());
    }
  }
  */
  void enterEvent(QEnterEvent *event) override {
    setCursor(Qt::ArrowCursor);
  }

  void updateWidgets(void){
    signature->setText(Rational(root->signature).toString());
    lpb->setValue(root->lpb);
    bpm->setValue(root->tempo);
    editlines->setValue(getNoteScrollLength());
    update_playback_mode();
  }

  void update_velocity_controls(){
    velocity->setToolTip(drunk_velocity ? "Maximum input velocity (%)" : "Input velocity (%)");
    velocity->setAccessibleName(velocity->toolTip());
    velocity->setToolTip(velocity->toolTip() + ". Click to edit; drag horizontally to adjust.");
    min_velocity->setVisible(drunk_velocity);

    min_velocity->setMaximum(velocity->value());

    if(drunk_velocity==true)
      velocity->setMinimum(min_velocity->value());
    else
      velocity->setMinimum(0);
  }

  void set_volume_slider_string(void){
    if (g_system_out_plugin != NULL){
      const SoundPluginType *type = g_system_out_plugin->type;
      
      int effect_num = type->num_effects+EFFNUM_INPUT_VOLUME;
      
      system_volume_slider->setPrefix(
        PLUGIN_has_midi_learn(g_system_out_plugin, effect_num) ? "*" : "");
    }
  }

public slots:
  
  void ShowBPMPopup(const QPoint& pos)
  {
    printf("GOTIT bpm\n");
    GFX_SimpleMenu("show BPM track", [](int command, bool onoff){
        showHideBPMTrack(-1);
      });
  }

  void ShowLPBPopup(const QPoint& pos)
  {
    printf("GOTIT lpb\n");
    GFX_SimpleMenu("show LPB track", [](int command, bool onoff){
        showHideLPBTrack(-1);
      });
  }

  void ShowSignaturePopup(const QPoint& pos)
  {
    printf("GOTIT signature\n");
    GFX_SimpleMenu("show time signature track", [](int command, bool onoff){
        showHideSignatureTrack(-1);
      });
  }

  void on_signature_editingFinished(){
    printf("signature bottombar\n");
    
    Rational rational(signature->text());
    signature->pushValuesToRoot(rational);

    signature->setText(Rational(root->signature).toString());
    set_editor_focus();
  }

  void on_lpb_editingFinished(){
    printf("lpb bottombar\n");
    setMainLPB(lpb->value());
    lpb->clearFocus();
    set_editor_focus();
  }

  void on_bpm_editingFinished(){
    printf("bpm bottombar\n");
    setMainBPM(bpm->value());
    bpm->clearFocus();
    set_editor_focus();
  }
  
  void on_system_volume_slider_valueChanged(int val){
    //printf("%d val: %d %p / %p / %p\n",_triggered_by_user,val,g_system_audio_instrument_widget,g_system_out_patch,g_system_out_plugin);
    //GFX_SetBrightness(root->song->tracker_windows, scale(val,0,10000,0,1));

    if (_triggered_by_user == true && g_system_audio_instrument_widget != NULL)
      g_system_audio_instrument_widget->input_volume_slider->setValue(val);
    
    set_volume_slider_string();
  }

  void on_octave_valueChanged(int val){
    if (root != NULL)
      setKeyAdd(val * 12);
  }

  void on_octave_editingFinished(){
    octave->clearFocus();
    set_editor_focus();
  }

  void on_editlines_valueChanged(int val){
    setNoteScrollLength(val);
  }

  void on_editlines_editingFinished(){
    editlines->clearFocus();
    set_editor_focus();
  }

  void on_sps_valueChanged(int val){
    scrolls_per_second = val;
    SETTINGS_write_int("scrolls_per_second", val);
  }

  void on_sps_editingFinished(){
    set_editor_focus();
  }

  void on_drunk_velocity_onoff_toggled(bool val){
    if(_initing==true)
      return;
    
    drunk_velocity = val;
    update_velocity_controls();
    velocity->update();
  }

  void on_min_velocity_valueChanged(int val){
    if(_initing==true)
      return;
    root->min_standardvel = val*MAX_VELOCITY / 10000;
    update_velocity_controls();
  }

  void on_velocity_valueChanged(int val){
    if(_initing==true)
      return;
    root->standardvel = val*MAX_VELOCITY / 10000;
    update_velocity_controls();
  }

  void on_edit_onoff_toggled(bool val){
    if(val!=ATOMIC_GET(root->editonoff))
      switchEditOnOff();
  }

  void on_click_onoff_toggled(bool val){
    enableMetronome(val);
  }


};

extern "C"{
  /*
  void GFX_OS_set_system_volume_peak_pointers(float *pointers, int num_channels){
    SLIDERPAINTER_set_peak_value_pointers(g_bottom_bar_widget->system_volume_slider->_painter, num_channels, pointers);
  }
  */

  // Note: plugin might be NULL.
  void GFX_OS_set_system_volume_plugin(struct SoundPlugin *plugin){
    {
      radium::PlayerLock lock;
      g_system_out_plugin = plugin;
      g_RT_system_out_input_latency = 0;
    }
    
    g_system_out_patch = NULL;
    g_system_audio_instrument_widget = NULL;
        
    for(auto *bottom_bar_widget : g_bottom_bars){
      
      auto *system_volume_slider = bottom_bar_widget->system_volume_slider;
      
      system_volume_slider->_patch.set(NULL);
      system_volume_slider->_effect_num = EFFNUM_INPUT_VOLUME;
      
      if (plugin == NULL){
        
        static float nullfloats[2] = {0.0f, 0.0f}; // [NO_STATIC_ARRAY_WARNING]
        SLIDERPAINTER_set_peak_value_pointers(system_volume_slider->_painter, 2, nullfloats, true);
        
      } else {
        
        const SoundPluginType *type = plugin->type;
        
        SLIDERPAINTER_set_peak_value_pointers(system_volume_slider->_painter, type->num_inputs, plugin->input_volume_peaks.decaying_dbs, true);
        
      }
    }
      
  }

  void GFX_OS_call_very_often(void){
    for(auto *bottom_bar_widget : g_bottom_bars){
      auto *system_volume_slider = bottom_bar_widget->system_volume_slider;
      if (system_volume_slider->_patch.data() != NULL)
        system_volume_slider->calledRegularlyByParent();
    }
  }

  void GFX_OS_UpdateKeyOctave(void){
    if(g_is_starting_up==false)
      for(auto *bottom_bar_widget : g_bottom_bars){
        auto *octave = bottom_bar_widget->octave;
        const bool was_blocked = octave->blockSignals(true);
        octave->setValue(root->keyoct / 12);
        octave->blockSignals(was_blocked);
      }
  }

  void GFX_OS_update_bottombar(void){
    if(g_is_starting_up==false)
      for(auto *bottom_bar_widget : g_bottom_bars)
        bottom_bar_widget->updateWidgets();
  } 
 
  void OS_GFX_NumUndosHaveChanged(int num_undos, bool redos_are_available, bool has_unsaved_undos){
    for(auto *bottom_bar_widget : g_bottom_bars){
      bottom_bar_widget->history->setHistoryState(num_undos, redos_are_available, has_unsaved_undos);
    }
  }

  // This function could be deleted since the timer polls the system out plugin now. But it's a little bit smoother to call this function directly, and it probably doesn't hurt to keep it.
  void OS_GFX_SetVolume(int value){
    for(auto *bottom_bar_widget : g_bottom_bars){
      bottom_bar_widget->_triggered_by_user=false;
      bottom_bar_widget->system_volume_slider->setValue(value);
      bottom_bar_widget->_triggered_by_user=true;
    }
  }

  void OS_GFX_IncVolume(int how_much){
    for(auto *bottom_bar_widget : g_bottom_bars){
      int new_value = how_much + bottom_bar_widget->system_volume_slider->value();
      if(new_value<0)
        new_value=0;
      if(new_value>10000)
        new_value=10000;
      
      bottom_bar_widget->system_volume_slider->setValue(new_value);
    }
  }
}

bool GFX_OS_patch_is_system_out(struct Patch *patch){
  return patch==g_system_out_patch;
}

struct Patch *GFX_OS_get_system_out(void){
  if (g_system_out_patch==NULL)
    return NULL;
  
  if (PATCH_get_from_id(g_system_out_patch->id)==NULL){
    R_ASSERT_NON_RELEASE(false);
    return NULL;
  }

  return g_system_out_patch;
}

struct SoundPlugin *RT_get_system_out_plugin(void){
  return g_system_out_plugin;
}

// Called when the hamburger popup menu (the main menus) has been opened.
// Stops the attention drawing of the hamburger buttons.
void GFX_OS_hamburger_menu_was_opened(void){
  QToolTip::hideText();

  for(auto *bottom_bar_widget : g_bottom_bars)
    if (bottom_bar_widget->_hamburger_button != NULL)
      bottom_bar_widget->_hamburger_button->set_attention(false);
}

bool EDITOR_switch_drunk_velocity(void){
  drunk_velocity = !drunk_velocity;
  
  for(auto *bottom_bar_widget : g_bottom_bars){
    bottom_bar_widget->_initing = true;
    bottom_bar_widget->update_velocity_controls();
    bottom_bar_widget->velocity->update();
    bottom_bar_widget->drunk_velocity_onoff->setChecked(drunk_velocity);
    bottom_bar_widget->_initing = false;
  }
  
  return drunk_velocity;
}
  
QWidget *BottomBar_create(QWidget *parent, bool include_editor_elements, bool include_navigator){
  auto *ret = new Bottom_bar_widget(parent, include_navigator);
  if(!include_editor_elements)
    ret->remove_editor_elements();

  {
    // Hamburger button opening a popup menu with the main menus. (See FROM_C-popup-main-menus)
    auto *hamburger_button = new MyQButton("hamburger-list-menu-svgrepo-com.svg", ret);
    hamburger_button->setObjectName("hamburger_button");
    hamburger_button->setToolTip("Main menus");
    hamburger_button->_statusbar_text = "Open main menu. Press left alt to open";
    hamburger_button->setFocusPolicy(Qt::NoFocus);
    // The button text is an SVG filename, so make the button square instead of letting
    // its width be based on the length of the text.
    const int hamburger_size = ret->minimumHeight();
    hamburger_button->setFixedSize(hamburger_size, hamburger_size);
    QObject::connect(hamburger_button, &MyQButton::clicked, [hamburger_button](){
      GFX_open_hamburger_popup_menu(hamburger_button);
    });
    hamburger_button->_show_popup_menu = [](){
      S7CALL2(void_void, "FROM_C-show-hamburger-keybinding-popup");
    };
    ret->menu_group_layout->addWidget(hamburger_button);
    ret->_hamburger_button = hamburger_button;
  }
  
  {
    const QFontMetrics fn = QFontMetrics(QApplication::font());
    ret->status_label->setMinimumWidth(fn.boundingRect("Main Pipe/System Compression Release").width()); // sion Release: 9.99
    ret->status_label->_use_custom_painter = true;
  }
  
  {
    QColor system_color = get_qcolor(HIGH_BACKGROUND_COLOR_NUM);
    QPalette pal(ret->palette());
    pal.setColor( QPalette::Active, QPalette::Dark, system_color);
    pal.setColor( QPalette::Active, QPalette::Light, system_color);
    pal.setColor( QPalette::Inactive, QPalette::Dark, system_color);
    pal.setColor( QPalette::Inactive, QPalette::Light, system_color);
    pal.setColor( QPalette::Disabled, QPalette::Dark, system_color);
    pal.setColor( QPalette::Disabled, QPalette::Light, system_color);
    ret->setPalette(pal);
  }

  ret->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");

  /*
  {
    QColor system_color(SETTINGS_read_string("system_color","#d2d0d5"));
    ret->setStyleSheet("#frame { border: 1px solid " + system_color.darker(150).name(QColor::HexArgb) + "; }");
  }
  */
  return ret;
}


/*
void BottomBar_set_system_audio_instrument_widget_and_patch(Ui::Audio_instrument_widget *system_audio_instrument_widget, struct Patch *system_out_patch){
g_system_audio_instrument_widget = system_audio_instrument_widget;
  g_system_out_patch = system_out_patch;

  SoundPlugin *plugin = (SoundPlugin*)g_system_out_patch->patchdata;
  const SoundPluginType *type = plugin->type;

  { // for undo
    g_bottom_bar_widget->system_volume_slider->_patch = g_system_out_patch;
    g_bottom_bar_widget->system_volume_slider->_effect_num = EFFNUM_INPUT_VOLUME;
    SLIDERPAINTER_set_num_channels(g_bottom_bar_widget->system_volume_slider->_painter, type->num_inputs);
  }

  GFX_OS_UpdateKeyOctave();
}
  
*/
