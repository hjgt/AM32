
test-firmware/fd6288-S4U-ADJACENT-20261028/S4U_ADJACENT.elf:     file format elf32-littlearm


Disassembly of section .text:

08002c68 <fdRevTick>:
 8002c68:	e92d 41f0 	stmdb	sp!, {r4, r5, r6, r7, r8, lr}
 8002c6c:	b082      	sub	sp, #8
 8002c6e:	f3ef 8510 	mrs	r5, PRIMASK
 8002c72:	b672      	cpsid	i
 8002c74:	f3ef 8210 	mrs	r2, PRIMASK
 8002c78:	b672      	cpsid	i
 8002c7a:	4b8b      	ldr	r3, [pc, #556]	@ (8002ea8 <fdRevTick+0x240>)
 8002c7c:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002c80:	b119      	cbz	r1, 8002c8a <fdRevTick+0x22>
 8002c82:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8002c86:	2900      	cmp	r1, #0
 8002c88:	d059      	beq.n	8002d3e <fdRevTick+0xd6>
 8002c8a:	f382 8810 	msr	PRIMASK, r2
 8002c8e:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8002c92:	2900      	cmp	r1, #0
 8002c94:	d14e      	bne.n	8002d34 <fdRevTick+0xcc>
 8002c96:	f8d3 2674 	ldr.w	r2, [r3, #1652]	@ 0x674
 8002c9a:	2a00      	cmp	r2, #0
 8002c9c:	d04a      	beq.n	8002d34 <fdRevTick+0xcc>
 8002c9e:	4c83      	ldr	r4, [pc, #524]	@ (8002eac <fdRevTick+0x244>)
 8002ca0:	f894 20cc 	ldrb.w	r2, [r4, #204]	@ 0xcc
 8002ca4:	2a00      	cmp	r2, #0
 8002ca6:	d045      	beq.n	8002d34 <fdRevTick+0xcc>
 8002ca8:	f8b4 2470 	ldrh.w	r2, [r4, #1136]	@ 0x470
 8002cac:	2a00      	cmp	r2, #0
 8002cae:	d041      	beq.n	8002d34 <fdRevTick+0xcc>
 8002cb0:	f894 2474 	ldrb.w	r2, [r4, #1140]	@ 0x474
 8002cb4:	2a00      	cmp	r2, #0
 8002cb6:	d03d      	beq.n	8002d34 <fdRevTick+0xcc>
 8002cb8:	4a7d      	ldr	r2, [pc, #500]	@ (8002eb0 <fdRevTick+0x248>)
 8002cba:	f8d4 643c 	ldr.w	r6, [r4, #1084]	@ 0x43c
 8002cbe:	6852      	ldr	r2, [r2, #4]
 8002cc0:	4617      	mov	r7, r2
 8002cc2:	f894 245c 	ldrb.w	r2, [r4, #1116]	@ 0x45c
 8002cc6:	2a02      	cmp	r2, #2
 8002cc8:	f000 80fa 	beq.w	8002ec0 <fdRevTick+0x258>
 8002ccc:	2a03      	cmp	r2, #3
 8002cce:	f000 80bc 	beq.w	8002e4a <fdRevTick+0x1e2>
 8002cd2:	2a05      	cmp	r2, #5
 8002cd4:	f000 8152 	beq.w	8002f7c <fdRevTick+0x314>
 8002cd8:	2a01      	cmp	r2, #1
 8002cda:	f040 80e0 	bne.w	8002e9e <fdRevTick+0x236>
 8002cde:	f894 1442 	ldrb.w	r1, [r4, #1090]	@ 0x442
 8002ce2:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8002ce6:	2901      	cmp	r1, #1
 8002ce8:	eba7 0202 	sub.w	r2, r7, r2
 8002cec:	f000 81e4 	beq.w	80030b8 <fdRevTick+0x450>
 8002cf0:	4b70      	ldr	r3, [pc, #448]	@ (8002eb4 <fdRevTick+0x24c>)
 8002cf2:	429a      	cmp	r2, r3
 8002cf4:	d31e      	bcc.n	8002d34 <fdRevTick+0xcc>
 8002cf6:	4b70      	ldr	r3, [pc, #448]	@ (8002eb8 <fdRevTick+0x250>)
 8002cf8:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8002cfc:	3901      	subs	r1, #1
 8002cfe:	f884 1442 	strb.w	r1, [r4, #1090]	@ 0x442
 8002d02:	6919      	ldr	r1, [r3, #16]
 8002d04:	699a      	ldr	r2, [r3, #24]
 8002d06:	b962      	cbnz	r2, 8002d22 <fdRevTick+0xba>
 8002d08:	2917      	cmp	r1, #23
 8002d0a:	d80a      	bhi.n	8002d22 <fdRevTick+0xba>
 8002d0c:	68da      	ldr	r2, [r3, #12]
 8002d0e:	203c      	movs	r0, #60	@ 0x3c
 8002d10:	fb00 3101 	mla	r1, r0, r1, r3
 8002d14:	1aba      	subs	r2, r7, r2
 8002d16:	624a      	str	r2, [r1, #36]	@ 0x24
 8002d18:	2201      	movs	r2, #1
 8002d1a:	658a      	str	r2, [r1, #88]	@ 0x58
 8002d1c:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8002d20:	611a      	str	r2, [r3, #16]
 8002d22:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 8002d26:	2300      	movs	r3, #0
 8002d28:	6253      	str	r3, [r2, #36]	@ 0x24
 8002d2a:	f7ff fd2d 	bl	8002788 <commutate>
 8002d2e:	2300      	movs	r3, #0
 8002d30:	f884 30de 	strb.w	r3, [r4, #222]	@ 0xde
 8002d34:	f385 8810 	msr	PRIMASK, r5
 8002d38:	b002      	add	sp, #8
 8002d3a:	e8bd 81f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, pc}
 8002d3e:	495c      	ldr	r1, [pc, #368]	@ (8002eb0 <fdRevTick+0x248>)
 8002d40:	4c5a      	ldr	r4, [pc, #360]	@ (8002eac <fdRevTick+0x244>)
 8002d42:	6848      	ldr	r0, [r1, #4]
 8002d44:	f894 147c 	ldrb.w	r1, [r4, #1148]	@ 0x47c
 8002d48:	b9a9      	cbnz	r1, 8002d76 <fdRevTick+0x10e>
 8002d4a:	f894 10c2 	ldrb.w	r1, [r4, #194]	@ 0xc2
 8002d4e:	2900      	cmp	r1, #0
 8002d50:	d166      	bne.n	8002e20 <fdRevTick+0x1b8>
 8002d52:	f8b4 1470 	ldrh.w	r1, [r4, #1136]	@ 0x470
 8002d56:	b9a9      	cbnz	r1, 8002d84 <fdRevTick+0x11c>
 8002d58:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002d5c:	2900      	cmp	r1, #0
 8002d5e:	d166      	bne.n	8002e2e <fdRevTick+0x1c6>
 8002d60:	2100      	movs	r1, #0
 8002d62:	f884 1474 	strb.w	r1, [r4, #1140]	@ 0x474
 8002d66:	f8a4 1430 	strh.w	r1, [r4, #1072]	@ 0x430
 8002d6a:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002d6e:	2101      	movs	r1, #1
 8002d70:	f8c4 1432 	str.w	r1, [r4, #1074]	@ 0x432
 8002d74:	e789      	b.n	8002c8a <fdRevTick+0x22>
 8002d76:	2002      	movs	r0, #2
 8002d78:	9201      	str	r2, [sp, #4]
 8002d7a:	f7ff f863 	bl	8001e44 <fdBenchStop>
 8002d7e:	9a01      	ldr	r2, [sp, #4]
 8002d80:	4b49      	ldr	r3, [pc, #292]	@ (8002ea8 <fdRevTick+0x240>)
 8002d82:	e782      	b.n	8002c8a <fdRevTick+0x22>
 8002d84:	f894 147d 	ldrb.w	r1, [r4, #1149]	@ 0x47d
 8002d88:	2900      	cmp	r1, #0
 8002d8a:	d057      	beq.n	8002e3c <fdRevTick+0x1d4>
 8002d8c:	f894 10cc 	ldrb.w	r1, [r4, #204]	@ 0xcc
 8002d90:	f894 c475 	ldrb.w	ip, [r4, #1141]	@ 0x475
 8002d94:	ea51 010c 	orrs.w	r1, r1, ip
 8002d98:	d050      	beq.n	8002e3c <fdRevTick+0x1d4>
 8002d9a:	f894 1478 	ldrb.w	r1, [r4, #1144]	@ 0x478
 8002d9e:	2900      	cmp	r1, #0
 8002da0:	d14c      	bne.n	8002e3c <fdRevTick+0x1d4>
 8002da2:	f894 e45c 	ldrb.w	lr, [r4, #1116]	@ 0x45c
 8002da6:	f8d4 1428 	ldr.w	r1, [r4, #1064]	@ 0x428
 8002daa:	f1be 0f04 	cmp.w	lr, #4
 8002dae:	eba0 0c01 	sub.w	ip, r0, r1
 8002db2:	f000 81c4 	beq.w	800313e <fdRevTick+0x4d6>
 8002db6:	f894 1440 	ldrb.w	r1, [r4, #1088]	@ 0x440
 8002dba:	2900      	cmp	r1, #0
 8002dbc:	f040 81d5 	bne.w	800316a <fdRevTick+0x502>
 8002dc0:	f1be 0f02 	cmp.w	lr, #2
 8002dc4:	f000 8298 	beq.w	80032f8 <fdRevTick+0x690>
 8002dc8:	493c      	ldr	r1, [pc, #240]	@ (8002ebc <fdRevTick+0x254>)
 8002dca:	458c      	cmp	ip, r1
 8002dcc:	f080 81d1 	bcs.w	8003172 <fdRevTick+0x50a>
 8002dd0:	f1be 0f01 	cmp.w	lr, #1
 8002dd4:	f000 8277 	beq.w	80032c6 <fdRevTick+0x65e>
 8002dd8:	4836      	ldr	r0, [pc, #216]	@ (8002eb4 <fdRevTick+0x24c>)
 8002dda:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 8002dde:	4584      	cmp	ip, r0
 8002de0:	f0c0 82c0 	bcc.w	8003364 <fdRevTick+0x6fc>
 8002de4:	2900      	cmp	r1, #0
 8002de6:	bf14      	ite	ne
 8002de8:	2100      	movne	r1, #0
 8002dea:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 8002dee:	f1be 0f05 	cmp.w	lr, #5
 8002df2:	bf08      	it	eq
 8002df4:	2100      	moveq	r1, #0
 8002df6:	f894 40c3 	ldrb.w	r4, [r4, #195]	@ 0xc3
 8002dfa:	88d8      	ldrh	r0, [r3, #6]
 8002dfc:	b13c      	cbz	r4, 8002e0e <fdRevTick+0x1a6>
 8002dfe:	f9b3 4008 	ldrsh.w	r4, [r3, #8]
 8002e02:	ea24 74e4 	bic.w	r4, r4, r4, asr #31
 8002e06:	b2a4      	uxth	r4, r4
 8002e08:	42a0      	cmp	r0, r4
 8002e0a:	bf28      	it	cs
 8002e0c:	4620      	movcs	r0, r4
 8002e0e:	4281      	cmp	r1, r0
 8002e10:	bf28      	it	cs
 8002e12:	4601      	movcs	r1, r0
 8002e14:	b289      	uxth	r1, r1
 8002e16:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002e1a:	2800      	cmp	r0, #0
 8002e1c:	f47f af35 	bne.w	8002c8a <fdRevTick+0x22>
 8002e20:	2003      	movs	r0, #3
 8002e22:	9201      	str	r2, [sp, #4]
 8002e24:	f7ff f80e 	bl	8001e44 <fdBenchStop>
 8002e28:	9a01      	ldr	r2, [sp, #4]
 8002e2a:	4b1f      	ldr	r3, [pc, #124]	@ (8002ea8 <fdRevTick+0x240>)
 8002e2c:	e72d      	b.n	8002c8a <fdRevTick+0x22>
 8002e2e:	2007      	movs	r0, #7
 8002e30:	9201      	str	r2, [sp, #4]
 8002e32:	f7fe ffa3 	bl	8001d7c <fdBenchFinish.part.0>
 8002e36:	4b1c      	ldr	r3, [pc, #112]	@ (8002ea8 <fdRevTick+0x240>)
 8002e38:	9a01      	ldr	r2, [sp, #4]
 8002e3a:	e791      	b.n	8002d60 <fdRevTick+0xf8>
 8002e3c:	2004      	movs	r0, #4
 8002e3e:	9201      	str	r2, [sp, #4]
 8002e40:	f7ff f800 	bl	8001e44 <fdBenchStop>
 8002e44:	9a01      	ldr	r2, [sp, #4]
 8002e46:	4b18      	ldr	r3, [pc, #96]	@ (8002ea8 <fdRevTick+0x240>)
 8002e48:	e71f      	b.n	8002c8a <fdRevTick+0x22>
 8002e4a:	f8d4 2450 	ldr.w	r2, [r4, #1104]	@ 0x450
 8002e4e:	f8d4 1458 	ldr.w	r1, [r4, #1112]	@ 0x458
 8002e52:	1aba      	subs	r2, r7, r2
 8002e54:	428a      	cmp	r2, r1
 8002e56:	d322      	bcc.n	8002e9e <fdRevTick+0x236>
 8002e58:	f894 245e 	ldrb.w	r2, [r4, #1118]	@ 0x45e
 8002e5c:	2a05      	cmp	r2, #5
 8002e5e:	f200 81bb 	bhi.w	80031d8 <fdRevTick+0x570>
 8002e62:	4915      	ldr	r1, [pc, #84]	@ (8002eb8 <fdRevTick+0x250>)
 8002e64:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8002e68:	2301      	movs	r3, #1
 8002e6a:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8002e6e:	2202      	movs	r2, #2
 8002e70:	690b      	ldr	r3, [r1, #16]
 8002e72:	f8a4 245c 	strh.w	r2, [r4, #1116]	@ 0x45c
 8002e76:	698a      	ldr	r2, [r1, #24]
 8002e78:	2a00      	cmp	r2, #0
 8002e7a:	f47f af52 	bne.w	8002d22 <fdRevTick+0xba>
 8002e7e:	2b17      	cmp	r3, #23
 8002e80:	f63f af4f 	bhi.w	8002d22 <fdRevTick+0xba>
 8002e84:	68ca      	ldr	r2, [r1, #12]
 8002e86:	ebc3 1303 	rsb	r3, r3, r3, lsl #4
 8002e8a:	eb01 0383 	add.w	r3, r1, r3, lsl #2
 8002e8e:	1aba      	subs	r2, r7, r2
 8002e90:	625a      	str	r2, [r3, #36]	@ 0x24
 8002e92:	2202      	movs	r2, #2
 8002e94:	659a      	str	r2, [r3, #88]	@ 0x58
 8002e96:	f04f 33ff 	mov.w	r3, #4294967295	@ 0xffffffff
 8002e9a:	610b      	str	r3, [r1, #16]
 8002e9c:	e741      	b.n	8002d22 <fdRevTick+0xba>
 8002e9e:	2301      	movs	r3, #1
 8002ea0:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8002ea4:	e746      	b.n	8002d34 <fdRevTick+0xcc>
 8002ea6:	bf00      	nop
 8002ea8:	20000000 	.word	0x20000000
 8002eac:	20001184 	.word	0x20001184
 8002eb0:	e0001000 	.word	0xe0001000
 8002eb4:	00493e00 	.word	0x00493e00
 8002eb8:	20000b90 	.word	0x20000b90
 8002ebc:	03b82600 	.word	0x03b82600
 8002ec0:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8002ec4:	f8b4 1440 	ldrh.w	r1, [r4, #1088]	@ 0x440
 8002ec8:	f8d4 0448 	ldr.w	r0, [r4, #1096]	@ 0x448
 8002ecc:	1aba      	subs	r2, r7, r2
 8002ece:	2900      	cmp	r1, #0
 8002ed0:	f000 8099 	beq.w	8003006 <fdRevTick+0x39e>
 8002ed4:	4282      	cmp	r2, r0
 8002ed6:	d3e2      	bcc.n	8002e9e <fdRevTick+0x236>
 8002ed8:	49b3      	ldr	r1, [pc, #716]	@ (80031a8 <fdRevTick+0x540>)
 8002eda:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8002ede:	2200      	movs	r2, #0
 8002ee0:	4288      	cmp	r0, r1
 8002ee2:	f8a4 245d 	strh.w	r2, [r4, #1117]	@ 0x45d
 8002ee6:	f8c4 2454 	str.w	r2, [r4, #1108]	@ 0x454
 8002eea:	f8c4 246c 	str.w	r2, [r4, #1132]	@ 0x46c
 8002eee:	bf04      	itt	eq
 8002ef0:	4aae      	ldreq	r2, [pc, #696]	@ (80031ac <fdRevTick+0x544>)
 8002ef2:	f8c4 2448 	streq.w	r2, [r4, #1096]	@ 0x448
 8002ef6:	f894 2469 	ldrb.w	r2, [r4, #1129]	@ 0x469
 8002efa:	2a00      	cmp	r2, #0
 8002efc:	f000 8140 	beq.w	8003180 <fdRevTick+0x518>
 8002f00:	3a01      	subs	r2, #1
 8002f02:	b2d2      	uxtb	r2, r2
 8002f04:	f884 2469 	strb.w	r2, [r4, #1129]	@ 0x469
 8002f08:	2a00      	cmp	r2, #0
 8002f0a:	f000 8231 	beq.w	8003370 <fdRevTick+0x708>
 8002f0e:	2a06      	cmp	r2, #6
 8002f10:	bf28      	it	cs
 8002f12:	2206      	movcs	r2, #6
 8002f14:	49a6      	ldr	r1, [pc, #664]	@ (80031b0 <fdRevTick+0x548>)
 8002f16:	3a01      	subs	r2, #1
 8002f18:	f831 2012 	ldrh.w	r2, [r1, r2, lsl #1]
 8002f1c:	eb02 0282 	add.w	r2, r2, r2, lsl #2
 8002f20:	0152      	lsls	r2, r2, #5
 8002f22:	f8c4 2448 	str.w	r2, [r4, #1096]	@ 0x448
 8002f26:	2201      	movs	r2, #1
 8002f28:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 8002f2c:	4aa1      	ldr	r2, [pc, #644]	@ (80031b4 <fdRevTick+0x54c>)
 8002f2e:	6911      	ldr	r1, [r2, #16]
 8002f30:	6990      	ldr	r0, [r2, #24]
 8002f32:	b968      	cbnz	r0, 8002f50 <fdRevTick+0x2e8>
 8002f34:	2917      	cmp	r1, #23
 8002f36:	d80b      	bhi.n	8002f50 <fdRevTick+0x2e8>
 8002f38:	68d0      	ldr	r0, [r2, #12]
 8002f3a:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 8002f3e:	eb02 0181 	add.w	r1, r2, r1, lsl #2
 8002f42:	1a38      	subs	r0, r7, r0
 8002f44:	6248      	str	r0, [r1, #36]	@ 0x24
 8002f46:	2003      	movs	r0, #3
 8002f48:	6588      	str	r0, [r1, #88]	@ 0x58
 8002f4a:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8002f4e:	6111      	str	r1, [r2, #16]
 8002f50:	6b99      	ldr	r1, [r3, #56]	@ 0x38
 8002f52:	f894 247e 	ldrb.w	r2, [r4, #1150]	@ 0x47e
 8002f56:	3101      	adds	r1, #1
 8002f58:	6399      	str	r1, [r3, #56]	@ 0x38
 8002f5a:	3201      	adds	r2, #1
 8002f5c:	2300      	movs	r3, #0
 8002f5e:	2001      	movs	r0, #1
 8002f60:	f884 247e 	strb.w	r2, [r4, #1150]	@ 0x47e
 8002f64:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 8002f68:	f004 ff3e 	bl	8007de8 <adcZcdResetAcquisition>
 8002f6c:	f7ff f97a 	bl	8002264 <stuckRotorGuardInput>
 8002f70:	2800      	cmp	r0, #0
 8002f72:	f43f aed6 	beq.w	8002d22 <fdRevTick+0xba>
 8002f76:	f7fe ffdb 	bl	8001f30 <fdBenchService>
 8002f7a:	e6db      	b.n	8002d34 <fdRevTick+0xcc>
 8002f7c:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8002f80:	488d      	ldr	r0, [pc, #564]	@ (80031b8 <fdRevTick+0x550>)
 8002f82:	1aba      	subs	r2, r7, r2
 8002f84:	4282      	cmp	r2, r0
 8002f86:	d38a      	bcc.n	8002e9e <fdRevTick+0x236>
 8002f88:	2238      	movs	r2, #56	@ 0x38
 8002f8a:	f504 6087 	add.w	r0, r4, #1080	@ 0x438
 8002f8e:	f006 fdf7 	bl	8009b80 <memset>
 8002f92:	2201      	movs	r2, #1
 8002f94:	4987      	ldr	r1, [pc, #540]	@ (80031b4 <fdRevTick+0x54c>)
 8002f96:	f884 245c 	strb.w	r2, [r4, #1116]	@ 0x45c
 8002f9a:	f884 2440 	strb.w	r2, [r4, #1088]	@ 0x440
 8002f9e:	4a82      	ldr	r2, [pc, #520]	@ (80031a8 <fdRevTick+0x540>)
 8002fa0:	6908      	ldr	r0, [r1, #16]
 8002fa2:	f8c4 2448 	str.w	r2, [r4, #1096]	@ 0x448
 8002fa6:	f8d1 c018 	ldr.w	ip, [r1, #24]
 8002faa:	4b84      	ldr	r3, [pc, #528]	@ (80031bc <fdRevTick+0x554>)
 8002fac:	f8c4 643c 	str.w	r6, [r4, #1084]	@ 0x43c
 8002fb0:	22ff      	movs	r2, #255	@ 0xff
 8002fb2:	f884 2467 	strb.w	r2, [r4, #1127]	@ 0x467
 8002fb6:	2202      	movs	r2, #2
 8002fb8:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8002fbc:	f884 2442 	strb.w	r2, [r4, #1090]	@ 0x442
 8002fc0:	f1bc 0f00 	cmp.w	ip, #0
 8002fc4:	d10d      	bne.n	8002fe2 <fdRevTick+0x37a>
 8002fc6:	2817      	cmp	r0, #23
 8002fc8:	d80b      	bhi.n	8002fe2 <fdRevTick+0x37a>
 8002fca:	68ca      	ldr	r2, [r1, #12]
 8002fcc:	f04f 0c3c 	mov.w	ip, #60	@ 0x3c
 8002fd0:	fb0c 1000 	mla	r0, ip, r0, r1
 8002fd4:	1aba      	subs	r2, r7, r2
 8002fd6:	6242      	str	r2, [r0, #36]	@ 0x24
 8002fd8:	2208      	movs	r2, #8
 8002fda:	6582      	str	r2, [r0, #88]	@ 0x58
 8002fdc:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8002fe0:	610a      	str	r2, [r1, #16]
 8002fe2:	f8d4 2428 	ldr.w	r2, [r4, #1064]	@ 0x428
 8002fe6:	2106      	movs	r1, #6
 8002fe8:	1aba      	subs	r2, r7, r2
 8002fea:	f8c4 2414 	str.w	r2, [r4, #1044]	@ 0x414
 8002fee:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 8002ff2:	2200      	movs	r2, #0
 8002ff4:	f242 7110 	movw	r1, #10000	@ 0x2710
 8002ff8:	2004      	movs	r0, #4
 8002ffa:	6119      	str	r1, [r3, #16]
 8002ffc:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 8003000:	f004 fef2 	bl	8007de8 <adcZcdResetAcquisition>
 8003004:	e68d      	b.n	8002d22 <fdRevTick+0xba>
 8003006:	f894 146a 	ldrb.w	r1, [r4, #1130]	@ 0x46a
 800300a:	2900      	cmp	r1, #0
 800300c:	f43f af62 	beq.w	8002ed4 <fdRevTick+0x26c>
 8003010:	f894 1469 	ldrb.w	r1, [r4, #1129]	@ 0x469
 8003014:	2900      	cmp	r1, #0
 8003016:	f47f af5d 	bne.w	8002ed4 <fdRevTick+0x26c>
 800301a:	f1a7 716e 	sub.w	r1, r7, #62390272	@ 0x3b80000
 800301e:	f8df c198 	ldr.w	ip, [pc, #408]	@ 80031b8 <fdRevTick+0x550>
 8003022:	f5a1 5118 	sub.w	r1, r1, #9728	@ 0x2600
 8003026:	1b89      	subs	r1, r1, r6
 8003028:	4561      	cmp	r1, ip
 800302a:	f4bf af53 	bcs.w	8002ed4 <fdRevTick+0x26c>
 800302e:	4282      	cmp	r2, r0
 8003030:	f4ff af35 	bcc.w	8002e9e <fdRevTick+0x236>
 8003034:	2300      	movs	r3, #0
 8003036:	2201      	movs	r2, #1
 8003038:	f884 345e 	strb.w	r3, [r4, #1118]	@ 0x45e
 800303c:	f8c4 346c 	str.w	r3, [r4, #1132]	@ 0x46c
 8003040:	f8c4 3454 	str.w	r3, [r4, #1108]	@ 0x454
 8003044:	4b5b      	ldr	r3, [pc, #364]	@ (80031b4 <fdRevTick+0x54c>)
 8003046:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 800304a:	f884 2440 	strb.w	r2, [r4, #1088]	@ 0x440
 800304e:	691a      	ldr	r2, [r3, #16]
 8003050:	6999      	ldr	r1, [r3, #24]
 8003052:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8003056:	2005      	movs	r0, #5
 8003058:	f8a4 045c 	strh.w	r0, [r4, #1116]	@ 0x45c
 800305c:	b969      	cbnz	r1, 800307a <fdRevTick+0x412>
 800305e:	2a17      	cmp	r2, #23
 8003060:	d80b      	bhi.n	800307a <fdRevTick+0x412>
 8003062:	68d9      	ldr	r1, [r3, #12]
 8003064:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 8003068:	eb03 0282 	add.w	r2, r3, r2, lsl #2
 800306c:	1a79      	subs	r1, r7, r1
 800306e:	6251      	str	r1, [r2, #36]	@ 0x24
 8003070:	2107      	movs	r1, #7
 8003072:	6591      	str	r1, [r2, #88]	@ 0x58
 8003074:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003078:	611a      	str	r2, [r3, #16]
 800307a:	f8d4 3428 	ldr.w	r3, [r4, #1064]	@ 0x428
 800307e:	1afb      	subs	r3, r7, r3
 8003080:	f8c4 3410 	str.w	r3, [r4, #1040]	@ 0x410
 8003084:	f006 faea 	bl	800965c <allOff>
 8003088:	4a4d      	ldr	r2, [pc, #308]	@ (80031c0 <fdRevTick+0x558>)
 800308a:	2300      	movs	r3, #0
 800308c:	f8a4 30d0 	strh.w	r3, [r4, #208]	@ 0xd0
 8003090:	f8a4 30d4 	strh.w	r3, [r4, #212]	@ 0xd4
 8003094:	f8a4 30d2 	strh.w	r3, [r4, #210]	@ 0xd2
 8003098:	f8a4 30ce 	strh.w	r3, [r4, #206]	@ 0xce
 800309c:	2004      	movs	r0, #4
 800309e:	6353      	str	r3, [r2, #52]	@ 0x34
 80030a0:	6393      	str	r3, [r2, #56]	@ 0x38
 80030a2:	63d3      	str	r3, [r2, #60]	@ 0x3c
 80030a4:	6413      	str	r3, [r2, #64]	@ 0x40
 80030a6:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 80030aa:	f004 fe9d 	bl	8007de8 <adcZcdResetAcquisition>
 80030ae:	4b45      	ldr	r3, [pc, #276]	@ (80031c4 <fdRevTick+0x55c>)
 80030b0:	685b      	ldr	r3, [r3, #4]
 80030b2:	f8c4 3444 	str.w	r3, [r4, #1092]	@ 0x444
 80030b6:	e63d      	b.n	8002d34 <fdRevTick+0xcc>
 80030b8:	4943      	ldr	r1, [pc, #268]	@ (80031c8 <fdRevTick+0x560>)
 80030ba:	428a      	cmp	r2, r1
 80030bc:	f4ff ae3a 	bcc.w	8002d34 <fdRevTick+0xcc>
 80030c0:	2200      	movs	r2, #0
 80030c2:	f884 2442 	strb.w	r2, [r4, #1090]	@ 0x442
 80030c6:	2202      	movs	r2, #2
 80030c8:	f884 245c 	strb.w	r2, [r4, #1116]	@ 0x45c
 80030cc:	2001      	movs	r0, #1
 80030ce:	4a39      	ldr	r2, [pc, #228]	@ (80031b4 <fdRevTick+0x54c>)
 80030d0:	f8c4 00ec 	str.w	r0, [r4, #236]	@ 0xec
 80030d4:	6911      	ldr	r1, [r2, #16]
 80030d6:	f8d2 c018 	ldr.w	ip, [r2, #24]
 80030da:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 80030de:	f8c4 7438 	str.w	r7, [r4, #1080]	@ 0x438
 80030e2:	f1bc 0f00 	cmp.w	ip, #0
 80030e6:	d10f      	bne.n	8003108 <fdRevTick+0x4a0>
 80030e8:	2917      	cmp	r1, #23
 80030ea:	d80d      	bhi.n	8003108 <fdRevTick+0x4a0>
 80030ec:	f8d2 c00c 	ldr.w	ip, [r2, #12]
 80030f0:	f04f 0e3c 	mov.w	lr, #60	@ 0x3c
 80030f4:	fb0e 2101 	mla	r1, lr, r1, r2
 80030f8:	eba7 0c0c 	sub.w	ip, r7, ip
 80030fc:	f8c1 c024 	str.w	ip, [r1, #36]	@ 0x24
 8003100:	6588      	str	r0, [r1, #88]	@ 0x58
 8003102:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8003106:	6111      	str	r1, [r2, #16]
 8003108:	f006 faa8 	bl	800965c <allOff>
 800310c:	492d      	ldr	r1, [pc, #180]	@ (80031c4 <fdRevTick+0x55c>)
 800310e:	4b2b      	ldr	r3, [pc, #172]	@ (80031bc <fdRevTick+0x554>)
 8003110:	6848      	ldr	r0, [r1, #4]
 8003112:	684a      	ldr	r2, [r1, #4]
 8003114:	1a12      	subs	r2, r2, r0
 8003116:	f5b2 7f48 	cmp.w	r2, #800	@ 0x320
 800311a:	d3fa      	bcc.n	8003112 <fdRevTick+0x4aa>
 800311c:	f894 2440 	ldrb.w	r2, [r4, #1088]	@ 0x440
 8003120:	2101      	movs	r1, #1
 8003122:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 8003126:	2a00      	cmp	r2, #0
 8003128:	f43f adfb 	beq.w	8002d22 <fdRevTick+0xba>
 800312c:	f893 2718 	ldrb.w	r2, [r3, #1816]	@ 0x718
 8003130:	2a00      	cmp	r2, #0
 8003132:	bf0c      	ite	eq
 8003134:	2204      	moveq	r2, #4
 8003136:	2202      	movne	r2, #2
 8003138:	f883 2719 	strb.w	r2, [r3, #1817]	@ 0x719
 800313c:	e5f1      	b.n	8002d22 <fdRevTick+0xba>
 800313e:	f8d4 1450 	ldr.w	r1, [r4, #1104]	@ 0x450
 8003142:	eba0 0e01 	sub.w	lr, r0, r1
 8003146:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 800314a:	460f      	mov	r7, r1
 800314c:	2900      	cmp	r1, #0
 800314e:	f000 80e8 	beq.w	8003322 <fdRevTick+0x6ba>
 8003152:	4919      	ldr	r1, [pc, #100]	@ (80031b8 <fdRevTick+0x550>)
 8003154:	458e      	cmp	lr, r1
 8003156:	d306      	bcc.n	8003166 <fdRevTick+0x4fe>
 8003158:	f8d4 1460 	ldr.w	r1, [r4, #1120]	@ 0x460
 800315c:	1a40      	subs	r0, r0, r1
 800315e:	491b      	ldr	r1, [pc, #108]	@ (80031cc <fdRevTick+0x564>)
 8003160:	4288      	cmp	r0, r1
 8003162:	f080 812a 	bcs.w	80033ba <fdRevTick+0x752>
 8003166:	2100      	movs	r1, #0
 8003168:	e645      	b.n	8002df6 <fdRevTick+0x18e>
 800316a:	4919      	ldr	r1, [pc, #100]	@ (80031d0 <fdRevTick+0x568>)
 800316c:	458c      	cmp	ip, r1
 800316e:	f4ff ae2f 	bcc.w	8002dd0 <fdRevTick+0x168>
 8003172:	2001      	movs	r0, #1
 8003174:	9201      	str	r2, [sp, #4]
 8003176:	f7fe fe65 	bl	8001e44 <fdBenchStop>
 800317a:	9a01      	ldr	r2, [sp, #4]
 800317c:	4b0f      	ldr	r3, [pc, #60]	@ (80031bc <fdRevTick+0x554>)
 800317e:	e584      	b.n	8002c8a <fdRevTick+0x22>
 8003180:	f894 246a 	ldrb.w	r2, [r4, #1130]	@ 0x46a
 8003184:	2a00      	cmp	r2, #0
 8003186:	f47f aece 	bne.w	8002f26 <fdRevTick+0x2be>
 800318a:	f894 2468 	ldrb.w	r2, [r4, #1128]	@ 0x468
 800318e:	3201      	adds	r2, #1
 8003190:	b2d2      	uxtb	r2, r2
 8003192:	2a02      	cmp	r2, #2
 8003194:	f884 2468 	strb.w	r2, [r4, #1128]	@ 0x468
 8003198:	f47f aec5 	bne.w	8002f26 <fdRevTick+0x2be>
 800319c:	f44f 7283 	mov.w	r2, #262	@ 0x106
 80031a0:	f8a4 2469 	strh.w	r2, [r4, #1129]	@ 0x469
 80031a4:	4a0b      	ldr	r2, [pc, #44]	@ (80031d4 <fdRevTick+0x56c>)
 80031a6:	e6bc      	b.n	8002f22 <fdRevTick+0x2ba>
 80031a8:	00493e00 	.word	0x00493e00
 80031ac:	0036ee80 	.word	0x0036ee80
 80031b0:	08009c1c 	.word	0x08009c1c
 80031b4:	20000b90 	.word	0x20000b90
 80031b8:	0030d400 	.word	0x0030d400
 80031bc:	20000000 	.word	0x20000000
 80031c0:	40012c00 	.word	0x40012c00
 80031c4:	e0001000 	.word	0xe0001000
 80031c8:	00927c00 	.word	0x00927c00
 80031cc:	016e3600 	.word	0x016e3600
 80031d0:	07d1f400 	.word	0x07d1f400
 80031d4:	00271000 	.word	0x00271000
 80031d8:	2204      	movs	r2, #4
 80031da:	4999      	ldr	r1, [pc, #612]	@ (8003440 <fdRevTick+0x7d8>)
 80031dc:	f884 245c 	strb.w	r2, [r4, #1116]	@ 0x45c
 80031e0:	2201      	movs	r2, #1
 80031e2:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 80031e6:	690a      	ldr	r2, [r1, #16]
 80031e8:	6988      	ldr	r0, [r1, #24]
 80031ea:	f8c4 7460 	str.w	r7, [r4, #1120]	@ 0x460
 80031ee:	b968      	cbnz	r0, 800320c <fdRevTick+0x5a4>
 80031f0:	2a17      	cmp	r2, #23
 80031f2:	d80b      	bhi.n	800320c <fdRevTick+0x5a4>
 80031f4:	68c8      	ldr	r0, [r1, #12]
 80031f6:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 80031fa:	eb01 0282 	add.w	r2, r1, r2, lsl #2
 80031fe:	1a38      	subs	r0, r7, r0
 8003200:	6250      	str	r0, [r2, #36]	@ 0x24
 8003202:	2004      	movs	r0, #4
 8003204:	6590      	str	r0, [r2, #88]	@ 0x58
 8003206:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 800320a:	610a      	str	r2, [r1, #16]
 800320c:	698a      	ldr	r2, [r1, #24]
 800320e:	b9a2      	cbnz	r2, 800323a <fdRevTick+0x5d2>
 8003210:	690a      	ldr	r2, [r1, #16]
 8003212:	6988      	ldr	r0, [r1, #24]
 8003214:	b968      	cbnz	r0, 8003232 <fdRevTick+0x5ca>
 8003216:	2a17      	cmp	r2, #23
 8003218:	d80b      	bhi.n	8003232 <fdRevTick+0x5ca>
 800321a:	68c8      	ldr	r0, [r1, #12]
 800321c:	ebc2 1202 	rsb	r2, r2, r2, lsl #4
 8003220:	eb01 0282 	add.w	r2, r1, r2, lsl #2
 8003224:	1a38      	subs	r0, r7, r0
 8003226:	6250      	str	r0, [r2, #36]	@ 0x24
 8003228:	2005      	movs	r0, #5
 800322a:	6590      	str	r0, [r2, #88]	@ 0x58
 800322c:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003230:	610a      	str	r2, [r1, #16]
 8003232:	2200      	movs	r2, #0
 8003234:	61ca      	str	r2, [r1, #28]
 8003236:	2201      	movs	r2, #1
 8003238:	618a      	str	r2, [r1, #24]
 800323a:	f8d4 2428 	ldr.w	r2, [r4, #1064]	@ 0x428
 800323e:	4981      	ldr	r1, [pc, #516]	@ (8003444 <fdRevTick+0x7dc>)
 8003240:	f8d4 046c 	ldr.w	r0, [r4, #1132]	@ 0x46c
 8003244:	1aba      	subs	r2, r7, r2
 8003246:	f8c4 241c 	str.w	r2, [r4, #1052]	@ 0x41c
 800324a:	6a9a      	ldr	r2, [r3, #40]	@ 0x28
 800324c:	1aba      	subs	r2, r7, r2
 800324e:	631a      	str	r2, [r3, #48]	@ 0x30
 8003250:	2200      	movs	r2, #0
 8003252:	f884 2474 	strb.w	r2, [r4, #1140]	@ 0x474
 8003256:	f884 24fe 	strb.w	r2, [r4, #1278]	@ 0x4fe
 800325a:	fba1 2000 	umull	r2, r0, r1, r0
 800325e:	f8d4 2454 	ldr.w	r2, [r4, #1108]	@ 0x454
 8003262:	fba1 c202 	umull	ip, r2, r1, r2
 8003266:	0992      	lsrs	r2, r2, #6
 8003268:	611a      	str	r2, [r3, #16]
 800326a:	691a      	ldr	r2, [r3, #16]
 800326c:	f8a4 2510 	strh.w	r2, [r4, #1296]	@ 0x510
 8003270:	2206      	movs	r2, #6
 8003272:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 8003276:	691a      	ldr	r2, [r3, #16]
 8003278:	f8a4 2504 	strh.w	r2, [r4, #1284]	@ 0x504
 800327c:	691a      	ldr	r2, [r3, #16]
 800327e:	f8a4 2506 	strh.w	r2, [r4, #1286]	@ 0x506
 8003282:	691a      	ldr	r2, [r3, #16]
 8003284:	f8a4 2508 	strh.w	r2, [r4, #1288]	@ 0x508
 8003288:	691a      	ldr	r2, [r3, #16]
 800328a:	f8a4 250a 	strh.w	r2, [r4, #1290]	@ 0x50a
 800328e:	691a      	ldr	r2, [r3, #16]
 8003290:	f8a4 250c 	strh.w	r2, [r4, #1292]	@ 0x50c
 8003294:	691a      	ldr	r2, [r3, #16]
 8003296:	f8a4 250e 	strh.w	r2, [r4, #1294]	@ 0x50e
 800329a:	691a      	ldr	r2, [r3, #16]
 800329c:	f8c4 2500 	str.w	r2, [r4, #1280]	@ 0x500
 80032a0:	691b      	ldr	r3, [r3, #16]
 80032a2:	f8c4 3514 	str.w	r3, [r4, #1300]	@ 0x514
 80032a6:	0980      	lsrs	r0, r0, #6
 80032a8:	f004 fd86 	bl	8007db8 <adcZcdSeedHandoff>
 80032ac:	f8d4 3450 	ldr.w	r3, [r4, #1104]	@ 0x450
 80032b0:	4964      	ldr	r1, [pc, #400]	@ (8003444 <fdRevTick+0x7dc>)
 80032b2:	1afb      	subs	r3, r7, r3
 80032b4:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 80032b8:	fba1 1303 	umull	r1, r3, r1, r3
 80032bc:	099b      	lsrs	r3, r3, #6
 80032be:	6253      	str	r3, [r2, #36]	@ 0x24
 80032c0:	f7ff fa62 	bl	8002788 <commutate>
 80032c4:	e536      	b.n	8002d34 <fdRevTick+0xcc>
 80032c6:	f894 1442 	ldrb.w	r1, [r4, #1090]	@ 0x442
 80032ca:	2901      	cmp	r1, #1
 80032cc:	d047      	beq.n	800335e <fdRevTick+0x6f6>
 80032ce:	f8d4 1444 	ldr.w	r1, [r4, #1092]	@ 0x444
 80032d2:	1a40      	subs	r0, r0, r1
 80032d4:	495c      	ldr	r1, [pc, #368]	@ (8003448 <fdRevTick+0x7e0>)
 80032d6:	4288      	cmp	r0, r1
 80032d8:	d841      	bhi.n	800335e <fdRevTick+0x6f6>
 80032da:	495a      	ldr	r1, [pc, #360]	@ (8003444 <fdRevTick+0x7dc>)
 80032dc:	fba1 0100 	umull	r0, r1, r1, r0
 80032e0:	09c9      	lsrs	r1, r1, #7
 80032e2:	485a      	ldr	r0, [pc, #360]	@ (800344c <fdRevTick+0x7e4>)
 80032e4:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 80032e8:	fb0c f101 	mul.w	r1, ip, r1
 80032ec:	fba0 0101 	umull	r0, r1, r0, r1
 80032f0:	0b49      	lsrs	r1, r1, #13
 80032f2:	31d2      	adds	r1, #210	@ 0xd2
 80032f4:	b289      	uxth	r1, r1
 80032f6:	e57e      	b.n	8002df6 <fdRevTick+0x18e>
 80032f8:	f894 1441 	ldrb.w	r1, [r4, #1089]	@ 0x441
 80032fc:	2900      	cmp	r1, #0
 80032fe:	d039      	beq.n	8003374 <fdRevTick+0x70c>
 8003300:	4953      	ldr	r1, [pc, #332]	@ (8003450 <fdRevTick+0x7e8>)
 8003302:	458c      	cmp	ip, r1
 8003304:	f4bf af35 	bcs.w	8003172 <fdRevTick+0x50a>
 8003308:	4952      	ldr	r1, [pc, #328]	@ (8003454 <fdRevTick+0x7ec>)
 800330a:	458c      	cmp	ip, r1
 800330c:	bf34      	ite	cc
 800330e:	f44f 71b4 	movcc.w	r1, #360	@ 0x168
 8003312:	f44f 71c8 	movcs.w	r1, #400	@ 0x190
 8003316:	f894 046b 	ldrb.w	r0, [r4, #1131]	@ 0x46b
 800331a:	2800      	cmp	r0, #0
 800331c:	bf18      	it	ne
 800331e:	2100      	movne	r1, #0
 8003320:	e569      	b.n	8002df6 <fdRevTick+0x18e>
 8003322:	f8d4 1460 	ldr.w	r1, [r4, #1120]	@ 0x460
 8003326:	1a40      	subs	r0, r0, r1
 8003328:	494b      	ldr	r1, [pc, #300]	@ (8003458 <fdRevTick+0x7f0>)
 800332a:	4288      	cmp	r0, r1
 800332c:	d232      	bcs.n	8003394 <fdRevTick+0x72c>
 800332e:	494b      	ldr	r1, [pc, #300]	@ (800345c <fdRevTick+0x7f4>)
 8003330:	458e      	cmp	lr, r1
 8003332:	d271      	bcs.n	8003418 <fdRevTick+0x7b0>
 8003334:	4947      	ldr	r1, [pc, #284]	@ (8003454 <fdRevTick+0x7ec>)
 8003336:	458c      	cmp	ip, r1
 8003338:	d311      	bcc.n	800335e <fdRevTick+0x6f6>
 800333a:	f06f 0127 	mvn.w	r1, #39	@ 0x27
 800333e:	fb01 f10e 	mul.w	r1, r1, lr
 8003342:	f101 61f4 	add.w	r1, r1, #127926272	@ 0x7a00000
 8003346:	4845      	ldr	r0, [pc, #276]	@ (800345c <fdRevTick+0x7f4>)
 8003348:	f501 3190 	add.w	r1, r1, #73728	@ 0x12000
 800334c:	fbb1 f1f0 	udiv	r1, r1, r0
 8003350:	f501 71b4 	add.w	r1, r1, #360	@ 0x168
 8003354:	b289      	uxth	r1, r1
 8003356:	e54e      	b.n	8002df6 <fdRevTick+0x18e>
 8003358:	2f00      	cmp	r7, #0
 800335a:	f47f af04 	bne.w	8003166 <fdRevTick+0x4fe>
 800335e:	f44f 71b4 	mov.w	r1, #360	@ 0x168
 8003362:	e548      	b.n	8002df6 <fdRevTick+0x18e>
 8003364:	2900      	cmp	r1, #0
 8003366:	bf14      	ite	ne
 8003368:	2100      	movne	r1, #0
 800336a:	f44f 71b4 	moveq.w	r1, #360	@ 0x168
 800336e:	e53e      	b.n	8002dee <fdRevTick+0x186>
 8003370:	4a3b      	ldr	r2, [pc, #236]	@ (8003460 <fdRevTick+0x7f8>)
 8003372:	e5d6      	b.n	8002f22 <fdRevTick+0x2ba>
 8003374:	f894 146a 	ldrb.w	r1, [r4, #1130]	@ 0x46a
 8003378:	b181      	cbz	r1, 800339c <fdRevTick+0x734>
 800337a:	f894 1469 	ldrb.w	r1, [r4, #1129]	@ 0x469
 800337e:	b969      	cbnz	r1, 800339c <fdRevTick+0x734>
 8003380:	4933      	ldr	r1, [pc, #204]	@ (8003450 <fdRevTick+0x7e8>)
 8003382:	458c      	cmp	ip, r1
 8003384:	d30e      	bcc.n	80033a4 <fdRevTick+0x73c>
 8003386:	4937      	ldr	r1, [pc, #220]	@ (8003464 <fdRevTick+0x7fc>)
 8003388:	458c      	cmp	ip, r1
 800338a:	f4bf aef2 	bcs.w	8003172 <fdRevTick+0x50a>
 800338e:	f44f 71c8 	mov.w	r1, #400	@ 0x190
 8003392:	e7c0      	b.n	8003316 <fdRevTick+0x6ae>
 8003394:	2101      	movs	r1, #1
 8003396:	f884 146b 	strb.w	r1, [r4, #1131]	@ 0x46b
 800339a:	e6e4      	b.n	8003166 <fdRevTick+0x4fe>
 800339c:	492c      	ldr	r1, [pc, #176]	@ (8003450 <fdRevTick+0x7e8>)
 800339e:	458c      	cmp	ip, r1
 80033a0:	f4bf aee7 	bcs.w	8003172 <fdRevTick+0x50a>
 80033a4:	482b      	ldr	r0, [pc, #172]	@ (8003454 <fdRevTick+0x7ec>)
 80033a6:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 80033aa:	4584      	cmp	ip, r0
 80033ac:	d327      	bcc.n	80033fe <fdRevTick+0x796>
 80033ae:	2900      	cmp	r1, #0
 80033b0:	bf14      	ite	ne
 80033b2:	2100      	movne	r1, #0
 80033b4:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 80033b8:	e51d      	b.n	8002df6 <fdRevTick+0x18e>
 80033ba:	8819      	ldrh	r1, [r3, #0]
 80033bc:	f8df e0a8 	ldr.w	lr, [pc, #168]	@ 8003468 <fdRevTick+0x800>
 80033c0:	f1a0 70b7 	sub.w	r0, r0, #23986176	@ 0x16e0000
 80033c4:	f5a0 5058 	sub.w	r0, r0, #13824	@ 0x3600
 80033c8:	4570      	cmp	r0, lr
 80033ca:	d230      	bcs.n	800342e <fdRevTick+0x7c6>
 80033cc:	885e      	ldrh	r6, [r3, #2]
 80033ce:	428e      	cmp	r6, r1
 80033d0:	46b0      	mov	r8, r6
 80033d2:	d21a      	bcs.n	800340a <fdRevTick+0x7a2>
 80033d4:	4e1f      	ldr	r6, [pc, #124]	@ (8003454 <fdRevTick+0x7ec>)
 80033d6:	45b4      	cmp	ip, r6
 80033d8:	d3be      	bcc.n	8003358 <fdRevTick+0x6f0>
 80033da:	2f00      	cmp	r7, #0
 80033dc:	f47f aec3 	bne.w	8003166 <fdRevTick+0x4fe>
 80033e0:	eba1 0c08 	sub.w	ip, r1, r8
 80033e4:	ebae 0e00 	sub.w	lr, lr, r0
 80033e8:	21a0      	movs	r1, #160	@ 0xa0
 80033ea:	4820      	ldr	r0, [pc, #128]	@ (800346c <fdRevTick+0x804>)
 80033ec:	fbbe f1f1 	udiv	r1, lr, r1
 80033f0:	fb0c f101 	mul.w	r1, ip, r1
 80033f4:	fbb1 f1f0 	udiv	r1, r1, r0
 80033f8:	4441      	add	r1, r8
 80033fa:	b289      	uxth	r1, r1
 80033fc:	e4fb      	b.n	8002df6 <fdRevTick+0x18e>
 80033fe:	2900      	cmp	r1, #0
 8003400:	bf14      	ite	ne
 8003402:	2100      	movne	r1, #0
 8003404:	f44f 71b4 	moveq.w	r1, #360	@ 0x168
 8003408:	e4f5      	b.n	8002df6 <fdRevTick+0x18e>
 800340a:	4812      	ldr	r0, [pc, #72]	@ (8003454 <fdRevTick+0x7ec>)
 800340c:	4584      	cmp	ip, r0
 800340e:	d3a3      	bcc.n	8003358 <fdRevTick+0x6f0>
 8003410:	2f00      	cmp	r7, #0
 8003412:	f47f aea8 	bne.w	8003166 <fdRevTick+0x4fe>
 8003416:	e4ee      	b.n	8002df6 <fdRevTick+0x18e>
 8003418:	f8df e054 	ldr.w	lr, [pc, #84]	@ 8003470 <fdRevTick+0x808>
 800341c:	8819      	ldrh	r1, [r3, #0]
 800341e:	4570      	cmp	r0, lr
 8003420:	d2cc      	bcs.n	80033bc <fdRevTick+0x754>
 8003422:	480c      	ldr	r0, [pc, #48]	@ (8003454 <fdRevTick+0x7ec>)
 8003424:	4584      	cmp	ip, r0
 8003426:	bf38      	it	cc
 8003428:	f44f 71b4 	movcc.w	r1, #360	@ 0x168
 800342c:	e4e3      	b.n	8002df6 <fdRevTick+0x18e>
 800342e:	4909      	ldr	r1, [pc, #36]	@ (8003454 <fdRevTick+0x7ec>)
 8003430:	458c      	cmp	ip, r1
 8003432:	f4bf ae98 	bcs.w	8003166 <fdRevTick+0x4fe>
 8003436:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 800343a:	2900      	cmp	r1, #0
 800343c:	d08f      	beq.n	800335e <fdRevTick+0x6f6>
 800343e:	e692      	b.n	8003166 <fdRevTick+0x4fe>
 8003440:	20000b90 	.word	0x20000b90
 8003444:	cccccccd 	.word	0xcccccccd
 8003448:	00186a9f 	.word	0x00186a9f
 800344c:	d1b71759 	.word	0xd1b71759
 8003450:	03b82600 	.word	0x03b82600
 8003454:	00493e00 	.word	0x00493e00
 8003458:	09896800 	.word	0x09896800
 800345c:	0030d400 	.word	0x0030d400
 8003460:	0036ee80 	.word	0x0036ee80
 8003464:	03e8fa00 	.word	0x03e8fa00
 8003468:	02dc6c00 	.word	0x02dc6c00
 800346c:	000493e0 	.word	0x000493e0
 8003470:	016e3600 	.word	0x016e3600
