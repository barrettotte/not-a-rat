; Not a virus

.386
.model flat,stdcall
option casemap: none

; includes
include \masm32\include\windows.inc                     ; Windows constants and types (always first)
include \masm32\include\user32.inc                      ; User32.dll functions
include \masm32\include\kernel32.inc                    ; Kernel32.dll functions

includelib \masm32\lib\user32.lib                       ; link with User32.lib
includelib \masm32\lib\kernel32.lib                     ; link with Kernel32.lib

; constants

MSGBOX_YES equ 6h                                       ; message box yes selected
MSGBOX_NO  equ 7h                                       ; message box no selected


; =====================================================================================================

.data

; for anyone that runs strings or disassembles this
unauth       db "STOP! You are not authorized to read this! ",
                "Stop reading this immediately. OR ELSE!!!", 0

msgBox1Title db "Free Discord Nitro 2024 v2.7 - Nephilim Edition", 0
msgBox1Text  db "IMPORTANT NOTICE: This program is 100% SAFE and verified ",
                "by top security professionals to be VIRUS FREE.          ",
                "                                                         ",
                "                                                         ",
                "Please confirm that you completely trust this program and",
                " would like to proceed with secure and easy installation.", 0


; =====================================================================================================

.code

start:                                                  ; entry point
  call main                                             ; call main function
  invoke ExitProcess, eax                               ; exit program with status code


; =====================================================================================================

main proc

  display_msg:
  invoke MessageBoxA, 0, offset msgBox1Text,            ; display yes/no messagebox
    offset msgBox1Title, (MB_ICONQUESTION or MB_YESNO)  ;

  cmp eax, MSGBOX_YES                                   ; check for yes selection
  jne display_msg                                       ; continually show messagebox until yes selected

  ; TODO: download rat program

  ; TODO: run rat program

  done:
  mov eax, 0                                            ; set status code
  ret                                                   ; exit main procedure

main endp


; =====================================================================================================

end start
