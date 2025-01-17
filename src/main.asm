org 0x7C00             ; Boot loader offset for legacy boot
bits 16                ; 16 bit code

%define ENDL 0x0D, 0x0A

start:
    jmp main

; Prints a string to the screen
; Args:
;     ds:si - points to a string
puts:
    ; Save current register states (so when we return they are popped back to their original value)
    push si
    push ax

.loop:
    lodsb              ; Loads a byte from ds:si into al (8-bit ax) and increments si
    or al, al          ; Sets zero processor flag if '\0' is found
    jz .done

    mov ah, 0x0E       ; Print character in BIOS TTY mode
    mov bh, 0          ; Set page number to 0
    int 0x10           ; Video interrupt

    jmp .loop

.done:
    pop ax
    pop si
    ret

main:
    ; Code segment (cs) should be set to 0 with the offset of 0x7C00
    ; Setup data segments
    mov ax, 0          ; We cannot write consts to DS/ES directly
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00     ; Stack grows downwards, so it won't overwrite our program

    ; Print message
    mov si, msg_hello
    call puts

    hlt

.halt:
    jmp .halt

msg_hello: db 'Hello World!', 0, ENDL

times 510-($-$$) db 0 ; Pad the program so that the signature begins 511 bytes into the sector
dw 0xAA55