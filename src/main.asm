; Not a virus

.386
.model flat,stdcall
option casemap: none

; includes

include \masm32\include\windows.inc  ; Windows constants and types (always first)

include \masm32\include\user32.inc   ; User32.dll functions
include \masm32\include\kernel32.inc ; Kernel32.dll functions

includelib \masm32\lib\user32.lib    ; link with User32.lib
includelib \masm32\lib\kernel32.lib  ; link with Kernel32.lib

.data

msgBoxTitle db "Free Discord Nitro 2024 - Nephilim Edition", 0

; TODO: fake long install message
; TODO: install or cancel buttons
; TODO: on cancel, "continually ask are you sure?" and beg to install. Actually exit if 'x' is pressed.
msgBoxText  db "Press Ok to continue with installation", 0

; for anyone that runs strings or disassembles this
unauth db "You are not authorized to read this! ",
          "Close this immediately or else!", 0

.code

; entry point
main proc

  ; display message
  invoke MessageBoxA, 0, offset msgBoxText, offset msgBoxTitle, (MB_OKCANCEL or MB_ICONWARNING)

  ; exit program
  invoke ExitProcess, 0

main endp

end
