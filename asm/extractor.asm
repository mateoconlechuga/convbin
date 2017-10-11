#include "ti84pce.inc"

#macro relocate(new_location)
 #ifdef old_location
   .echo "Mateo: ",__file,":",__line,": error: You cannot nest relocate blocks."
 #else
   #define old_location eval($)
   .org new_location
   #define g_location eval(new_location)
 #endif
#endmacro

#macro endrelocate()
 #ifdef g_location
   .org $-g_location + old_location
   #undefine g_location
   #undefine old_location
 #else
   .echo "Error line ",__line,": No relocate statements corresponds to this endrelocate."
 #endif
#endmacro

.org usermem-2
.db $EF, $7B
.assume adl=1
	
	ld	hl,_runinramlabel         ; label to start of data section
	ld	de,plotSScreen
	ld	bc,_runinramlabel_end - _runinramlabel
	ldir
	jp	plotSScreen

_runinramlabel:
relocate(plotSScreen)
	ld	de,(asm_prgm_size)
	ld	hl,usermem
	call	_DelMem
	or	a,a
	sbc	hl,hl
	ld	(asm_prgm_size),hl
	ld	de,0                        ; uncompressed size *here*
	call	_memchker
	jp	c,_ErrMemory
	ld	(asm_prgm_size),de
	ex	de,hl
	ld	de,usermem
	call	_InsertMem
	call	_ChkFindSym
	call	_ChkInRAM
	ex	de,hl
	jr	z,_inram
	ld	de,9
	add	hl,de
	ld	e,(hl)
	add	hl,de
	inc	hl
_inram:
	ld	de,0                        ; change label *here*
	add	hl,de
	ld	de,0                        ; decompression location goes *here* -- add whatever offset bytes nesassary
	call	_dzx7_Standard
	jp	0                           ; decompression location goes *here* -- add whatever offset bytes nesassary              
_memchker:
	call	_MemChk
	or	a,a
	sbc	hl,de
	ret
	
#include "decompress.asm"

endrelocate()
_runinramlabel_end:

.echo "loader size: ", _runinramlabel_end - _runinramlabel