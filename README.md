# not-a-virus

A joke "virus" using OpenGL and a little MASM.

This was an excuse to learn the basics of parsing an OBJ file
and rendering a 3D model in OpenGL.

I also wanted to refresh on MASM a little which is why the installer is in assembly.

## Summary

- User runs installer, `Discord-Nitro-Free-2024-v2.7.exe`
- Prompts user if they trust the program and want to install
  - Clicking "No", just continues to display prompt
- After clicking "Yes", installer downloads "second stage" named `Rat.exe` and places on Desktop
- Alert that install was successful pops up and `Rat.exe` is executed
- `Rat.exe` displays mutliple windows of rotating 3D model of a rat from Halo 3 

Get it? Rat as in RAT (Remote Access Trojan)...haha...

TODO: screenshots/video

## Asset Prep

- Downloaded [Halo3 Rat Model by Bylan](https://sketchfab.com/3d-models/halo-3-the-god-rat-e1853357d88545c9ab33e069641bc65c)
- Converted `rat.fbx` to `rat.obj` using Blender
- Converted `rat.png` to `rat.bmp` using Paint.NET
  - set to 4-bit bit depth, Octree, dither 0
- Replaced `rat.png` with `rat.bmp` in `rat.mtl`

## Limitations

- Materials are restricted to only diffuse maps
- Textures have to be 4-bit BMP format
- I probably have some memory leaks, I'm not a C++ master...

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
