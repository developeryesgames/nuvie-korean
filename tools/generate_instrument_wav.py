#!/usr/bin/env python3
"""
Generate instrument WAV files for Nuvie using SoundFont
Requires: pip install midi2audio fluidsynth (or system fluidsynth)

Usage:
    python generate_instrument_wav.py --soundfont /path/to/soundfont.sf2 --output ./sfx/

This generates 50 WAV files (5 instruments x 10 notes) for custom SFX in Nuvie.
"""

import os
import sys
import argparse
import subprocess
import tempfile
import struct
import wave
import math

# Original U6 frequency table (from seg_27a1.c:1533)
# These values are already Hz frequencies passed directly to SOUND_FREQ macro
# Keys 1-9-0 play ascending scale (do-re-mi-fa-sol-la-si-do-re-mi)
INSTRUMENT_FREQ_TABLE = [
    7851,   # note 0 (key '0'): 7851 Hz - highest
    3116,   # note 1 (key '1'): 3116 Hz - lowest (do)
    3497,   # note 2 (key '2'): 3497 Hz (re)
    3926,   # note 3 (key '3'): 3926 Hz (mi)
    4159,   # note 4 (key '4'): 4159 Hz (fa)
    4668,   # note 5 (key '5'): 4668 Hz (sol)
    5240,   # note 6 (key '6'): 5240 Hz (la)
    5882,   # note 7 (key '7'): 5882 Hz (si)
    6231,   # note 8 (key '8'): 6231 Hz (do)
    6995,   # note 9 (key '9'): 6995 Hz (re) - second highest
]

# Instrument configurations
# GM Program numbers (0-indexed):
#   46 = Orchestral Harp
#   6 = Harpsichord
#   24 = Acoustic Guitar (nylon) - closest to Lute
#   75 = Pan Flute
#   13 = Xylophone
#
# target_octave: Target MIDI octave for best sound quality
#   Octave 3 = C3-B3 (MIDI 48-59), Octave 4 = C4-B4 (MIDI 60-71)
#   Octave 5 = C5-B5 (MIDI 72-83), Octave 6 = C6-B6 (MIDI 84-95)
INSTRUMENTS = {
    'harp': {
        'type': 0,
        'gm_program': 46,  # Orchestral Harp
        'duration_ms': 500,
        'velocity': 80,
        'freq_divisor': 2,  # Original uses di >> 1
        'target_octave': 4,  # Harp sounds good in mid range
    },
    'harpsichord': {
        'type': 1,
        'gm_program': 6,   # Harpsichord
        'duration_ms': 400,
        'velocity': 100,
        'freq_divisor': 1,
        'target_octave': 4,  # Harpsichord sounds best in octave 4-5
    },
    'lute': {
        'type': 2,
        'gm_program': 24,  # Acoustic Guitar (nylon)
        'duration_ms': 450,
        'velocity': 90,
        'freq_divisor': 2,  # Original uses di >> 1
        'target_octave': 4,  # Guitar natural range
    },
    'panpipes': {
        'type': 3,
        'gm_program': 75,  # Pan Flute
        'duration_ms': 400,
        'velocity': 85,
        'freq_divisor': 4,  # Original uses di >> 2
        'target_octave': 5,  # Pan flute sounds good higher
    },
    'xylophone': {
        'type': 4,
        'gm_program': 13,  # Xylophone
        'duration_ms': 200,
        'velocity': 110,
        'freq_divisor': 1,
        'target_octave': 5,  # Xylophone is a high instrument
    },
}

def hz_to_midi_note(hz, target_octave=4, note_index=0):
    """Convert frequency to MIDI note using major scale (do-re-mi-fa-sol-la-si-do-re-mi)

    Original U6 uses 10 notes: do-re-mi-fa-sol-la-si-do-re-mi (1.5 octaves)
    Keys 1-9-0 map to ascending major scale notes.

    Args:
        hz: Frequency in Hz (used for reference, not direct conversion)
        target_octave: Target MIDI octave (4 = C4-B4, middle range)
        note_index: The note index (0-9) from the original frequency table

    Returns:
        MIDI note number in major scale
    """
    # Major scale intervals from root (in semitones):
    # do=0, re=2, mi=4, fa=5, sol=7, la=9, si=11, do=12, re=14, mi=16
    major_scale = [0, 2, 4, 5, 7, 9, 11, 12, 14, 16]

    # Map note indices to scale positions:
    # Index 0 = key '0' = 10th note (mi, highest)
    # Index 1 = key '1' = 1st note (do, lowest)
    # Index 2-9 = keys '2'-'9' = 2nd to 9th notes
    if note_index == 0:
        scale_position = 9  # Key '0' -> 10th note (high mi)
    else:
        scale_position = note_index - 1  # Key '1' -> 0 (do), '2' -> 1 (re), etc.

    # Base MIDI note for target octave (C of that octave)
    base_midi = (target_octave + 1) * 12  # Octave 4 -> MIDI 60 (C4)

    # Add major scale interval
    target_midi = base_midi + major_scale[scale_position]

    # Clamp to valid MIDI range
    target_midi = max(21, min(108, target_midi))

    return target_midi

