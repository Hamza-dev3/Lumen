# LUMEN ☄️

A 3D ball-rolling game. You are a purple energy core rolling across three abandoned orbital platforms. Collect the blue orbs and reach the teleporter ring at the end of each stage.

Built with OpenGL 3.3 (GLEW + GLFW + GLM + stb_image).

## Controls

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` / Arrow keys | Roll |
| `Space` | Jump |
| `Mouse` | Orbit camera |
| `Esc` | Quit |

## Build

Requires GLEW, GLFW, GLM, and `stb_image.h`. Open the Visual Studio project, make sure the include and library paths for GLEW and GLFW are set, and build.

## Assets

Place the following files in an `assets/` folder next to the executable:

- `platform.jpg` — static platform texture
- `moving.jpg` — moving platform texture
- `sky.jpg` — skybox texture

If any are missing the game still runs with a grey fallback.
<img width="498" height="498" alt="PortalGIF" src="https://github.com/user-attachments/assets/4649e511-0de9-46a0-9514-222a5c7be405" />
