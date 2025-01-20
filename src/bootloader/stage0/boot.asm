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
    ; Code segment (cs) should be set to 0 with the offset of 0x7C00
    ; Setup data segments
    mov ax, 0                  ; We cannot write consts to DS/ES directly
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00             ; Stack grows downwards, so it won't overwrite our program

    ; Some BIOSes start at 07C0:0000 instead of 0000:7C00
    ; This trick ensures we are in the right location
    push es
    push word .after
    retf
.after:

    ; Read something from the floppy disk
    ; BIOS should set DL to the correct drive number
    mov [ebr_drive_number], dl ; Set drive number to DL

    ; Show loading message
    mov si, msg_hello
    call puts

    ; Read drive parameters from bios instead of relying on
    ; data from the disk
    push es
    mov ah, 08h
    int 13h
    jc floppy_fail
    pop es

    and cl, 3Fh                        ; Remove top two bits
    xor ch, ch
    mov [bdb_sectors_per_track], cx    ; Sector count

    inc dh
    mov [bdb_heads], dh                ; Head count

    ; Find LBA of root directory = reserved_sectors + (fats * sectors_per_fat)
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh                         ; Ensure we're only multiplying the low byte
    mul bx                             ; ax = (fats * sectors_per_fat)
    add ax, [bdb_reserved_sectors]     ; ax = LBA of root directory
    push ax

    ; compute size of root directory = (32 * number_of_entries) / bytes_per_sector
    ; https://wiki.osdev.org/FAT#Directories_on_FAT12/16/32
    mov ax, [bdb_dir_entries_count]
    shl ax, 5                          ; ax *= 32
    xor dx, dx                         ; dx = 0 (stores remainder)
    div word [bdb_bytes_per_sector]    ; ax /= 32 (number of sectors we need to read)

    test dx, dx                        ; if dx != 0, add 1
    jz .root_dir_after
    inc ax                             ; Division remainder != 0, add 1
                                       ; This ensures we have enough sectors (even partially filled ones)

