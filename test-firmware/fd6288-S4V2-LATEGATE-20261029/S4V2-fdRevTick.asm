
obj-startup-S4J-alignnative-hold-acqstats-launch-settle-reposition-safe-fullretry-adjacent-stale-late/AM32_CUSTOM_FD6288_G431_2.20.elf:     file format elf32-littlearm


Disassembly of section .text:

08002c34 <fdRevTick>:
 8002c34:	e92d 47f0 	stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
 8002c38:	b082      	sub	sp, #8
 8002c3a:	f3ef 8510 	mrs	r5, PRIMASK
 8002c3e:	b672      	cpsid	i
 8002c40:	f3ef 8210 	mrs	r2, PRIMASK
 8002c44:	b672      	cpsid	i
 8002c46:	4baf      	ldr	r3, [pc, #700]	@ (8002f04 <fdRevTick+0x2d0>)
 8002c48:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002c4c:	b119      	cbz	r1, 8002c56 <fdRevTick+0x22>
 8002c4e:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8002c52:	2900      	cmp	r1, #0
 8002c54:	d053      	beq.n	8002cfe <fdRevTick+0xca>
 8002c56:	f382 8810 	msr	PRIMASK, r2
 8002c5a:	f8d3 266c 	ldr.w	r2, [r3, #1644]	@ 0x66c
 8002c5e:	2a00      	cmp	r2, #0
 8002c60:	d148      	bne.n	8002cf4 <fdRevTick+0xc0>
 8002c62:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002c66:	2900      	cmp	r1, #0
 8002c68:	d044      	beq.n	8002cf4 <fdRevTick+0xc0>
 8002c6a:	4ca7      	ldr	r4, [pc, #668]	@ (8002f08 <fdRevTick+0x2d4>)
 8002c6c:	f894 10cc 	ldrb.w	r1, [r4, #204]	@ 0xcc
 8002c70:	2900      	cmp	r1, #0
 8002c72:	d03f      	beq.n	8002cf4 <fdRevTick+0xc0>
 8002c74:	f8b4 1470 	ldrh.w	r1, [r4, #1136]	@ 0x470
 8002c78:	2900      	cmp	r1, #0
 8002c7a:	d03b      	beq.n	8002cf4 <fdRevTick+0xc0>
 8002c7c:	f894 1474 	ldrb.w	r1, [r4, #1140]	@ 0x474
 8002c80:	2900      	cmp	r1, #0
 8002c82:	d037      	beq.n	8002cf4 <fdRevTick+0xc0>
 8002c84:	49a1      	ldr	r1, [pc, #644]	@ (8002f0c <fdRevTick+0x2d8>)
 8002c86:	f8d4 040c 	ldr.w	r0, [r4, #1036]	@ 0x40c
 8002c8a:	684e      	ldr	r6, [r1, #4]
 8002c8c:	f894 742c 	ldrb.w	r7, [r4, #1068]	@ 0x42c
 8002c90:	f1a6 716e 	sub.w	r1, r6, #62390272	@ 0x3b80000
 8002c94:	f5a1 5118 	sub.w	r1, r1, #9728	@ 0x2600
 8002c98:	1a09      	subs	r1, r1, r0
 8002c9a:	4681      	mov	r9, r0
 8002c9c:	489c      	ldr	r0, [pc, #624]	@ (8002f10 <fdRevTick+0x2dc>)
 8002c9e:	4281      	cmp	r1, r0
 8002ca0:	46b0      	mov	r8, r6
 8002ca2:	f080 80b7 	bcs.w	8002e14 <fdRevTick+0x1e0>
 8002ca6:	f8d4 1414 	ldr.w	r1, [r4, #1044]	@ 0x414
 8002caa:	4630      	mov	r0, r6
 8002cac:	1a71      	subs	r1, r6, r1
 8002cae:	f8d4 6418 	ldr.w	r6, [r4, #1048]	@ 0x418
 8002cb2:	42b1      	cmp	r1, r6
 8002cb4:	46b2      	mov	sl, r6
 8002cb6:	f0c0 819a 	bcc.w	8002fee <fdRevTick+0x3ba>
 8002cba:	9100      	str	r1, [sp, #0]
 8002cbc:	9201      	str	r2, [sp, #4]
 8002cbe:	f7ff f85d 	bl	8001d7c <fdRevRetryEligible.constprop.0>
 8002cc2:	9900      	ldr	r1, [sp, #0]
 8002cc4:	4b8f      	ldr	r3, [pc, #572]	@ (8002f04 <fdRevTick+0x2d0>)
 8002cc6:	2800      	cmp	r0, #0
 8002cc8:	f040 81a2 	bne.w	8003010 <fdRevTick+0x3dc>
 8002ccc:	1e7a      	subs	r2, r7, #1
 8002cce:	2a04      	cmp	r2, #4
 8002cd0:	d80d      	bhi.n	8002cee <fdRevTick+0xba>
 8002cd2:	e8df f012 	tbh	[pc, r2, lsl #1]
 8002cd6:	00f4      	.short	0x00f4
 8002cd8:	00bf0132 	.word	0x00bf0132
 8002cdc:	0008000c 	.word	0x0008000c
 8002ce0:	f8d4 1414 	ldr.w	r1, [r4, #1044]	@ 0x414
 8002ce4:	1a71      	subs	r1, r6, r1
 8002ce6:	4a8a      	ldr	r2, [pc, #552]	@ (8002f10 <fdRevTick+0x2dc>)
 8002ce8:	4291      	cmp	r1, r2
 8002cea:	f080 81ec 	bcs.w	80030c6 <fdRevTick+0x492>
 8002cee:	2301      	movs	r3, #1
 8002cf0:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8002cf4:	f385 8810 	msr	PRIMASK, r5
 8002cf8:	b002      	add	sp, #8
 8002cfa:	e8bd 87f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
 8002cfe:	4983      	ldr	r1, [pc, #524]	@ (8002f0c <fdRevTick+0x2d8>)
 8002d00:	4c81      	ldr	r4, [pc, #516]	@ (8002f08 <fdRevTick+0x2d4>)
 8002d02:	6849      	ldr	r1, [r1, #4]
 8002d04:	460e      	mov	r6, r1
 8002d06:	f894 147c 	ldrb.w	r1, [r4, #1148]	@ 0x47c
 8002d0a:	b9b9      	cbnz	r1, 8002d3c <fdRevTick+0x108>
 8002d0c:	f894 10c2 	ldrb.w	r1, [r4, #194]	@ 0xc2
 8002d10:	2900      	cmp	r1, #0
 8002d12:	d145      	bne.n	8002da0 <fdRevTick+0x16c>
 8002d14:	f8b4 0470 	ldrh.w	r0, [r4, #1136]	@ 0x470
 8002d18:	2800      	cmp	r0, #0
 8002d1a:	d148      	bne.n	8002dae <fdRevTick+0x17a>
 8002d1c:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002d20:	2900      	cmp	r1, #0
 8002d22:	f040 8089 	bne.w	8002e38 <fdRevTick+0x204>
 8002d26:	2100      	movs	r1, #0
 8002d28:	f884 1474 	strb.w	r1, [r4, #1140]	@ 0x474
 8002d2c:	f8a4 1468 	strh.w	r1, [r4, #1128]	@ 0x468
 8002d30:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002d34:	2101      	movs	r1, #1
 8002d36:	f8c4 146a 	str.w	r1, [r4, #1130]	@ 0x46a
 8002d3a:	e78c      	b.n	8002c56 <fdRevTick+0x22>
 8002d3c:	2002      	movs	r0, #2
 8002d3e:	9200      	str	r2, [sp, #0]
 8002d40:	f7ff f8ca 	bl	8001ed8 <fdBenchStop>
 8002d44:	9a00      	ldr	r2, [sp, #0]
 8002d46:	4b6f      	ldr	r3, [pc, #444]	@ (8002f04 <fdRevTick+0x2d0>)
 8002d48:	e785      	b.n	8002c56 <fdRevTick+0x22>
 8002d4a:	f8d4 1420 	ldr.w	r1, [r4, #1056]	@ 0x420
 8002d4e:	eba6 0e01 	sub.w	lr, r6, r1
 8002d52:	f894 143b 	ldrb.w	r1, [r4, #1083]	@ 0x43b
 8002d56:	460f      	mov	r7, r1
 8002d58:	2900      	cmp	r1, #0
 8002d5a:	f000 830c 	beq.w	8003376 <fdRevTick+0x742>
 8002d5e:	496c      	ldr	r1, [pc, #432]	@ (8002f10 <fdRevTick+0x2dc>)
 8002d60:	458e      	cmp	lr, r1
 8002d62:	d307      	bcc.n	8002d74 <fdRevTick+0x140>
 8002d64:	f8d4 1430 	ldr.w	r1, [r4, #1072]	@ 0x430
 8002d68:	eba6 0c01 	sub.w	ip, r6, r1
 8002d6c:	4969      	ldr	r1, [pc, #420]	@ (8002f14 <fdRevTick+0x2e0>)
 8002d6e:	458c      	cmp	ip, r1
 8002d70:	f080 8333 	bcs.w	80033da <fdRevTick+0x7a6>
 8002d74:	2100      	movs	r1, #0
 8002d76:	f894 40c3 	ldrb.w	r4, [r4, #195]	@ 0xc3
 8002d7a:	88d8      	ldrh	r0, [r3, #6]
 8002d7c:	b13c      	cbz	r4, 8002d8e <fdRevTick+0x15a>
 8002d7e:	f9b3 4008 	ldrsh.w	r4, [r3, #8]
 8002d82:	ea24 74e4 	bic.w	r4, r4, r4, asr #31
 8002d86:	b2a4      	uxth	r4, r4
 8002d88:	42a0      	cmp	r0, r4
 8002d8a:	bf28      	it	cs
 8002d8c:	4620      	movcs	r0, r4
 8002d8e:	4281      	cmp	r1, r0
 8002d90:	bf28      	it	cs
 8002d92:	4601      	movcs	r1, r0
 8002d94:	b289      	uxth	r1, r1
 8002d96:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002d9a:	2800      	cmp	r0, #0
 8002d9c:	f47f af5b 	bne.w	8002c56 <fdRevTick+0x22>
 8002da0:	2003      	movs	r0, #3
 8002da2:	9200      	str	r2, [sp, #0]
 8002da4:	f7ff f898 	bl	8001ed8 <fdBenchStop>
 8002da8:	9a00      	ldr	r2, [sp, #0]
 8002daa:	4b56      	ldr	r3, [pc, #344]	@ (8002f04 <fdRevTick+0x2d0>)
 8002dac:	e753      	b.n	8002c56 <fdRevTick+0x22>
 8002dae:	f894 147d 	ldrb.w	r1, [r4, #1149]	@ 0x47d
 8002db2:	2900      	cmp	r1, #0
 8002db4:	d047      	beq.n	8002e46 <fdRevTick+0x212>
 8002db6:	f894 10cc 	ldrb.w	r1, [r4, #204]	@ 0xcc
 8002dba:	f894 0475 	ldrb.w	r0, [r4, #1141]	@ 0x475
 8002dbe:	4301      	orrs	r1, r0
 8002dc0:	d041      	beq.n	8002e46 <fdRevTick+0x212>
 8002dc2:	f894 1478 	ldrb.w	r1, [r4, #1144]	@ 0x478
 8002dc6:	2900      	cmp	r1, #0
 8002dc8:	d13d      	bne.n	8002e46 <fdRevTick+0x212>
 8002dca:	f894 142c 	ldrb.w	r1, [r4, #1068]	@ 0x42c
 8002dce:	f8d4 0460 	ldr.w	r0, [r4, #1120]	@ 0x460
 8002dd2:	2904      	cmp	r1, #4
 8002dd4:	eba6 0000 	sub.w	r0, r6, r0
 8002dd8:	460f      	mov	r7, r1
 8002dda:	d0b6      	beq.n	8002d4a <fdRevTick+0x116>
 8002ddc:	f894 1410 	ldrb.w	r1, [r4, #1040]	@ 0x410
 8002de0:	2900      	cmp	r1, #0
 8002de2:	f040 81f4 	bne.w	80031ce <fdRevTick+0x59a>
 8002de6:	494c      	ldr	r1, [pc, #304]	@ (8002f18 <fdRevTick+0x2e4>)
 8002de8:	4288      	cmp	r0, r1
 8002dea:	f0c0 81f4 	bcc.w	80031d6 <fdRevTick+0x5a2>
 8002dee:	494b      	ldr	r1, [pc, #300]	@ (8002f1c <fdRevTick+0x2e8>)
 8002df0:	4288      	cmp	r0, r1
 8002df2:	d208      	bcs.n	8002e06 <fdRevTick+0x1d2>
 8002df4:	4630      	mov	r0, r6
 8002df6:	9200      	str	r2, [sp, #0]
 8002df8:	f7fe ffc0 	bl	8001d7c <fdRevRetryEligible.constprop.0>
 8002dfc:	9a00      	ldr	r2, [sp, #0]
 8002dfe:	4b41      	ldr	r3, [pc, #260]	@ (8002f04 <fdRevTick+0x2d0>)
 8002e00:	2800      	cmp	r0, #0
 8002e02:	f040 82d4 	bne.w	80033ae <fdRevTick+0x77a>
 8002e06:	2001      	movs	r0, #1
 8002e08:	9200      	str	r2, [sp, #0]
 8002e0a:	f7ff f865 	bl	8001ed8 <fdBenchStop>
 8002e0e:	9a00      	ldr	r2, [sp, #0]
 8002e10:	4b3c      	ldr	r3, [pc, #240]	@ (8002f04 <fdRevTick+0x2d0>)
 8002e12:	e720      	b.n	8002c56 <fdRevTick+0x22>
 8002e14:	1e7a      	subs	r2, r7, #1
 8002e16:	2a04      	cmp	r2, #4
 8002e18:	f63f af69 	bhi.w	8002cee <fdRevTick+0xba>
 8002e1c:	a101      	add	r1, pc, #4	@ (adr r1, 8002e24 <fdRevTick+0x1f0>)
 8002e1e:	f851 f022 	ldr.w	pc, [r1, r2, lsl #2]
 8002e22:	bf00      	nop
 8002e24:	08002eb9 	.word	0x08002eb9
 8002e28:	08002f29 	.word	0x08002f29
 8002e2c:	08002e55 	.word	0x08002e55
 8002e30:	08002cef 	.word	0x08002cef
 8002e34:	08002ce1 	.word	0x08002ce1
 8002e38:	2007      	movs	r0, #7
 8002e3a:	9200      	str	r2, [sp, #0]
 8002e3c:	f7fe ffe8 	bl	8001e10 <fdBenchFinish.part.0>
 8002e40:	4b30      	ldr	r3, [pc, #192]	@ (8002f04 <fdRevTick+0x2d0>)
 8002e42:	9a00      	ldr	r2, [sp, #0]
 8002e44:	e76f      	b.n	8002d26 <fdRevTick+0xf2>
 8002e46:	2004      	movs	r0, #4
 8002e48:	9200      	str	r2, [sp, #0]
 8002e4a:	f7ff f845 	bl	8001ed8 <fdBenchStop>
 8002e4e:	9a00      	ldr	r2, [sp, #0]
 8002e50:	4b2c      	ldr	r3, [pc, #176]	@ (8002f04 <fdRevTick+0x2d0>)
 8002e52:	e700      	b.n	8002c56 <fdRevTick+0x22>
 8002e54:	f8d4 2420 	ldr.w	r2, [r4, #1056]	@ 0x420
 8002e58:	f8d4 1428 	ldr.w	r1, [r4, #1064]	@ 0x428
 8002e5c:	eba8 0202 	sub.w	r2, r8, r2
 8002e60:	428a      	cmp	r2, r1
 8002e62:	f4ff af44 	bcc.w	8002cee <fdRevTick+0xba>
 8002e66:	f894 242e 	ldrb.w	r2, [r4, #1070]	@ 0x42e
 8002e6a:	2a05      	cmp	r2, #5
 8002e6c:	f200 81ea 	bhi.w	8003244 <fdRevTick+0x610>
 8002e70:	4a2b      	ldr	r2, [pc, #172]	@ (8002f20 <fdRevTick+0x2ec>)
 8002e72:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 8002e76:	2301      	movs	r3, #1
 8002e78:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8002e7c:	6913      	ldr	r3, [r2, #16]
 8002e7e:	6990      	ldr	r0, [r2, #24]
 8002e80:	2102      	movs	r1, #2
 8002e82:	f8a4 142c 	strh.w	r1, [r4, #1068]	@ 0x42c
 8002e86:	b968      	cbnz	r0, 8002ea4 <fdRevTick+0x270>
 8002e88:	2b17      	cmp	r3, #23
 8002e8a:	d80b      	bhi.n	8002ea4 <fdRevTick+0x270>
 8002e8c:	68d0      	ldr	r0, [r2, #12]
 8002e8e:	ebc3 1303 	rsb	r3, r3, r3, lsl #4
 8002e92:	eb02 0383 	add.w	r3, r2, r3, lsl #2
 8002e96:	eba8 0000 	sub.w	r0, r8, r0
 8002e9a:	6258      	str	r0, [r3, #36]	@ 0x24
 8002e9c:	6599      	str	r1, [r3, #88]	@ 0x58
 8002e9e:	f04f 33ff 	mov.w	r3, #4294967295	@ 0xffffffff
 8002ea2:	6113      	str	r3, [r2, #16]
 8002ea4:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 8002ea8:	2300      	movs	r3, #0
 8002eaa:	6253      	str	r3, [r2, #36]	@ 0x24
 8002eac:	f7ff fc7e 	bl	80027ac <commutate>
 8002eb0:	2300      	movs	r3, #0
 8002eb2:	f884 30de 	strb.w	r3, [r4, #222]	@ 0xde
 8002eb6:	e71d      	b.n	8002cf4 <fdRevTick+0xc0>
 8002eb8:	f8d4 1414 	ldr.w	r1, [r4, #1044]	@ 0x414
 8002ebc:	1a71      	subs	r1, r6, r1
 8002ebe:	f894 2413 	ldrb.w	r2, [r4, #1043]	@ 0x413
 8002ec2:	2a01      	cmp	r2, #1
 8002ec4:	f000 8140 	beq.w	8003148 <fdRevTick+0x514>
 8002ec8:	4b16      	ldr	r3, [pc, #88]	@ (8002f24 <fdRevTick+0x2f0>)
 8002eca:	4299      	cmp	r1, r3
 8002ecc:	f4ff af12 	bcc.w	8002cf4 <fdRevTick+0xc0>
 8002ed0:	4b13      	ldr	r3, [pc, #76]	@ (8002f20 <fdRevTick+0x2ec>)
 8002ed2:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 8002ed6:	3a01      	subs	r2, #1
 8002ed8:	6919      	ldr	r1, [r3, #16]
 8002eda:	f884 2413 	strb.w	r2, [r4, #1043]	@ 0x413
 8002ede:	699a      	ldr	r2, [r3, #24]
 8002ee0:	4640      	mov	r0, r8
 8002ee2:	2a00      	cmp	r2, #0
 8002ee4:	d1de      	bne.n	8002ea4 <fdRevTick+0x270>
 8002ee6:	2917      	cmp	r1, #23
 8002ee8:	d8dc      	bhi.n	8002ea4 <fdRevTick+0x270>
 8002eea:	68da      	ldr	r2, [r3, #12]
 8002eec:	1a82      	subs	r2, r0, r2
 8002eee:	203c      	movs	r0, #60	@ 0x3c
 8002ef0:	fb00 3101 	mla	r1, r0, r1, r3
 8002ef4:	624a      	str	r2, [r1, #36]	@ 0x24
 8002ef6:	2201      	movs	r2, #1
 8002ef8:	658a      	str	r2, [r1, #88]	@ 0x58
 8002efa:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8002efe:	611a      	str	r2, [r3, #16]
 8002f00:	e7d0      	b.n	8002ea4 <fdRevTick+0x270>
 8002f02:	bf00      	nop
 8002f04:	20000000 	.word	0x20000000
 8002f08:	20001184 	.word	0x20001184
 8002f0c:	e0001000 	.word	0xe0001000
 8002f10:	0030d400 	.word	0x0030d400
 8002f14:	016e3600 	.word	0x016e3600
 8002f18:	03b82600 	.word	0x03b82600
 8002f1c:	03e8fa00 	.word	0x03e8fa00
 8002f20:	20000b90 	.word	0x20000b90
 8002f24:	00493e00 	.word	0x00493e00
 8002f28:	f8d4 2414 	ldr.w	r2, [r4, #1044]	@ 0x414
 8002f2c:	f8d4 0418 	ldr.w	r0, [r4, #1048]	@ 0x418
 8002f30:	1ab2      	subs	r2, r6, r2
 8002f32:	4282      	cmp	r2, r0
 8002f34:	4682      	mov	sl, r0
 8002f36:	f4ff aeda 	bcc.w	8002cee <fdRevTick+0xba>
 8002f3a:	f894 2412 	ldrb.w	r2, [r4, #1042]	@ 0x412
 8002f3e:	49b7      	ldr	r1, [pc, #732]	@ (800321c <fdRevTick+0x5e8>)
 8002f40:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 8002f44:	2aff      	cmp	r2, #255	@ 0xff
 8002f46:	bf1c      	itt	ne
 8002f48:	3201      	addne	r2, #1
 8002f4a:	f884 2412 	strbne.w	r2, [r4, #1042]	@ 0x412
 8002f4e:	2200      	movs	r2, #0
 8002f50:	458a      	cmp	sl, r1
 8002f52:	f8a4 242d 	strh.w	r2, [r4, #1069]	@ 0x42d
 8002f56:	f8c4 2424 	str.w	r2, [r4, #1060]	@ 0x424
 8002f5a:	f8c4 243c 	str.w	r2, [r4, #1084]	@ 0x43c
 8002f5e:	bf04      	itt	eq
 8002f60:	4aaf      	ldreq	r2, [pc, #700]	@ (8003220 <fdRevTick+0x5ec>)
 8002f62:	f8c4 2418 	streq.w	r2, [r4, #1048]	@ 0x418
 8002f66:	f894 2439 	ldrb.w	r2, [r4, #1081]	@ 0x439
 8002f6a:	2a00      	cmp	r2, #0
 8002f6c:	f000 8146 	beq.w	80031fc <fdRevTick+0x5c8>
 8002f70:	3a01      	subs	r2, #1
 8002f72:	b2d2      	uxtb	r2, r2
 8002f74:	f884 2439 	strb.w	r2, [r4, #1081]	@ 0x439
 8002f78:	2a00      	cmp	r2, #0
 8002f7a:	f000 8222 	beq.w	80033c2 <fdRevTick+0x78e>
 8002f7e:	2a06      	cmp	r2, #6
 8002f80:	bf28      	it	cs
 8002f82:	2206      	movcs	r2, #6
 8002f84:	49a7      	ldr	r1, [pc, #668]	@ (8003224 <fdRevTick+0x5f0>)
 8002f86:	3a01      	subs	r2, #1
 8002f88:	f831 2012 	ldrh.w	r2, [r1, r2, lsl #1]
 8002f8c:	eb02 0282 	add.w	r2, r2, r2, lsl #2
 8002f90:	0152      	lsls	r2, r2, #5
 8002f92:	f8c4 2418 	str.w	r2, [r4, #1048]	@ 0x418
 8002f96:	2201      	movs	r2, #1
 8002f98:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 8002f9c:	4aa2      	ldr	r2, [pc, #648]	@ (8003228 <fdRevTick+0x5f4>)
 8002f9e:	6911      	ldr	r1, [r2, #16]
 8002fa0:	6990      	ldr	r0, [r2, #24]
 8002fa2:	b970      	cbnz	r0, 8002fc2 <fdRevTick+0x38e>
 8002fa4:	2917      	cmp	r1, #23
 8002fa6:	d80c      	bhi.n	8002fc2 <fdRevTick+0x38e>
 8002fa8:	68d0      	ldr	r0, [r2, #12]
 8002faa:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 8002fae:	eb02 0181 	add.w	r1, r2, r1, lsl #2
 8002fb2:	eba8 0000 	sub.w	r0, r8, r0
 8002fb6:	6248      	str	r0, [r1, #36]	@ 0x24
 8002fb8:	2003      	movs	r0, #3
 8002fba:	6588      	str	r0, [r1, #88]	@ 0x58
 8002fbc:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8002fc0:	6111      	str	r1, [r2, #16]
 8002fc2:	6b99      	ldr	r1, [r3, #56]	@ 0x38
 8002fc4:	f894 247e 	ldrb.w	r2, [r4, #1150]	@ 0x47e
 8002fc8:	3101      	adds	r1, #1
 8002fca:	6399      	str	r1, [r3, #56]	@ 0x38
 8002fcc:	3201      	adds	r2, #1
 8002fce:	2300      	movs	r3, #0
 8002fd0:	2001      	movs	r0, #1
 8002fd2:	f884 247e 	strb.w	r2, [r4, #1150]	@ 0x47e
 8002fd6:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 8002fda:	f004 febd 	bl	8007d58 <adcZcdResetAcquisition>
 8002fde:	f7ff f953 	bl	8002288 <stuckRotorGuardInput>
 8002fe2:	2800      	cmp	r0, #0
 8002fe4:	f43f af5e 	beq.w	8002ea4 <fdRevTick+0x270>
 8002fe8:	f7fe ffec 	bl	8001fc4 <fdBenchService>
 8002fec:	e682      	b.n	8002cf4 <fdRevTick+0xc0>
 8002fee:	1e7a      	subs	r2, r7, #1
 8002ff0:	2a04      	cmp	r2, #4
 8002ff2:	f63f ae7c 	bhi.w	8002cee <fdRevTick+0xba>
 8002ff6:	a001      	add	r0, pc, #4	@ (adr r0, 8002ffc <fdRevTick+0x3c8>)
 8002ff8:	f850 f022 	ldr.w	pc, [r0, r2, lsl #2]
 8002ffc:	08002ebf 	.word	0x08002ebf
 8003000:	08002cef 	.word	0x08002cef
 8003004:	08002e55 	.word	0x08002e55
 8003008:	08002cef 	.word	0x08002cef
 800300c:	08002ce7 	.word	0x08002ce7
 8003010:	2301      	movs	r3, #1
 8003012:	9a01      	ldr	r2, [sp, #4]
 8003014:	f884 3410 	strb.w	r3, [r4, #1040]	@ 0x410
 8003018:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 800301c:	2f01      	cmp	r7, #1
 800301e:	f04f 0305 	mov.w	r3, #5
 8003022:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 8003026:	f884 242e 	strb.w	r2, [r4, #1070]	@ 0x42e
 800302a:	f8c4 243c 	str.w	r2, [r4, #1084]	@ 0x43c
 800302e:	f8c4 2424 	str.w	r2, [r4, #1060]	@ 0x424
 8003032:	f8a4 342c 	strh.w	r3, [r4, #1068]	@ 0x42c
 8003036:	d132      	bne.n	800309e <fdRevTick+0x46a>
 8003038:	497b      	ldr	r1, [pc, #492]	@ (8003228 <fdRevTick+0x5f4>)
 800303a:	690b      	ldr	r3, [r1, #16]
 800303c:	698a      	ldr	r2, [r1, #24]
 800303e:	b972      	cbnz	r2, 800305e <fdRevTick+0x42a>
 8003040:	2b17      	cmp	r3, #23
 8003042:	d80c      	bhi.n	800305e <fdRevTick+0x42a>
 8003044:	68ca      	ldr	r2, [r1, #12]
 8003046:	ebc3 1303 	rsb	r3, r3, r3, lsl #4
 800304a:	eb01 0383 	add.w	r3, r1, r3, lsl #2
 800304e:	eba8 0202 	sub.w	r2, r8, r2
 8003052:	625a      	str	r2, [r3, #36]	@ 0x24
 8003054:	2201      	movs	r2, #1
 8003056:	659a      	str	r2, [r3, #88]	@ 0x58
 8003058:	f04f 33ff 	mov.w	r3, #4294967295	@ 0xffffffff
 800305c:	610b      	str	r3, [r1, #16]
 800305e:	f8d4 3460 	ldr.w	r3, [r4, #1120]	@ 0x460
 8003062:	eba8 0303 	sub.w	r3, r8, r3
 8003066:	f8c4 3448 	str.w	r3, [r4, #1096]	@ 0x448
 800306a:	f006 faaf 	bl	80095cc <allOff>
 800306e:	4a6f      	ldr	r2, [pc, #444]	@ (800322c <fdRevTick+0x5f8>)
 8003070:	2300      	movs	r3, #0
 8003072:	f8a4 30d0 	strh.w	r3, [r4, #208]	@ 0xd0
 8003076:	f8a4 30d4 	strh.w	r3, [r4, #212]	@ 0xd4
 800307a:	f8a4 30d2 	strh.w	r3, [r4, #210]	@ 0xd2
 800307e:	f8a4 30ce 	strh.w	r3, [r4, #206]	@ 0xce
 8003082:	2004      	movs	r0, #4
 8003084:	6353      	str	r3, [r2, #52]	@ 0x34
 8003086:	6393      	str	r3, [r2, #56]	@ 0x38
 8003088:	63d3      	str	r3, [r2, #60]	@ 0x3c
 800308a:	6413      	str	r3, [r2, #64]	@ 0x40
 800308c:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 8003090:	f004 fe62 	bl	8007d58 <adcZcdResetAcquisition>
 8003094:	4b66      	ldr	r3, [pc, #408]	@ (8003230 <fdRevTick+0x5fc>)
 8003096:	685b      	ldr	r3, [r3, #4]
 8003098:	f8c4 3414 	str.w	r3, [r4, #1044]	@ 0x414
 800309c:	e62a      	b.n	8002cf4 <fdRevTick+0xc0>
 800309e:	4962      	ldr	r1, [pc, #392]	@ (8003228 <fdRevTick+0x5f4>)
 80030a0:	690a      	ldr	r2, [r1, #16]
 80030a2:	698b      	ldr	r3, [r1, #24]
 80030a4:	2b00      	cmp	r3, #0
 80030a6:	d1da      	bne.n	800305e <fdRevTick+0x42a>
 80030a8:	2a17      	cmp	r2, #23
 80030aa:	d8d8      	bhi.n	800305e <fdRevTick+0x42a>
 80030ac:	68cb      	ldr	r3, [r1, #12]
 80030ae:	203c      	movs	r0, #60	@ 0x3c
 80030b0:	fb00 1202 	mla	r2, r0, r2, r1
 80030b4:	eba8 0303 	sub.w	r3, r8, r3
 80030b8:	6253      	str	r3, [r2, #36]	@ 0x24
 80030ba:	2307      	movs	r3, #7
 80030bc:	6593      	str	r3, [r2, #88]	@ 0x58
 80030be:	f04f 33ff 	mov.w	r3, #4294967295	@ 0xffffffff
 80030c2:	610b      	str	r3, [r1, #16]
 80030c4:	e7cb      	b.n	800305e <fdRevTick+0x42a>
 80030c6:	2238      	movs	r2, #56	@ 0x38
 80030c8:	2100      	movs	r1, #0
 80030ca:	485a      	ldr	r0, [pc, #360]	@ (8003234 <fdRevTick+0x600>)
 80030cc:	f006 fd10 	bl	8009af0 <memset>
 80030d0:	2101      	movs	r1, #1
 80030d2:	4855      	ldr	r0, [pc, #340]	@ (8003228 <fdRevTick+0x5f4>)
 80030d4:	f884 142c 	strb.w	r1, [r4, #1068]	@ 0x42c
 80030d8:	f884 1410 	strb.w	r1, [r4, #1040]	@ 0x410
 80030dc:	494f      	ldr	r1, [pc, #316]	@ (800321c <fdRevTick+0x5e8>)
 80030de:	6902      	ldr	r2, [r0, #16]
 80030e0:	f8c4 1418 	str.w	r1, [r4, #1048]	@ 0x418
 80030e4:	f8d0 c018 	ldr.w	ip, [r0, #24]
 80030e8:	4b53      	ldr	r3, [pc, #332]	@ (8003238 <fdRevTick+0x604>)
 80030ea:	f8c4 940c 	str.w	r9, [r4, #1036]	@ 0x40c
 80030ee:	21ff      	movs	r1, #255	@ 0xff
 80030f0:	f884 1437 	strb.w	r1, [r4, #1079]	@ 0x437
 80030f4:	2102      	movs	r1, #2
 80030f6:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 80030fa:	f884 1413 	strb.w	r1, [r4, #1043]	@ 0x413
 80030fe:	f1bc 0f00 	cmp.w	ip, #0
 8003102:	d10e      	bne.n	8003122 <fdRevTick+0x4ee>
 8003104:	2a17      	cmp	r2, #23
 8003106:	d80c      	bhi.n	8003122 <fdRevTick+0x4ee>
 8003108:	68c1      	ldr	r1, [r0, #12]
 800310a:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 800310e:	eb00 0282 	add.w	r2, r0, r2, lsl #2
 8003112:	eba8 0101 	sub.w	r1, r8, r1
 8003116:	6251      	str	r1, [r2, #36]	@ 0x24
 8003118:	2108      	movs	r1, #8
 800311a:	6591      	str	r1, [r2, #88]	@ 0x58
 800311c:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003120:	6102      	str	r2, [r0, #16]
 8003122:	f8d4 2460 	ldr.w	r2, [r4, #1120]	@ 0x460
 8003126:	2106      	movs	r1, #6
 8003128:	eba8 0202 	sub.w	r2, r8, r2
 800312c:	f8c4 244c 	str.w	r2, [r4, #1100]	@ 0x44c
 8003130:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 8003134:	2200      	movs	r2, #0
 8003136:	f242 7110 	movw	r1, #10000	@ 0x2710
 800313a:	2004      	movs	r0, #4
 800313c:	6119      	str	r1, [r3, #16]
 800313e:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 8003142:	f004 fe09 	bl	8007d58 <adcZcdResetAcquisition>
 8003146:	e6ad      	b.n	8002ea4 <fdRevTick+0x270>
 8003148:	483c      	ldr	r0, [pc, #240]	@ (800323c <fdRevTick+0x608>)
 800314a:	4281      	cmp	r1, r0
 800314c:	f4ff add2 	bcc.w	8002cf4 <fdRevTick+0xc0>
 8003150:	4935      	ldr	r1, [pc, #212]	@ (8003228 <fdRevTick+0x5f4>)
 8003152:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 8003156:	2000      	movs	r0, #0
 8003158:	f884 0413 	strb.w	r0, [r4, #1043]	@ 0x413
 800315c:	6908      	ldr	r0, [r1, #16]
 800315e:	f8d1 c018 	ldr.w	ip, [r1, #24]
 8003162:	f8c4 8414 	str.w	r8, [r4, #1044]	@ 0x414
 8003166:	f04f 0e02 	mov.w	lr, #2
 800316a:	f8c4 8408 	str.w	r8, [r4, #1032]	@ 0x408
 800316e:	f884 e42c 	strb.w	lr, [r4, #1068]	@ 0x42c
 8003172:	f1bc 0f00 	cmp.w	ip, #0
 8003176:	d10f      	bne.n	8003198 <fdRevTick+0x564>
 8003178:	2817      	cmp	r0, #23
 800317a:	d80d      	bhi.n	8003198 <fdRevTick+0x564>
 800317c:	f8d1 c00c 	ldr.w	ip, [r1, #12]
 8003180:	f04f 0e3c 	mov.w	lr, #60	@ 0x3c
 8003184:	fb0e 1000 	mla	r0, lr, r0, r1
 8003188:	eba8 0c0c 	sub.w	ip, r8, ip
 800318c:	f8c0 c024 	str.w	ip, [r0, #36]	@ 0x24
 8003190:	6582      	str	r2, [r0, #88]	@ 0x58
 8003192:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003196:	610a      	str	r2, [r1, #16]
 8003198:	f006 fa18 	bl	80095cc <allOff>
 800319c:	4924      	ldr	r1, [pc, #144]	@ (8003230 <fdRevTick+0x5fc>)
 800319e:	4b26      	ldr	r3, [pc, #152]	@ (8003238 <fdRevTick+0x604>)
 80031a0:	6848      	ldr	r0, [r1, #4]
 80031a2:	684a      	ldr	r2, [r1, #4]
 80031a4:	1a12      	subs	r2, r2, r0
 80031a6:	f5b2 7f48 	cmp.w	r2, #800	@ 0x320
 80031aa:	d3fa      	bcc.n	80031a2 <fdRevTick+0x56e>
 80031ac:	f894 2410 	ldrb.w	r2, [r4, #1040]	@ 0x410
 80031b0:	2101      	movs	r1, #1
 80031b2:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 80031b6:	2a00      	cmp	r2, #0
 80031b8:	f43f ae74 	beq.w	8002ea4 <fdRevTick+0x270>
 80031bc:	f893 2718 	ldrb.w	r2, [r3, #1816]	@ 0x718
 80031c0:	2a00      	cmp	r2, #0
 80031c2:	bf0c      	ite	eq
 80031c4:	2204      	moveq	r2, #4
 80031c6:	2202      	movne	r2, #2
 80031c8:	f883 2719 	strb.w	r2, [r3, #1817]	@ 0x719
 80031cc:	e66a      	b.n	8002ea4 <fdRevTick+0x270>
 80031ce:	491c      	ldr	r1, [pc, #112]	@ (8003240 <fdRevTick+0x60c>)
 80031d0:	4288      	cmp	r0, r1
 80031d2:	f4bf ae18 	bcs.w	8002e06 <fdRevTick+0x1d2>
 80031d6:	2f01      	cmp	r7, #1
 80031d8:	f000 80b4 	beq.w	8003344 <fdRevTick+0x710>
 80031dc:	f8df c03c 	ldr.w	ip, [pc, #60]	@ 800321c <fdRevTick+0x5e8>
 80031e0:	f894 143b 	ldrb.w	r1, [r4, #1083]	@ 0x43b
 80031e4:	4560      	cmp	r0, ip
 80031e6:	f080 80e6 	bcs.w	80033b6 <fdRevTick+0x782>
 80031ea:	2900      	cmp	r1, #0
 80031ec:	bf14      	ite	ne
 80031ee:	2100      	movne	r1, #0
 80031f0:	f44f 71b4 	moveq.w	r1, #360	@ 0x168
 80031f4:	2f05      	cmp	r7, #5
 80031f6:	bf08      	it	eq
 80031f8:	2100      	moveq	r1, #0
 80031fa:	e5bc      	b.n	8002d76 <fdRevTick+0x142>
 80031fc:	f894 243a 	ldrb.w	r2, [r4, #1082]	@ 0x43a
 8003200:	b942      	cbnz	r2, 8003214 <fdRevTick+0x5e0>
 8003202:	f894 2438 	ldrb.w	r2, [r4, #1080]	@ 0x438
 8003206:	3201      	adds	r2, #1
 8003208:	b2d2      	uxtb	r2, r2
 800320a:	2a02      	cmp	r2, #2
 800320c:	f884 2438 	strb.w	r2, [r4, #1080]	@ 0x438
 8003210:	f000 80dd 	beq.w	80033ce <fdRevTick+0x79a>
 8003214:	2201      	movs	r2, #1
 8003216:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 800321a:	e6bf      	b.n	8002f9c <fdRevTick+0x368>
 800321c:	00493e00 	.word	0x00493e00
 8003220:	0036ee80 	.word	0x0036ee80
 8003224:	08009b8c 	.word	0x08009b8c
 8003228:	20000b90 	.word	0x20000b90
 800322c:	40012c00 	.word	0x40012c00
 8003230:	e0001000 	.word	0xe0001000
 8003234:	2000158c 	.word	0x2000158c
 8003238:	20000000 	.word	0x20000000
 800323c:	00927c00 	.word	0x00927c00
 8003240:	07d1f400 	.word	0x07d1f400
 8003244:	4987      	ldr	r1, [pc, #540]	@ (8003464 <fdRevTick+0x830>)
 8003246:	f8c4 8430 	str.w	r8, [r4, #1072]	@ 0x430
 800324a:	2201      	movs	r2, #1
 800324c:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 8003250:	690a      	ldr	r2, [r1, #16]
 8003252:	f8d1 c018 	ldr.w	ip, [r1, #24]
 8003256:	2004      	movs	r0, #4
 8003258:	f884 042c 	strb.w	r0, [r4, #1068]	@ 0x42c
 800325c:	f1bc 0f00 	cmp.w	ip, #0
 8003260:	d10f      	bne.n	8003282 <fdRevTick+0x64e>
 8003262:	2a17      	cmp	r2, #23
 8003264:	d80d      	bhi.n	8003282 <fdRevTick+0x64e>
 8003266:	f8d1 c00c 	ldr.w	ip, [r1, #12]
 800326a:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 800326e:	eb01 0282 	add.w	r2, r1, r2, lsl #2
 8003272:	eba8 0c0c 	sub.w	ip, r8, ip
 8003276:	f8c2 c024 	str.w	ip, [r2, #36]	@ 0x24
 800327a:	6590      	str	r0, [r2, #88]	@ 0x58
 800327c:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003280:	610a      	str	r2, [r1, #16]
 8003282:	698a      	ldr	r2, [r1, #24]
 8003284:	b9aa      	cbnz	r2, 80032b2 <fdRevTick+0x67e>
 8003286:	690a      	ldr	r2, [r1, #16]
 8003288:	6988      	ldr	r0, [r1, #24]
 800328a:	b970      	cbnz	r0, 80032aa <fdRevTick+0x676>
 800328c:	2a17      	cmp	r2, #23
 800328e:	d80c      	bhi.n	80032aa <fdRevTick+0x676>
 8003290:	68c8      	ldr	r0, [r1, #12]
 8003292:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 8003296:	eb01 0282 	add.w	r2, r1, r2, lsl #2
 800329a:	eba8 0000 	sub.w	r0, r8, r0
 800329e:	6250      	str	r0, [r2, #36]	@ 0x24
 80032a0:	2005      	movs	r0, #5
 80032a2:	6590      	str	r0, [r2, #88]	@ 0x58
 80032a4:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 80032a8:	610a      	str	r2, [r1, #16]
 80032aa:	2200      	movs	r2, #0
 80032ac:	61ca      	str	r2, [r1, #28]
 80032ae:	2201      	movs	r2, #1
 80032b0:	618a      	str	r2, [r1, #24]
 80032b2:	f8d4 2460 	ldr.w	r2, [r4, #1120]	@ 0x460
 80032b6:	496c      	ldr	r1, [pc, #432]	@ (8003468 <fdRevTick+0x834>)
 80032b8:	f8d4 043c 	ldr.w	r0, [r4, #1084]	@ 0x43c
 80032bc:	eba8 0202 	sub.w	r2, r8, r2
 80032c0:	f8c4 2454 	str.w	r2, [r4, #1108]	@ 0x454
 80032c4:	6a9a      	ldr	r2, [r3, #40]	@ 0x28
 80032c6:	eba8 0202 	sub.w	r2, r8, r2
 80032ca:	631a      	str	r2, [r3, #48]	@ 0x30
 80032cc:	2200      	movs	r2, #0
 80032ce:	f884 2474 	strb.w	r2, [r4, #1140]	@ 0x474
 80032d2:	f884 24fe 	strb.w	r2, [r4, #1278]	@ 0x4fe
 80032d6:	fba1 2000 	umull	r2, r0, r1, r0
 80032da:	f8d4 2424 	ldr.w	r2, [r4, #1060]	@ 0x424
 80032de:	fba1 c202 	umull	ip, r2, r1, r2
 80032e2:	0992      	lsrs	r2, r2, #6
 80032e4:	611a      	str	r2, [r3, #16]
 80032e6:	691a      	ldr	r2, [r3, #16]
 80032e8:	f8a4 2510 	strh.w	r2, [r4, #1296]	@ 0x510
 80032ec:	2206      	movs	r2, #6
 80032ee:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 80032f2:	691a      	ldr	r2, [r3, #16]
 80032f4:	f8a4 2504 	strh.w	r2, [r4, #1284]	@ 0x504
 80032f8:	691a      	ldr	r2, [r3, #16]
 80032fa:	f8a4 2506 	strh.w	r2, [r4, #1286]	@ 0x506
 80032fe:	691a      	ldr	r2, [r3, #16]
 8003300:	f8a4 2508 	strh.w	r2, [r4, #1288]	@ 0x508
 8003304:	691a      	ldr	r2, [r3, #16]
 8003306:	f8a4 250a 	strh.w	r2, [r4, #1290]	@ 0x50a
 800330a:	691a      	ldr	r2, [r3, #16]
 800330c:	f8a4 250c 	strh.w	r2, [r4, #1292]	@ 0x50c
 8003310:	691a      	ldr	r2, [r3, #16]
 8003312:	f8a4 250e 	strh.w	r2, [r4, #1294]	@ 0x50e
 8003316:	691a      	ldr	r2, [r3, #16]
 8003318:	f8c4 2500 	str.w	r2, [r4, #1280]	@ 0x500
 800331c:	691b      	ldr	r3, [r3, #16]
 800331e:	f8c4 3514 	str.w	r3, [r4, #1300]	@ 0x514
 8003322:	0980      	lsrs	r0, r0, #6
 8003324:	f004 fd00 	bl	8007d28 <adcZcdSeedHandoff>
 8003328:	f8d4 3420 	ldr.w	r3, [r4, #1056]	@ 0x420
 800332c:	494e      	ldr	r1, [pc, #312]	@ (8003468 <fdRevTick+0x834>)
 800332e:	eba8 0303 	sub.w	r3, r8, r3
 8003332:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 8003336:	fba1 1303 	umull	r1, r3, r1, r3
 800333a:	099b      	lsrs	r3, r3, #6
 800333c:	6253      	str	r3, [r2, #36]	@ 0x24
 800333e:	f7ff fa35 	bl	80027ac <commutate>
 8003342:	e4d7      	b.n	8002cf4 <fdRevTick+0xc0>
 8003344:	f894 1413 	ldrb.w	r1, [r4, #1043]	@ 0x413
 8003348:	2901      	cmp	r1, #1
 800334a:	d02d      	beq.n	80033a8 <fdRevTick+0x774>
 800334c:	f8d4 1414 	ldr.w	r1, [r4, #1044]	@ 0x414
 8003350:	4846      	ldr	r0, [pc, #280]	@ (800346c <fdRevTick+0x838>)
 8003352:	1a71      	subs	r1, r6, r1
 8003354:	4281      	cmp	r1, r0
 8003356:	d827      	bhi.n	80033a8 <fdRevTick+0x774>
 8003358:	4843      	ldr	r0, [pc, #268]	@ (8003468 <fdRevTick+0x834>)
 800335a:	fba0 0101 	umull	r0, r1, r0, r1
 800335e:	09c9      	lsrs	r1, r1, #7
 8003360:	4843      	ldr	r0, [pc, #268]	@ (8003470 <fdRevTick+0x83c>)
 8003362:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 8003366:	fb0c f101 	mul.w	r1, ip, r1
 800336a:	fba0 0101 	umull	r0, r1, r0, r1
 800336e:	0b49      	lsrs	r1, r1, #13
 8003370:	31d2      	adds	r1, #210	@ 0xd2
 8003372:	b289      	uxth	r1, r1
 8003374:	e4ff      	b.n	8002d76 <fdRevTick+0x142>
 8003376:	f8d4 1430 	ldr.w	r1, [r4, #1072]	@ 0x430
 800337a:	eba6 0c01 	sub.w	ip, r6, r1
 800337e:	493d      	ldr	r1, [pc, #244]	@ (8003474 <fdRevTick+0x840>)
 8003380:	458c      	cmp	ip, r1
 8003382:	d220      	bcs.n	80033c6 <fdRevTick+0x792>
 8003384:	493c      	ldr	r1, [pc, #240]	@ (8003478 <fdRevTick+0x844>)
 8003386:	458e      	cmp	lr, r1
 8003388:	d35a      	bcc.n	8003440 <fdRevTick+0x80c>
 800338a:	f8df e0f0 	ldr.w	lr, [pc, #240]	@ 800347c <fdRevTick+0x848>
 800338e:	8819      	ldrh	r1, [r3, #0]
 8003390:	45f4      	cmp	ip, lr
 8003392:	d223      	bcs.n	80033dc <fdRevTick+0x7a8>
 8003394:	f8df c0e8 	ldr.w	ip, [pc, #232]	@ 8003480 <fdRevTick+0x84c>
 8003398:	4560      	cmp	r0, ip
 800339a:	bf38      	it	cc
 800339c:	f44f 71b4 	movcc.w	r1, #360	@ 0x168
 80033a0:	e4e9      	b.n	8002d76 <fdRevTick+0x142>
 80033a2:	2f00      	cmp	r7, #0
 80033a4:	f47f ace6 	bne.w	8002d74 <fdRevTick+0x140>
 80033a8:	f44f 71b4 	mov.w	r1, #360	@ 0x168
 80033ac:	e4e3      	b.n	8002d76 <fdRevTick+0x142>
 80033ae:	2f01      	cmp	r7, #1
 80033b0:	d0c8      	beq.n	8003344 <fdRevTick+0x710>
 80033b2:	f894 143b 	ldrb.w	r1, [r4, #1083]	@ 0x43b
 80033b6:	2900      	cmp	r1, #0
 80033b8:	bf14      	ite	ne
 80033ba:	2100      	movne	r1, #0
 80033bc:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 80033c0:	e718      	b.n	80031f4 <fdRevTick+0x5c0>
 80033c2:	4a30      	ldr	r2, [pc, #192]	@ (8003484 <fdRevTick+0x850>)
 80033c4:	e5e5      	b.n	8002f92 <fdRevTick+0x35e>
 80033c6:	2101      	movs	r1, #1
 80033c8:	f884 143b 	strb.w	r1, [r4, #1083]	@ 0x43b
 80033cc:	e4d2      	b.n	8002d74 <fdRevTick+0x140>
 80033ce:	f44f 7283 	mov.w	r2, #262	@ 0x106
 80033d2:	f8a4 2439 	strh.w	r2, [r4, #1081]	@ 0x439
 80033d6:	4a2c      	ldr	r2, [pc, #176]	@ (8003488 <fdRevTick+0x854>)
 80033d8:	e5db      	b.n	8002f92 <fdRevTick+0x35e>
 80033da:	8819      	ldrh	r1, [r3, #0]
 80033dc:	f8df e0ac 	ldr.w	lr, [pc, #172]	@ 800348c <fdRevTick+0x858>
 80033e0:	f1ac 7cb7 	sub.w	ip, ip, #23986176	@ 0x16e0000
 80033e4:	f5ac 5c58 	sub.w	ip, ip, #13824	@ 0x3600
 80033e8:	45f4      	cmp	ip, lr
 80033ea:	d220      	bcs.n	800342e <fdRevTick+0x7fa>
 80033ec:	885e      	ldrh	r6, [r3, #2]
 80033ee:	428e      	cmp	r6, r1
 80033f0:	46b0      	mov	r8, r6
 80033f2:	d214      	bcs.n	800341e <fdRevTick+0x7ea>
 80033f4:	4e22      	ldr	r6, [pc, #136]	@ (8003480 <fdRevTick+0x84c>)
 80033f6:	42b0      	cmp	r0, r6
 80033f8:	d3d3      	bcc.n	80033a2 <fdRevTick+0x76e>
 80033fa:	2f00      	cmp	r7, #0
 80033fc:	f47f acba 	bne.w	8002d74 <fdRevTick+0x140>
 8003400:	ebae 0e0c 	sub.w	lr, lr, ip
 8003404:	eba1 0c08 	sub.w	ip, r1, r8
 8003408:	4821      	ldr	r0, [pc, #132]	@ (8003490 <fdRevTick+0x85c>)
 800340a:	21a0      	movs	r1, #160	@ 0xa0
 800340c:	fbbe f1f1 	udiv	r1, lr, r1
 8003410:	fb0c f101 	mul.w	r1, ip, r1
 8003414:	fbb1 f1f0 	udiv	r1, r1, r0
 8003418:	4441      	add	r1, r8
 800341a:	b289      	uxth	r1, r1
 800341c:	e4ab      	b.n	8002d76 <fdRevTick+0x142>
 800341e:	f8df c060 	ldr.w	ip, [pc, #96]	@ 8003480 <fdRevTick+0x84c>
 8003422:	4560      	cmp	r0, ip
 8003424:	d3bd      	bcc.n	80033a2 <fdRevTick+0x76e>
 8003426:	2f00      	cmp	r7, #0
 8003428:	f47f aca4 	bne.w	8002d74 <fdRevTick+0x140>
 800342c:	e4a3      	b.n	8002d76 <fdRevTick+0x142>
 800342e:	4914      	ldr	r1, [pc, #80]	@ (8003480 <fdRevTick+0x84c>)
 8003430:	4288      	cmp	r0, r1
 8003432:	f4bf ac9f 	bcs.w	8002d74 <fdRevTick+0x140>
 8003436:	f894 143b 	ldrb.w	r1, [r4, #1083]	@ 0x43b
 800343a:	2900      	cmp	r1, #0
 800343c:	d0b4      	beq.n	80033a8 <fdRevTick+0x774>
 800343e:	e499      	b.n	8002d74 <fdRevTick+0x140>
 8003440:	490f      	ldr	r1, [pc, #60]	@ (8003480 <fdRevTick+0x84c>)
 8003442:	4288      	cmp	r0, r1
 8003444:	d3b0      	bcc.n	80033a8 <fdRevTick+0x774>
 8003446:	f06f 0127 	mvn.w	r1, #39	@ 0x27
 800344a:	fb01 f10e 	mul.w	r1, r1, lr
 800344e:	f101 61f4 	add.w	r1, r1, #127926272	@ 0x7a00000
 8003452:	4809      	ldr	r0, [pc, #36]	@ (8003478 <fdRevTick+0x844>)
 8003454:	f501 3190 	add.w	r1, r1, #73728	@ 0x12000
 8003458:	fbb1 f1f0 	udiv	r1, r1, r0
 800345c:	f501 71b4 	add.w	r1, r1, #360	@ 0x168
 8003460:	b289      	uxth	r1, r1
 8003462:	e488      	b.n	8002d76 <fdRevTick+0x142>
 8003464:	20000b90 	.word	0x20000b90
 8003468:	cccccccd 	.word	0xcccccccd
 800346c:	00186a9f 	.word	0x00186a9f
 8003470:	d1b71759 	.word	0xd1b71759
 8003474:	09896800 	.word	0x09896800
 8003478:	0030d400 	.word	0x0030d400
 800347c:	016e3600 	.word	0x016e3600
 8003480:	00493e00 	.word	0x00493e00
 8003484:	0036ee80 	.word	0x0036ee80
 8003488:	00271000 	.word	0x00271000
 800348c:	02dc6c00 	.word	0x02dc6c00
 8003490:	000493e0 	.word	0x000493e0
