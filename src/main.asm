org 0x7C00             ; Boot loader offset for legacy boot
bits 16                ; 16 bit code

main:
    hlt

.halt:
    jmp .halt

times 510-($-$$) db 0 ; Pad the program so that the signature begins 511 bytes into the sector
dw 0xAA55