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

# Original U6 PIT counter values -> Hz conversion
# Hz = 1193180 / counter
INSTRUMENT_FREQ_TABLE = [
    (7851, 151),   # note 0: counter=7851, hz=151
    (3116, 383),   # note 1: counter=3116, hz=383
    (3497, 341),   # note 2: counter=3497, hz=341
    (3926, 303),   # note 3: counter=3926, hz=303
    (4159, 286),   # note 4: counter=4159, hz=286
    (4668, 255),   # note 5: counter=4668, hz=255
    (5240, 227),   # note 6: counter=5240, hz=227
    (5882, 202),   # note 7: counter=5882, hz=202
    (6231, 191),   # note 8: counter=6231, hz=191
    (6995, 170),   # note 9: counter=6995, hz=170
]

# Instrument configurations
# GM Program numbers (0-indexed):
#   46 = Orchestral Harp
#   6 = Harpsichord
#   24 = Acoustic Guitar (nylon) - closest to Lute
#   75 = Pan Flute
#   13 = Xylophone
INSTRUMENTS = {
    'harp': {
        'type': 0,
        'gm_program': 46,  # Orchestral Harp
        'duration_ms': 500,
        'velocity': 80,
        'freq_divisor': 2,  # Original uses di >> 1
    },
    'harpsichord': {
        'type': 1,
        'gm_program': 6,   # Harpsichord
        'duration_ms': 400,
        'velocity': 100,
        'freq_divisor': 1,
    },
    'lute': {
        'type': 2,
        'gm_program': 24,  # Acoustic Guitar (nylon)
        'duration_ms': 450,
        'velocity': 90,
        'freq_divisor': 2,  # Original uses di >> 1
    },
    'panpipes': {
        'type': 3,
        'gm_program': 75,  # Pan Flute
        'duration_ms': 400,
        'velocity': 85,
        'freq_divisor': 4,  # Original uses di >> 2
    },
    'xylophone': {
        'type': 4,
        'gm_program': 13,  # Xylophone
        'duration_ms': 200,
        'velocity': 110,
        'freq_divisor': 1,
    },
}

def hz_to_midi_note(hz):
    """Convert frequency in Hz to MIDI note number"""
    if hz <= 0:
        return 60  # Default to middle C
    # MIDI note = 69 + 12 * log2(freq / 440)
    return int(round(69 + 12 * math.log2(hz / 440.0)))

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
            counter, base_hz = INSTRUMENT_FREQ_TABLE[note_idx]

            # Apply instrument-specific frequency divisor
            effective_hz = base_hz // inst_config['freq_divisor']
            if effective_hz < 20:
                effective_hz = 20  # Minimum audible frequency

            wav_filename = f"{wav_id}.wav"
            wav_path = os.path.join(args.output, wav_filename)

            if use_fluidsynth:
                midi_note = hz_to_midi_note(effective_hz)
                # Clamp to valid MIDI range
                midi_note = max(21, min(108, midi_note))

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
