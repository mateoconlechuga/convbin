include '../deps/fasmg-ez80/ez80.inc'
include './ti84pceg.inc'

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x, 10
	end repeat
end macro

relocaddr := ti.plotSScreen

extractableprgm:
	ld	hl,0
extractentrylabel := $% - 3
	ld	de,relocaddr
	ld	bc,0
extractsizelabel := $% - 3
	ldir
	jp	relocaddr
.relocate:
	org	relocaddr
extractprgm:
	ld	hl,0
extracttotalsizelabel := $% - 3
	call	ti.ErrNotEnoughMem
	ld	hl,ti.userMem
	ld	de,(ti.asm_prgm_size)
	call	ti.DelMem			; remove ourseleves
	xor	a,a
	sbc	hl,hl
	ld	(ti.asm_prgm_size),hl
	ld	hl,extractdata
.loop:
	ld	a,(hl)
	or	a,a
	jp	z,ti.userMem
	push	hl
	call	ti.Mov9ToOP1
.find:
	call	ti.ChkFindSym
	jp	c,.notfound
	call 	ti.ChkInRam
	jr	nz,.inarc
	call	ti.PushOP1
	call	ti.Arc_Unarc
	call	ti.PopOP1
	jr	.find
.inarc:
	ex	de,hl
	ld	de,9
	add	hl,de
	ld	e,(hl)
	add	hl,de
	inc	hl
	call	ti.LoadDEInd_s
	push	hl
	push	de
	ld	de,ti.userMem
	ld	hl,(ti.asm_prgm_size)
	add	hl,de
	ex	de,hl
	pop	hl
	push	de
	push	hl
	call	ti.InsertMem
	pop	hl
	push	hl
	ld	de,(ti.asm_prgm_size)
	add	hl,de
	ld	(ti.asm_prgm_size),hl
	pop	bc
	pop	de
	pop	hl
	ldir
	pop	hl
	ld	bc,9
	add	hl,bc
	jr	.loop
.notfound:
	pop	hl
	ld	a,0
	jp	ti.JError
extractdata:

display 10
print "#define EXTRACTOR_ENTRY_OFFSET ", extractentrylabel
print "#define EXTRACTOR_PRGM_SIZE_OFFSET ", extractsizelabel
print "#define EXTRACTOR_EXTRACT_SIZE_OFFSET ", extracttotalsizelabel
print "#define EXTRACTOR_APPVARS_OFFSET ", $%
display 10
print "size: ", $%
