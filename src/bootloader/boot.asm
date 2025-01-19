org 0x7C00             ; Boot loader offset for legacy boot
bits 16                ; 16 bit code

%define ENDL 0x0D, 0x0A

;
; FAT12 header
;
jmp short start
nop

bdb_oem:                   db 'MSWIN4.1'         ; 8 bytes
bdb_bytes_per_sector:      dw 512                ; Bytes per sector
bdb_sectors_per_cluster:   db 1                  ; Number of sectors per cluster
bdb_reserved_sectors:      dw 1                  ; Number of reserved sectors (supposed to be a word, but mtools says no)
bdb_fat_count:             db 2                  ; File Allocation Table count
bdb_dir_entries_count:     dw 0E0h               ; Number of directory entries
bdb_total_sectors:         dw 2880               ; 2880 * 512 = 1.44 MB
bdb_media_descriptor_type: db 0F0h               ; 3.5 in. floppy disk
bdb_sectors_per_fat:       dw 9                  ; 9 sectors per fat
bdb_sectors_per_track:     dw 18                 ; Number of sectors per track
bdb_heads:                 dw 2                  ; Head count
bdb_hidden_sectors:        dd 0                  ; Hidden sector count
bdb_large_sectors:         dd 0                  ; Large sector count

; Extended boot record
ebr_drive_number:          db 0                  ; Drive number
                           db 0                  ; Random reserved byte ig
ebr_signature:             db 29h                ; 0x28 or 0x29
ebr_volume_id:             db 65h, 23h, 72h, 27h ; Four random bytes
ebr_volume_label:          db ' eatcheese '      ; 11 bytes
ebr_system_id:             db 'FAT12   '         ; 8 bytes

;
; Code below
;

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
    mov ax, 0                  ; We cannot write consts to DS/ES directly
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00             ; Stack grows downwards, so it won't overwrite our program

    ; Read something from the floppy disk
    ; BIOS should set DL to the correct drive number
    mov [ebr_drive_number], dl ; Set drive number to DL

    mov ax, 1                  ; Read from second sector
    mov cl, 1                  ; Read one sector
    mov bx, 0x7E00             ; Right after the bootloader
    call disk_read

    ; Print message
    mov si, msg_hello
    call puts

    cli
    hlt

;
; Error handlers
;
floppy_fail:
    mov si, msg_read_failed
    call puts
    int 16h            ; Wait for a keypress
    jmp 0FFFFh:0        ; Beginning of the BIOS, should reboot computer
    hlt

.halt:
    cli                ; Disable interrupts to prevent exiting the "halt" state
    jmp .halt

;
; Disk routines
;

;
; Converts LBA address to CHS address
; Args:
;     ax - LBA address
; Returns:
;     sector   -> cx (bits 0-5 from the right)
;     cylinder -> cx (bits 6-15)
;     head     -> dh
lba_to_chs:                          ; Logical Block Addressing -> Cylinder Head Sector
    push ax
    push dx

    xor dx, dx                       ; dx = 0
    div word [bdb_sectors_per_track] ; ax = LBA / SectorsPerTrack
                                     ; dx = LBA % SectorsPerTrack
    inc dx                           ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                       ; cx = sector

    xor dx, dx                       ; dx = 0
    div word [bdb_heads]             ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                     ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                       ; dh = head (dh = 8 high bits of dx, dl = low 8 bits of dx; high=left, low=right)
    mov ch, al                       ; ch = lower 8 bits of cylinder
    shl ah, 6                        ; shift into place
    or  cl, ah                       ; place upper two bits of cylinder into the top of cl
                                     ; CX =       ---CH--- ---CL---
                                     ; cylinder : 76543210 98
                                     ; sector   :            543210
    pop ax                           ; ax = dl (from stack)
    mov dl, al                       ; dl = ax
    pop ax                           ; ax = ax (from stack)
    ret

;
; Read sectors from a disk
; Args
;     ax    - LBA address
;     cl    - number of sectors to read (up to 128)
;     dl    - drive number
;     es:bx - memory location to store read data (destination)
disk_read:

    push ax                          ; Save original values of registers we'll modify
    push bx
    push cx
    push dx
    push di

    push cx                          ; Save original value of cx (since it will be overwritten)
    call lba_to_chs                  ; Takes in ax as the LBA address and returns CHS (cx, dh)
    pop ax                           ; AL = number of sectors to read (originally cl)

    mov ah, 02h                      ; Required for BIOS function/interrupt
    mov di, 3                        ; Retry read 3 times (for real hardware)

.retry:
    pusha                            ; This BIOS interrupt may mess with our registers, save them for later
    stc                              ; Set carry flag... to check for a successful read, the carry flag will be cleared
    int 13h                          ; Calls the BIOS read function given the args we previously supplied
    jnc .done                        ; Jump if carry clear, success!

    ; If read failed
    popa
    call disk_reset

    dec di                           ; di--
    test di, di                      ; Check if di == 0 (I think it performs a bitwise AND and sets the zero flag if the result is 0)
    jnz .retry                       ; If di != 0, loop

.fail:
    jmp floppy_fail                  ; Display error message on fail

.done:
    popa
    pop di                           ; Restore registers
    pop dx
    pop cx
    pop bx
    pop ax
    ret

;
; Reset disk controller
; Args
;     dl - drive number
disk_reset:
    pusha
    mov ah, 0                        ; Causes the disk controller to reset
    stc
    int 13h
    jc floppy_fail                   ; Carry should be clear if reset was successful, otherwise jump to floppy_fail
    popa
    ret
    


msg_hello:       db 'Loading KatterOS', ENDL, 0
msg_read_failed: db 'Failed to read floppy!', ENDL, 0

times 510-($-$$) db 0 ; Pad the program so that the signature begins 511 bytes into the sector
dw 0xAA55