bits 16

section _ENTRY class=CODE

extern _cstart_
global entry

entry:
    cli                                ; Interrupts need to be disabled while setting up the stack
    mov ax, ds
    mov ss, ax                         ; ss = ds (small memory model places stack and data in the same segment)
    mov sp, 0                          ; Could overwrite code if it exceeds ~60KB
    mov bp, sp                         ; Stack base pointer = Stack pointer
    sti


    ; Expect boot drive in dl, send it as an argument to cstart
    xor dh, dh
    push dx
    call _cstart_

    cli                                ; If things go wrong
    hlt
