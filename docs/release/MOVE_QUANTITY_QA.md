# Move quantity regression verification — v1.6.1

Verified on 2026-10-01 with actual Release builds of the Nuvie SDL2 engine.

| Platform | Build | Checks | Save/reload arrow count |
| --- | --- | --- | --- |
| Windows | Visual Studio 2022, Release Win32, full rebuild | 277 passed, 0 failures | 10 → 10 |
| Mac | Apple Silicon arm64, Release CMake/Clang | 277 passed, 0 failures | 10 → 10 |

The suite uses SDL text/key events and the engine's selection callbacks.
It covers 10-item stacks moved as 1, 4, 10 and empty-Enter/all across nine
floor/inventory/container routes, Escape before/after typing, destination
cancellation, zero/excess/overflow/non-digit input, Backspace, singleton moves,
stack merging, rejected/unreachable destinations, actor pushing and save/reload.
The original translated Windows engine reproduced the missing question and
whole-stack transfer; its four baseline assertions passed.

Tests used copied Windows saves and an independent Mac save directory, with
original game data read-only. SDL dummy video/audio and disabled audio config
kept OS windows, focus and sound unchanged. Original file hashes were preserved.
Mac's fresh original-game start needed an in-memory party fixture relocation
to a nearby empty area; no original save was changed or transferred.

The Mac consumer executable loaded a game and exited normally after SDL quit
and confirmation events. Windows consumer startup/save loading passed too.
The Mac package's runtime data matched the data used for QA; only its new
Korean README differed. Native binary SHA-256:
`68f17bc7fb11b51659e78b1aa82f70db2a6374438f269427b5166e4315e51a5e`.

Mac verification exposed undefined behavior deleting a derived SfxManager
through a base pointer without a virtual destructor. One `virtual` qualifier
fixes the shutdown trap. Windows needed a full rebuild to replace stale cached
objects after this vtable change; the final suite exited successfully.

The Mac artifact targets Apple Silicon and depends on the existing Homebrew
SDL2 installation. Intel Mac and normal Gatekeeper installation were not tested.
No tags or GitHub Release are created by this change.
