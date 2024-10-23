# not-a-virus

A joke "virus" using OpenGL and a little MASM.

This was an excuse to learn the basics of rendering a 3D model in OpenGL
and refresh a bit on MASM.

## Summary

- User runs installer, `Discord-Nitro-Free-2024-v2.7.exe`
- Prompts user if they trust the program and want to install
  - Clicking "No", just continues to display prompt
- After clicking "Yes", installer downloads "second stage" named `Rat.exe` and places on Desktop
- Alert that install was successful pops up and `Rat.exe` is executed
- `Rat.exe` displays rotating 3D model of a rat from Halo 3 

Get it? Rat as in RAT (Remote Access Trojan)...haha...

TODO: screenshots/video

## Development

- Visual Studio 2022
- [MASM32 SDK](https://masm32.com/) (installed on same drive as repo)
- [GLFW 3.4](https://www.glfw.org/download.html)
- [Glad](https://glad.dav1d.de/)
  - Language=C/C++, Specification=OpenGL, GL=3.3, Profile=Core
  - Generate a loader enabled

File download in installer can be tested locally using - `python3 -m http.server`

## References

- https://learn.microsoft.com/en-us/windows/win32/api/
- https://learn.microsoft.com/en-us/cpp/assembler/masm/microsoft-macro-assembler-reference
- [Halo3 Rat Model by Bylan](https://sketchfab.com/3d-models/halo-3-the-god-rat-e1853357d88545c9ab33e069641bc65c)
- https://learnopengl.com/