def encode_variable_length(value):
    """Encode a value as MIDI variable-length quantity"""
    result = []
    result.append(value & 0x7F)
    value >>= 7
    while value:
        result.append((value & 0x7F) | 0x80)
        value >>= 7
    return bytes(reversed(result))

def create_midi_file(midi_path, program, note, velocity, duration_ms):
    """Create a simple MIDI file with one note"""
    # Simple MIDI file creation
    with open(midi_path, 'wb') as f:
        # MIDI Header
        f.write(b'MThd')
        f.write(struct.pack('>I', 6))      # Header length
        f.write(struct.pack('>H', 0))      # Format 0
        f.write(struct.pack('>H', 1))      # 1 track
        f.write(struct.pack('>H', 480))    # Ticks per quarter note

        # Track chunk
        track_data = bytearray()

        # Program change (delta=0)
        track_data.extend([0x00, 0xC0, program])

        # Note on (delta=0)
        track_data.extend([0x00, 0x90, note, velocity])

        # Calculate ticks for duration (assuming 120 BPM)
        # 1 quarter note = 500ms at 120 BPM
        # ticks = duration_ms * 480 / 500
        ticks = int(duration_ms * 480 / 500)

        # Variable length encoding for delta time
        track_data.extend(encode_variable_length(ticks))

        # Note off
        track_data.extend([0x80, note, 0])

        # End of track (delta=0)
        track_data.extend([0x00, 0xFF, 0x2F, 0x00])

        f.write(b'MTrk')
        f.write(struct.pack('>I', len(track_data)))
        f.write(track_data)

def convert_stereo_to_mono(wav_path):
    """Convert stereo WAV to mono (Nuvie requires mono WAV files)"""
    with wave.open(wav_path, 'rb') as wav_in:
        params = wav_in.getparams()
        if params.nchannels == 1:
            return  # Already mono

        frames = wav_in.readframes(params.nframes)

    # Convert stereo to mono by averaging channels
    mono_frames = bytearray()
    sample_width = params.sampwidth

    for i in range(0, len(frames), sample_width * 2):
        if sample_width == 2:
            left = struct.unpack('<h', frames[i:i+2])[0]
            right = struct.unpack('<h', frames[i+2:i+4])[0]
            mono = (left + right) // 2
            mono_frames.extend(struct.pack('<h', mono))
        elif sample_width == 1:
            left = frames[i]
            right = frames[i+1]
            mono = (left + right) // 2
            mono_frames.append(mono)

    with wave.open(wav_path, 'wb') as wav_out:
        wav_out.setnchannels(1)
        wav_out.setsampwidth(params.sampwidth)
        wav_out.setframerate(params.framerate)
        wav_out.writeframes(bytes(mono_frames))

def generate_wav_fluidsynth(soundfont, midi_path, wav_path, sample_rate=22050, fluidsynth_path=None):
    """Use FluidSynth to render MIDI to WAV"""
    # Try to find fluidsynth executable
    if fluidsynth_path:
        fs_exe = fluidsynth_path
    else:
        # Check for local fluidsynth in tools/bin
        script_dir = os.path.dirname(os.path.abspath(__file__))
        local_fs = os.path.join(script_dir, 'bin', 'fluidsynth.exe')
        if os.path.exists(local_fs):
            fs_exe = local_fs
        else:
            fs_exe = 'fluidsynth'

    cmd = [
        fs_exe,
        '-ni',
        '-g', '1.0',
        '-R', '0',          # No reverb
        '-C', '0',          # No chorus
        '-r', str(sample_rate),
        '-F', wav_path,
        soundfont,
        midi_path
    ]

    try:
        # Use shell=False and pass absolute paths for Windows compatibility
        result = subprocess.run(cmd, check=True, capture_output=True, shell=False)
        # Convert stereo to mono (Nuvie requires mono WAV)
        convert_stereo_to_mono(wav_path)
        return True
    except subprocess.CalledProcessError as e:
        print(f"FluidSynth error: {e.stderr.decode() if e.stderr else 'unknown error'}")
        print(f"Command was: {' '.join(cmd)}")
        return False
    except FileNotFoundError as e:
        print(f"FluidSynth not found: {e}")
        return False

