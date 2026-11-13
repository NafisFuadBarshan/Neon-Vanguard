# NEON VANGUARD: RIFT PROTOCOL

A complete, self-contained 3D arena-survival game built with **C++ and classic OpenGL/GLUT**. It is designed to open as a Code::Blocks project and uses procedural geometry, so no external models, textures, audio, or asset folders are required.

## Features

- Third-person 3D arena with a neon grid, perimeter, cover structures, and procedural environment layout
- Three weapons: Pulse Rifle, Arc Shotgun, and Rail Lance
- Wave-based enemy encounters with three enemy classes and health bars
- Enemy pursuit AI, collision-aware movement, player damage, shield absorption, and invulnerability frames
- Dash ability powered by regenerating energy
- Health, shield, energy, ammo, reload, score, wave, and elimination HUD
- Health, shield, and ammunition pickups
- Particle bursts, hit feedback, floating pickups, and animated enemy motion
- Title, pause, defeat, and redeploy screens
- No external assets required

## Requirements

- Windows
- Code::Blocks
- MinGW compiler
- FreeGLUT development package (headers, import library, and DLL)
- OpenGL and GLU (normally supplied with Windows graphics libraries)

## Build in Code::Blocks

1. Install Code::Blocks with the MinGW compiler.
2. Install **FreeGLUT**. You need `freeglut.h`, `freeglut` import library, and `freeglut.dll`.
3. Extract this ZIP.
4. Open `NEON_VANGUARD.cbp` in Code::Blocks.
5. If FreeGLUT is installed in a nonstandard directory, edit the project's **Search directories**:
   - Compiler: add the directory containing `GL/freeglut.h`
   - Linker: add the directory containing `libfreeglut.a` (or your FreeGLUT MinGW library)
6. In **Project → Build options → Linker settings**, ensure these libraries are linked:
   - `freeglut`
   - `opengl32`
   - `glu32`
7. Build and run with **F9**.
8. If Windows reports that `freeglut.dll` is missing, copy the matching 32-bit or 64-bit `freeglut.dll` beside the generated `.exe` (or place it on PATH). The DLL architecture must match your compiler.

### Alternative: compile from a terminal

From the project directory, with FreeGLUT installed and its paths configured:

```bash
g++ main.cpp -o NEON_VANGUARD.exe -lfreeglut -lopengl32 -lglu32
```

If your installation uses a static FreeGLUT library, its required system libraries may differ.

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Move |
| Mouse | Aim / rotate view |
| Left mouse button | Fire |
| R | Reload |
| 1 / 2 / 3 | Select weapon |
| Space | Dash |
| Shift | Sprint |
| P / Esc | Pause / resume |
| Enter | Start / redeploy |

## Gameplay

- Eliminate all enemies in a wave to trigger the next wave.
- Keep moving; enemies pursue your position.
- Your shield absorbs incoming damage before health.
- Collect glowing drops to restore health, shield, or ammunition.
- Dash consumes energy. Energy regenerates automatically.
- Weapon ammunition is shared through the reserve pool; switching weapons changes the magazine capacity.

## Troubleshooting

**`GL/freeglut.h: No such file or directory`**
Install FreeGLUT and add its include directory to the compiler search path.

**`cannot find -lfreeglut`**
Add the FreeGLUT library directory to the linker search path. Check whether your package names the library `freeglut` or `freeglut_static`.

**`freeglut.dll was not found`**
Place the correct architecture DLL next to the executable.

**Black window or immediate crash**
Update your graphics driver and confirm that OpenGL/GLU and FreeGLUT are installed correctly.

## Project structure

```text
NEON_VANGUARD_GLUT/
├── NEON_VANGUARD.cbp
├── main.cpp
└── README.md
```

## Notes

This project intentionally uses the classic fixed-function OpenGL pipeline for compatibility with standard GLUT/FreeGLUT setups and beginner-friendly Code::Blocks toolchains. It is a substantial playable prototype rather than a modern shader-based engine. The source is organized in one file to make first-time setup straightforward; you can split it into modules as a next development step.
