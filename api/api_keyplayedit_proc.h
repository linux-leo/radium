#ifndef _RADIUM_API_API_KEYPLAYEDIT_PROC_H
#define _RADIUM_API_API_KEYPLAYEDIT_PROC_H

enum {
  NOTEINPUT_KEYBOARD = 0,
  NOTEINPUT_MIDI = 1
};

extern LANGSPEC void NoteInput_cancel_all(void);
extern LANGSPEC void NoteInput_note_on(int source, struct Tracker_Windows *window, float notenum, float velocity, bool polyphonic, bool is_repeat);
extern LANGSPEC void NoteInput_note_off(int source, struct Tracker_Windows *window, float notenum);

#endif
