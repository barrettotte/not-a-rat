# not-a-virus

A joke "virus" using MASM, win32 API, and OpenGL.

This was an excuse to refresh on a little MASM and learn the basics of OpenGL.

## Summary

- User runs `Discord-Nitro-Free-2024-v2.4.exe`
- Program repeatedly prompts user if they trust the program and want to install
  - Clicking "No", just continues to display prompt
- After clicking "Yes", program downloads "second stage" named `rat.exe` and places on Desktop
- Displays text saying that a "Rat" has been installed at Desktop
- `rat.exe` displays rotating 3D model of a Rat from Halo 3 (Get it? ...Rat as in RAT...)

TODO: screenshots and summary

## Development

- Developed with Visual Studio 2022
- Relies on [MASM32 SDK](https://masm32.com/) installed on same drive as repo

## References

- https://learn.microsoft.com/en-us/windows/win32/api/
- https://learn.microsoft.com/en-us/cpp/assembler/masm/microsoft-macro-assembler-reference
- [Halo3 Rat Model by Bylan](https://sketchfab.com/3d-models/halo-3-the-god-rat-e1853357d88545c9ab33e069641bc65c)
- https://learnopengl.com/
  - https://glad.dav1d.de/
