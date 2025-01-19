org 0x0                ; Boot loader offset for legacy boot
bits 16                ; 16 bit code

%define ENDL 0x0D, 0x0A

start:
    ; Uses the same stack as the bootloader

    ; Print message
    mov si, msg_hello
    call puts

.halt:
    cli
    hlt


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

msg_hello: db 'Hello World!', 0, ENDL

