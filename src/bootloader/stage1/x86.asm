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

;
; _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient, uint32_t *remainder);
;
global _x86_div64_32
_x86_div64_32:
    ; Make new call frame
    push bp
    mov bp, sp

    push bx                            ; We have to save this one

    ; Divide upper 32 bits
    mov eax, [bp + 8]                  ; eax = upper 32 bits of dividend
    mov ecx, [bp + 12]                 ; ecx = divisor
    xor edx, edx
    div ecx                            ; eax = upper 32 bits of quotient
                                       ; edx = temp remainder
    ; Store upper 32 bits of quotient
    mov bx, [bp + 16]                  ; Load quotient pointer
    mov [bx + 4], eax                  ; Dereference and store

    ; Divide lower 32 bits
    mov eax, [bp + 4]                  ; eax = lower 32 bits of dividend
                                       ; edx = temp remainder
    div ecx                            ; Probably factors in edx

    ; Store the rest of the results
    ; mov bx, [bp + 16]
    mov [bx], eax                      ; Store lower half of quotient

    mov bx, [bp + 18]                  ; Pointer for remainder
    mov [bx], edx                      ; Dereference and store remainder

    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret


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

