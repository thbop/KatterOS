bits 16

section _TEXT class=CODE


; _cdecl format:
;     Arguments:
;         passsed through the stack
;         pushed from right to left
;         caller removes parameters from the stack
;     Return:
;         integers, pointers: EAX
;         floats, ST0
;     Registers:
;         EAX, ECX, EDX saved by the caller
;         all others saved by the callee (we must restore these registers)
;
;     Note: function label must start with an underscore


; A function accessable from C to print a character
; Args: character, page
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    ; Make new call frame
    push bp
    mov bp, sp

    push bx                            ; bx must be saved by the callee

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address (small memory model => 2 bytes)
    ; [bp + 4] - first argument (character)
    ; [bp + 6] - second argument (page)
    ; (You cannot push single bytes onto the stack, thus 2 and 4 are used instead of 1 and 2)
    mov ah, 0Eh                        ; Write character instruction
    mov al, [bp + 4]                   ; The character to write
    mov bh, [bp + 6]                   ; The page

    int 10h                            ; Call the video interrupt

    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret
