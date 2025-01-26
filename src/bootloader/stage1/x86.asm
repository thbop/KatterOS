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
;
; bool _cdecl x86_Disk_Reset(uint8_t driveNumber);
;
global _x86_Disk_Reset
_x86_Disk_Reset:
    ; Make new call frame
    push bp
    mov bp, sp

    mov dl, [bp + 4]                   ; Drive number
    mov ah, 0                          ; Causes the disk controller to reset
    stc
    int 13h                            ; Disk function

    mov ax, 1
    sbb ax, 0                          ; Set return to be subtract with borrow (inverts the carry flag)

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret

; bool _cdecl x86_Disk_Read(uint8_t driveNumber,
;                           uint16_t cylinder,
;                           uint16_t sector,
;                           uint16_t head,
;                           uint8_t count,
;                           uint8_t far *destBuffer);
global _x86_Disk_Read
_x86_Disk_Read:
    ; Make new call frame
    push bp
    mov bp, sp

    push bx
    push es

    ; pusha
    mov dl, [bp + 4]                   ; Drive number
    mov ch, [bp + 6]                   ; Cylinder (lower 8 bits)
    mov cl, [bp + 7]                   ; Cylinder (bits 6-7)
    shl cl, 6                          ; Shift left
    
    mov al, [bp + 8]                   ; Sector
    and al, 3Fh                        ; Mask out 6 low bits
    or cl, al                          ; Sector (bits 0-5)
    
    mov dh, [bp + 10]                  ; Head

    mov al, [bp + 12]                  ; Number of sectors to load (count)

    mov bx, [bp + 16]                  ; high 16 bits of the pointer to destination buffer
    mov es, bx
    mov bx, [bp + 14]                  ; low 16 bits of the pointer to destination buffer


    mov ah, 2                          ; Causes the disk controller to read
    stc
    int 13h                            ; Disk function

    mov ax, 1
    sbb ax, 0                          ; Set return to be subtract with borrow (inverts the carry flag)

    pop es
    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret

; bool _cdecl x86_Disk_GetDriveParams(uint8_t driveNumber,
;                                     uint8_t *driveTypeOut,
;                                     uint16_t *cylindersOut,
;                                     uint16_t *sectorsOut,
;                                     uint16_t *headsOut);
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:
    ; Make new call frame
    push bp
    mov bp, sp

    ; Save registers
    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]                   ; Drive number
    mov ah, 8                          ; Get drive parameters
    mov di, 0
    mov es, di                         ; Good idea to clear these due to "bios bugs"
    stc
    int 13h                            ; Disk function

    ; return
    mov ax, 1
    sbb ax, 0                          ; Set return to be subtract with borrow (inverts the carry flag)

    ; Out pointers
    mov si, [bp + 6]                   ; Load driveTypeOut* into si
    mov [si], bl                       ; Dereference and assign to drive type

    mov bl, ch                         ; Lower 8 bits of cylinders
    mov bh, cl                         ; Upper 2 bits of cylinders
    shr bh, 6                          ; Shift them right back into place
    mov si, [bp + 8]
    mov [si], bx                       ; Derefrence and write cylinders

    mov bl, cl                         ; Load number of sectors
    and bl, 3Fh                        ; Mask out upper two bits so that we only have bits 0-5
    mov si, [bp + 10]
    mov [si], bl                       ; Derefrence and store

    mov si, [bp + 12]
    mov [si], dh                       ; Derefrence and store head number


    ; Restore registers
    pop di
    pop si
    pop bx
    pop es

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret