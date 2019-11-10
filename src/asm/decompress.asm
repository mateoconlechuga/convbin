include '../deps/fasmg-ez80/ez80.inc'
include './ti84pceg.inc'

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x
	end repeat
end macro

relocaddr := ti.plotSScreen
relocoffset := compressedprgm.relocate - ti.userMem

	org	ti.userMem
	db	ti.tExtTok, ti.tAsm84CePrgm

compressedprgm:
	ld	hl,.relocate
	ld	de,decompressprgm
	ld	bc,compresseddata - decompressprgm
	ldir
	jp	decompressprgm
.relocate:
	org	relocaddr
decompressprgm:
	ld	de,(ti.asm_prgm_size)
	ld	hl,ti.userMem
	call	ti.DelMem
	or	a,a
	sbc	hl,hl
	ld	(ti.asm_prgm_size),hl
	ld	de,0                        ; uncompressed size set by program (constant offset = 3)
.uncompressedsize := $-3
	push	de
	call	ti.MemChk
	pop	de
	or	a,a
	sbc	hl,de
	jp	c,ti.ErrMemory
	ld	(ti.asm_prgm_size),de
	ex	de,hl
	ld	de,ti.userMem
	call	ti.InsertMem
	call	ti.ChkFindSym
	call	ti.ChkInRam
	ex	de,hl
	jr	z,.inram
	ld	de,9
	add	hl,de
	ld	e,(hl)
	add	hl,de
	inc	hl
.inram:
	ld	de,(compresseddata - relocaddr) + relocoffset
	add	hl,de
	ld	de,ti.userMem
	call	decompress
	jp	ti.userMem

decompress:
	ld	a,128
.copybyteloop:
	ldi
.mainloop:
	call	.nextbit
	jr	nc,.copybyteloop
	push	de
	ld	de,0
	ld	bc,0
.lensizeloop:
	inc	d
	call	.nextbit
	jr	nc,.lensizeloop
.lenvalueloop:
	call	nc,.nextbit
	rl	c
	rl	b
	jr	c,.exit
	dec	d
	jr	nz,.lenvalueloop
	inc	bc
	ld	e,(hl)
	inc	hl
	sla	e
	inc	e
	jr	nc,.offsetend
	ld	d,16
.rldnextbit:
	call	.nextbit
	rl	d
	jr	nc,.rldnextbit
	inc	d
	srl	d
.offsetend:
	rr	e
	ex	(sp),hl
	push	hl
	sbc	hl,de
	pop	de
	ldir
.exit:
	pop	hl
	jr	nc,.mainloop
.nextbit:
	add	a,a
	ret	nz
	ld	a,(hl)
	inc	hl
	rla
	ret

compresseddata:

print "uncompressed size offset: ", (decompressprgm.uncompressedsize - relocaddr) + relocoffset
