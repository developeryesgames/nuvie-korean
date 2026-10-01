Ultima6 Korean v1.6.1 — macOS Apple Silicon

nuvie is the tested Release arm64 consumer executable.
SHA-256: 68f17bc7fb11b51659e78b1aa82f70db2a6374438f269427b5166e4315e51a5e
It uses /opt/homebrew/opt/sdl2/lib/libSDL2-2.0.0.dylib, as the existing Mac release does.
Install SDL2 with: brew install sdl2
Copy this executable into your existing v1.6 Mac installation beside nuvie.cfg,
data and your own ULTIMA6 files, then run ./nuvie from that installation directory.
Keep your configuration and saves. Original game files and user saves are not included.

Build from the repository:
cmake -S . -B build_mac -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_mac --parallel 2
Output: build_mac/nuvie

QA: 277 actual SDL-engine checks passed. The consumer loaded a game and exited
normally via SDL quit/confirmation events. SDL dummy video/audio was used;
foreground application and original game/save hashes were unchanged.
Intel Mac and normal Gatekeeper installation are outside this test scope.
