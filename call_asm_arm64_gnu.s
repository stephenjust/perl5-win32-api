
.globl Call_arm64_real
Call_arm64_real:

	sub		sp,sp,0x50		/* keep space for 7 64bit params, padding, red zone */

	/* Store register parameters */ 
	stp		fp,lr,[sp,0x10]	/* link register */
	stp		x0,x1,[sp,0x20]	/* ApiFunction,int_registers */
	stp		x2,x3,[sp,0x30]	/* float_registers,stack */
	str		x4,[sp,0x40]  /* number of stack entries */

	mov		fp,sp

	/* Load up integer registers first... */
	ldr		x9,[sp,0x28]	/* x9 = int_registers */
	ldp		x0,x1,[x9],0x10
	ldp		x2,x3,[x9],0x10
	ldp		x4,x5,[x9],0x10
	ldp		x6,x7,[x9]

	/* Now floating-point registers */
	ldr		x9,[sp,0x30]	/* x9 = float_registers */
	ld1		{v0.1d,v1.1d,v2.1d,v3.1d},[x9],0x20
	ld1		{v4.1d,v5.1d,v6.1d,v7.1d},[x9]

	/* Now copy any parameters from the stack */
	ldr		x9,[sp,0x38]	/* x9 = stack */
	ldr		x10,[sp,0x40]	/* x10,x11 = number of stack entries */
	ldr		x11,[sp,0x40]

	/* align stack so *after* the copystack loop it will be 16 bytes */
	lsl		x10,x10,3		/* Get total size of stack */
	and		x12,x10,0x08	/* 8 if array will be unaligned */
	add		x10,x12,x10		/* x10 = space to allocate for stack */

	/* Adjust stack pointer based on number of parameters on stack */
	sub		sp,sp,x10
	mov		x10,sp

	/* Skip copystack if no stack to copy */
	cmp		x11,0x00
	beq		docall

copystack:
	sub		x11,x11,1
	ldr		x12,[x9],0x08
	str		x12,[x10],0x08
	cmp 	x11,0x00
	bne		copystack

docall:
	/* And call */
	ldr		x10,[fp,0x20]   /* x10 = ApiFunction */
	blr		x10

	/* Restore stack, frame pointer, link register */
	mov		sp,fp
	ldp		fp,lr,[sp,0x10]

	/* pass through x0 and d0 to caller */
	add	sp,sp,80
	ret
