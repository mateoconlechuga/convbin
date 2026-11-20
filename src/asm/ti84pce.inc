; TI-84 Plus CE symbols used by the embedded helpers.
; From ti84pceg.inc, with contributions by BrandonW, calc84,
; MateoConLechuga, Runer112, tr1p1ea, Kerm Martian, and Texas Instruments.

; OS entry points
.equ ti.Mov9ToOP1, 0x20320
.equ ti.MovFROP1, 0x2032C
.equ ti.MemChk, 0x204FC
.equ ti.ChkFindSym, 0x2050C
.equ ti.InsertMem, 0x20514
.equ ti.DelMem, 0x20590
.equ ti.PopRealO1, 0x205DC
.equ ti.PushRealO1, 0x20614
.equ ti.ErrNotEnoughMem, 0x2072C
.equ ti.ErrMemory, 0x20768
.equ ti.ErrCustom1, 0x2120C
.equ ti.Arc_Unarc, 0x21448
.equ ti.LoadDEInd_s, 0x21D9C
.equ ti.ChkInRam, 0x21F98

; OS variables and RAM addresses
.equ ti.asm_prgm_size, 0xD0118C
.equ ti.appErr1, 0xD025A9
.equ ti.plotSScreen, 0xD09466
.equ ti.userMem, 0xD1A881

.global ti.plotSScreen
