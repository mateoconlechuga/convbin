include '../deps/fasmg-ez80/ez80.inc'
include './ti84pceg.inc'

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x, 10
	end repeat
end macro

relocaddr := ti.plotSScreen
relocoffset := compressedprgm.relocate - ti.userMem
asmcompsize := 2
sizebytessize := 2

compressedprgm:
	ld	hl,0
decompressentrylabel := $% - 3
	ld	de,relocaddr
	ld	bc,compresseddata - decompressprgm
	ldir
	jp	relocaddr
.relocate:
	org	relocaddr
decompressprgm:
	ld	de,0
compressedsizelabel := $% - 3
	ld	hl,0
compressedusermemoffset0label := $% - 3
	push	de
	call	ti.DelMem
	pop	de
	ld	ix,ti.asm_prgm_size
	ld	hl,(ix)
	or	a,a
	sbc	hl,de
	ld	(ix),hl
	push	ix
	call	ti.MemChk
	pop	ix
	ld	de,0
uncompressedsizelabel := $% - 3
	or	a,a
	sbc	hl,de
	jp	c,ti.ErrMemory
	ld	hl,(ix)
	add	hl,de
	ld	(ix),hl
	ex	de,hl
	ld	de,0
compressedusermemoffset1label := $% - 3
	call	ti.InsertMem
	call	ti.ChkFindSym
	ret	c
	call	ti.ChkInRam
	ex	de,hl
	jr	z,.inram
	ld	de,9
	add	hl,de
	ld	e,(hl)
	add	hl,de
	inc	hl
.inram:
	ld	de,0
offsettocompresseddatalabel := $% - 3
	add	hl,de
	ld	de,0
compressedusermemoffset2label := $% - 3
	push	de
	call	decompress
	pop	hl
	jp	(hl)

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

display 10
print "#define DECOMPRESS_ENTRY_OFFSET ", decompressentrylabel
print "#define DECOMPRESS_DATA_OFFSET ", offsettocompresseddatalabel
print "#define DECOMPRESS_COMPRESSED_SIZE_OFFSET ", compressedsizelabel
print "#define DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET ", uncompressedsizelabel
print "#define DECOMPRESS_USERMEM_OFFSET_0 ", compressedusermemoffset0label
print "#define DECOMPRESS_USERMEM_OFFSET_1 ", compressedusermemoffset1label
print "#define DECOMPRESS_USERMEM_OFFSET_2 ", compressedusermemoffset2label
display 10
print "size: ", $%