def generate_sine_wav(wav_path, frequency, duration_ms, sample_rate=22050):
    """Fallback: Generate simple sine wave WAV (better than PC Speaker square wave)"""
    num_samples = int(sample_rate * duration_ms / 1000)

    with wave.open(wav_path, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)

        # Generate sine wave with envelope (attack-decay)
        for i in range(num_samples):
            t = i / sample_rate

            # Simple ADSR envelope
            attack = 0.01   # 10ms attack
            decay = 0.1     # 100ms decay
            sustain = 0.7   # sustain level
            release_start = duration_ms / 1000 - 0.05

            if t < attack:
                envelope = t / attack
            elif t < attack + decay:
                envelope = 1.0 - (1.0 - sustain) * (t - attack) / decay
            elif t < release_start:
                envelope = sustain
            else:
                envelope = sustain * (1.0 - (t - release_start) / 0.05)

            # Sine wave with slight harmonics for richer sound
            sample = (math.sin(2 * math.pi * frequency * t) * 0.7 +
                     math.sin(4 * math.pi * frequency * t) * 0.2 +
                     math.sin(6 * math.pi * frequency * t) * 0.1)
            sample = int(sample * envelope * 32767 * 0.8)
            sample = max(-32768, min(32767, sample))

            wav_file.writeframes(struct.pack('<h', sample))

def generate_sfx_map(output_dir, instruments, start_wav_id=100):
    """Generate sfx_map.cfg file

    NOTE: The Nuvie parser uses semicolon as delimiter, so comments starting
    with semicolon will break parsing. Do NOT add comments to sfx_map.cfg!
    """
    map_path = os.path.join(output_dir, 'sfx_map.cfg')

    with open(map_path, 'w') as f:
        # No comments! Nuvie parser uses semicolon as delimiter
        wav_id = start_wav_id
        for inst_name, inst_config in instruments.items():
            inst_type = inst_config['type']
            for note in range(10):
                sfx_id = 40 + (inst_type * 10) + note
                f.write(f"{sfx_id};{wav_id}\n")
                wav_id += 1

    print(f"Generated: {map_path}")

def main():
    parser = argparse.ArgumentParser(description='Generate instrument WAV files for Nuvie')
    parser.add_argument('--soundfont', '-s', help='Path to SoundFont (.sf2) file')
    parser.add_argument('--output', '-o', default='./instrument_sfx/', help='Output directory')
    parser.add_argument('--sine', action='store_true', help='Use sine wave generation (no FluidSynth needed)')
    parser.add_argument('--sample-rate', type=int, default=22050, help='Sample rate (default: 22050)')
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)

    use_fluidsynth = args.soundfont and not args.sine

    if use_fluidsynth and not os.path.exists(args.soundfont):
        print(f"SoundFont not found: {args.soundfont}")
        print("Falling back to sine wave generation")
        use_fluidsynth = False

    wav_id = 100  # Starting WAV ID

    for inst_name, inst_config in INSTRUMENTS.items():
        inst_type = inst_config['type']
        print(f"\nGenerating {inst_name} sounds...")

        for note_idx in range(10):
            base_hz = INSTRUMENT_FREQ_TABLE[note_idx]

            # Apply instrument-specific frequency divisor
            effective_hz = base_hz // inst_config['freq_divisor']
            if effective_hz < 20:
                effective_hz = 20  # Minimum audible frequency

            wav_filename = f"{wav_id}.wav"
            wav_path = os.path.join(args.output, wav_filename)

            if use_fluidsynth:
                target_octave = inst_config.get('target_octave', 4)
                midi_note = hz_to_midi_note(effective_hz, target_octave, note_idx)

                with tempfile.NamedTemporaryFile(suffix='.mid', delete=False) as tmp:
                    midi_path = tmp.name

                try:
                    create_midi_file(
                        midi_path,
                        inst_config['gm_program'],
                        midi_note,
                        inst_config['velocity'],
                        inst_config['duration_ms']
                    )

                    if not generate_wav_fluidsynth(args.soundfont, midi_path, wav_path, args.sample_rate):
                        # Fallback to sine wave
                        generate_sine_wav(wav_path, effective_hz, inst_config['duration_ms'], args.sample_rate)
                finally:
                    if os.path.exists(midi_path):
                        os.unlink(midi_path)
            else:
                generate_sine_wav(wav_path, effective_hz, inst_config['duration_ms'], args.sample_rate)

            sfx_id = 40 + (inst_type * 10) + note_idx
            print(f"  Note {note_idx}: SFX {sfx_id} -> {wav_filename} ({effective_hz} Hz)")
            wav_id += 1

    # Generate sfx_map.cfg
    generate_sfx_map(args.output, INSTRUMENTS)

    print(f"\n=== Generation Complete ===")
    print(f"Output directory: {args.output}")
    print(f"Total WAV files: {wav_id - 100}")
    print(f"\nTo use in Nuvie:")
    print(f"1. In nuvie.cfg <ultima6> section, set:")
    print(f"   <sfx>custom</sfx>")
    print(f"   <sfxdir>{os.path.abspath(args.output)}</sfxdir>")
    print(f"2. The sfx_map.cfg is already in the output directory")

if __name__ == '__main__':
    main()
