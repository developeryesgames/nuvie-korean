================================================================================
Musical Instrument Sound System - Nuvie
================================================================================

This document explains how to customize musical instrument sounds in Nuvie.

--------------------------------------------------------------------------------
1. DEFAULT BEHAVIOR (PC Speaker Emulation)
--------------------------------------------------------------------------------

By default, instruments use PC Speaker emulation based on the original U6 code.
This produces authentic but "retro" beeping sounds.

Instruments and their original implementations:
  - Harp (0):        Waved note with decay (case 2 in original)
  - Harpsichord (1): Waved note with fast decay (case 3 in original)
  - Lute (2):        Waved note with slow decay (case 18 in original)
  - Panpipes (3):    Simple tone (case 19 in original)
  - Xylophone (4):   Waved note with sharp attack (case 20 in original)

Each instrument has 10 notes (keys 0-9) based on the original frequency table:
  Note 0: 7851 Hz
  Note 1: 3116 Hz
  Note 2: 3497 Hz
  Note 3: 3926 Hz
  Note 4: 4159 Hz
  Note 5: 4668 Hz
  Note 6: 5240 Hz
  Note 7: 5882 Hz
  Note 8: 6231 Hz
  Note 9: 6995 Hz

--------------------------------------------------------------------------------
2. CUSTOM WAV FILES
--------------------------------------------------------------------------------

To use custom WAV files for instruments:

1) Set sfx_style to "custom" in nuvie.cfg:
   <sfx_style>custom</sfx_style>

2) Create a directory for custom SFX files and set it in nuvie.cfg:
   <sfxdir>/path/to/your/sfx/</sfxdir>

3) Create sfx_map.cfg in that directory with instrument SFX mappings.

SFX ID calculation:
  Base ID = 40
  Instrument offset: Harp=0, Harpsichord=10, Lute=20, Panpipes=30, Xylophone=40
  Note = 0-9

  Formula: SFX_ID = 40 + (instrument * 10) + note

Example sfx_map.cfg entries for custom instrument sounds:
--------------------------------------------------------------------------------
; Harp notes (SFX IDs 40-49)
40;100
41;101
42;102
43;103
44;104
45;105
46;106
47;107
48;108
49;109

; Harpsichord notes (SFX IDs 50-59)
50;200
51;201
52;202
53;203
54;204
55;205
56;206
57;207
58;208
59;209

; Lute notes (SFX IDs 60-69)
60;300
61;301
62;302
63;303
64;304
65;305
66;306
67;307
68;308
69;309

; Panpipes notes (SFX IDs 70-79)
70;400
71;401
72;402
73;403
74;404
75;405
76;406
77;407
78;408
79;409

; Xylophone notes (SFX IDs 80-89)
80;500
81;501
82;502
83;503
84;504
85;505
86;506
87;507
88;508
89;509
--------------------------------------------------------------------------------

Then place the corresponding WAV files in your sfxdir:
  100.wav, 101.wav, ... (for Harp)
  200.wav, 201.wav, ... (for Harpsichord)
  etc.

--------------------------------------------------------------------------------
3. HYBRID APPROACH
--------------------------------------------------------------------------------

If you only want to customize some instruments/notes:
- Only add entries to sfx_map.cfg for the sounds you want to replace
- Missing entries will fall back to PC Speaker emulation (if using pcspeaker
  style) or remain silent (if using custom style without mapping)

--------------------------------------------------------------------------------
4. FUTURE: MIDI SUPPORT
--------------------------------------------------------------------------------

MIDI instrument support may be added in a future version, allowing:
- Real-time MIDI note generation
- External MIDI device output
- SoundFont-based instrument playback

================================================================================