.root_dir_after:

    ; Read root directory
    mov cl, al                         ; cl = number of sectors to read = size of root directory
    pop ax                             ; ax = LBA of root directory
    mov dl, [ebr_drive_number]         ; dl = drive number
    mov bx, buffer                     ; es:bx = buffer (ig)
    call disk_read
    
    ; Search for stage1.bin
    xor bx, bx                         ; bx = 0 (how many directory entries we've searched so far)
    mov di, buffer                     ; di points to our directory entries

.search_stage1:
    mov si, file_stage1_bin            ; Load "STAGE1  BIN"
    mov cx, 11                         ; Load string length
    push di
    repe cmpsb                         ; Repeat while equal (zero flag = 1):
                                       ;     compare string bytes (ds:si == es:di)
                                       ;     also cx--
    pop di
    je .found_stage1                   ; Jump out if we found the file

    add di, 32                         ; Moves to the next directory entry
    inc bx
    cmp bx, [bdb_dir_entries_count]    ; Check if we have read through all of the entries
    jl .search_stage1                  ; If there are more to check, jump to the beginning of the loop

    jmp stage1_not_found_error         ; Kernel not found

.found_stage1:

    ; di should point to the directory entry
    mov ax, [di + 26]                  ; di now points to the first cluster number (word)
    mov [stage1_cluster], ax

    ; Now find the other clusters in the File Allocation Table
    mov ax, [bdb_reserved_sectors]     ; ax = LBA of FAT
    mov bx, buffer                     ; es:bx = buffer
    mov cl, [bdb_sectors_per_fat]      ; cl = sectors per fat
    mov dl, [ebr_drive_number]         ; dl = drive number
    call disk_read

; TODO:
;     Needs better space efficiency
;     ; Calculate start of data section (where the clusters are stored)
;     mov bx, [bdb_reserved_sectors]     ; bx = reserved_sectors
;     mov ax, [bdb_fat_count]            ; ax = fats
;     mul word [bdb_sectors_per_fat]     ; ax = (fats * sectors_per_fat)
;     add bx, ax                         ; bx = reserved_sectors + (fats * sectors_per_fat)
;     mov ax, [bdb_dir_entries_count]    ; ax = dir_entries_count
;     mov dx, 32
;     mul dx                             ; ax = (dir_entries_count * 32) (size in bytes of all entries)
;     xor dx, dx                         ; prepare for division (remainder is stored in dx)
;     div word [bdb_bytes_per_sector]    ; ax = (dir_entries_count * 32) / bytes_per_sector (512)
;     test dx, dx                        ; Ensure we have enough sectors (if there is a remainder, add another sector)
;     jz .after_calc_data_start
;     inc ax                             ; ax = size in sectors of the directory entries
; .after_calc_data_start:
;     add bx, ax                         ; bx = (reserved_sectors + (fats * sectors_per_fat) + directory entries) (start_sector)
;     mov [cluster_data_start], bx       ; Save it for later (I'm fairly sure we are going to reference this multiple times
;                                        ; so I didn't push it to the stack)
    
    ; Read stage1 and process FAT chain
    mov bx, STAGE1_LOAD_SEGMENT
    mov es, bx
    mov bx, STAGE1_LOAD_OFFSET         ; es:bx = STAGE1_LOAD_SEGMENT:STAGE1_LOAD_OFFSET

.load_stage1_loop:
    ; Read next cluster
    mov ax, [stage1_cluster]
    ; sub ax, 2                          ; ax = (stage1_cluster - 2) (-2 = reserved clusters)
    ; mul byte [bdb_sectors_per_cluster] ; ax = (stage1_cluster - 2) * sectors_per_cluster
    ; add ax, [cluster_data_start]       ; ax = (stage1_cluster - 2) * sectors_per_cluster + start_sector (calculated above)
    add ax, 31                         ; Does what a lot of that other code does


    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]    ; Will overflow bx if stage1.bin is larger than 64KB
    ; TODO:
    ;     Check if bx is 0xFFFF and increment the segment (es)

    ; Compute location of next cluster
    mov ax, [stage1_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx                             ; ax = index of entry in FAT
                                       ; dx = cluster % 2
    mov si, buffer
    add si, ax
    mov ax, [ds:si]                    ; read entry from FAT table at index ax
    
    or dx, dx
    jz .even

; Shifting around to deal with loading the 12 bit value
.odd:
    shr ax, 4
    jmp .next_cluster_after
.even:
    and ax, 0x0FFF
.next_cluster_after:
    cmp ax, 0x0FF8
    jae .read_finish                ; Jump out once we've read through all the clusters

    mov [stage1_cluster], ax
    jmp .load_stage1_loop

.read_finish:
    ; Jump to stage1 (finally)
    mov dl, [ebr_drive_number]       ; Boot device in dl
    mov ax, STAGE1_LOAD_SEGMENT      ; Set segment registers
    mov ds, ax
    mov es, ax

    jmp STAGE1_LOAD_SEGMENT:STAGE1_LOAD_OFFSET

    jmp wait_key_and_reboot          ; Should never happen

    cli                              ; Disable interrupts
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

;
; Error handlers
;
floppy_fail:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

stage1_not_found_error:
    mov si, msg_stage1_not_found
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h            ; Wait for a keypress
    jmp 0FFFFh:0       ; Beginning of the BIOS, should reboot computer

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
    


msg_hello:            db 'Loading', ENDL, 0
msg_read_failed:      db 'Floppy error', ENDL, 0
msg_stage1_not_found: db 'STAGE1.BIN missing', ENDL, 0
file_stage1_bin:      db 'STAGE1  BIN'
stage1_cluster:       dw 0
; cluster_data_start:   dw 0

STAGE1_LOAD_SEGMENT   equ 0x2000     ; equ means #define
STAGE1_LOAD_OFFSET    equ 0          ; it doesn't allocate memory

times 510-($-$$) db 0 ; Pad the program so that the signature begins 511 bytes into the sector
dw 0xAA55

buffer:               ; Everything loaded into the next sectors will be after this label