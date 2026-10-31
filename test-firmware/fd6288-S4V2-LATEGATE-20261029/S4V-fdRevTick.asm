
test-firmware/fd6288-S4V-STALE-20261028/S4V_STALE.elf:     file format elf32-littlearm


Disassembly of section .text:

08002d1c <fdRevTick>:
 8002d1c:	e92d 41f0 	stmdb	sp!, {r4, r5, r6, r7, r8, lr}
 8002d20:	b082      	sub	sp, #8
 8002d22:	f3ef 8510 	mrs	r5, PRIMASK
 8002d26:	b672      	cpsid	i
 8002d28:	f3ef 8210 	mrs	r2, PRIMASK
 8002d2c:	b672      	cpsid	i
 8002d2e:	4bb2      	ldr	r3, [pc, #712]	@ (8002ff8 <fdRevTick+0x2dc>)
 8002d30:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002d34:	b121      	cbz	r1, 8002d40 <fdRevTick+0x24>
 8002d36:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8002d3a:	2900      	cmp	r1, #0
 8002d3c:	f000 80b9 	beq.w	8002eb2 <fdRevTick+0x196>
 8002d40:	f382 8810 	msr	PRIMASK, r2
 8002d44:	f8d3 266c 	ldr.w	r2, [r3, #1644]	@ 0x66c
 8002d48:	2a00      	cmp	r2, #0
 8002d4a:	f040 80ad 	bne.w	8002ea8 <fdRevTick+0x18c>
 8002d4e:	f8d3 2674 	ldr.w	r2, [r3, #1652]	@ 0x674
 8002d52:	2a00      	cmp	r2, #0
 8002d54:	f000 80a8 	beq.w	8002ea8 <fdRevTick+0x18c>
 8002d58:	4ca8      	ldr	r4, [pc, #672]	@ (8002ffc <fdRevTick+0x2e0>)
 8002d5a:	f894 20cc 	ldrb.w	r2, [r4, #204]	@ 0xcc
 8002d5e:	2a00      	cmp	r2, #0
 8002d60:	f000 80a2 	beq.w	8002ea8 <fdRevTick+0x18c>
 8002d64:	f8b4 2470 	ldrh.w	r2, [r4, #1136]	@ 0x470
 8002d68:	2a00      	cmp	r2, #0
 8002d6a:	f000 809d 	beq.w	8002ea8 <fdRevTick+0x18c>
 8002d6e:	f894 2474 	ldrb.w	r2, [r4, #1140]	@ 0x474
 8002d72:	2a00      	cmp	r2, #0
 8002d74:	f000 8098 	beq.w	8002ea8 <fdRevTick+0x18c>
 8002d78:	4aa1      	ldr	r2, [pc, #644]	@ (8003000 <fdRevTick+0x2e4>)
 8002d7a:	f894 0442 	ldrb.w	r0, [r4, #1090]	@ 0x442
 8002d7e:	6852      	ldr	r2, [r2, #4]
 8002d80:	f894 145c 	ldrb.w	r1, [r4, #1116]	@ 0x45c
 8002d84:	f8d4 843c 	ldr.w	r8, [r4, #1084]	@ 0x43c
 8002d88:	2802      	cmp	r0, #2
 8002d8a:	4617      	mov	r7, r2
 8002d8c:	f240 80b6 	bls.w	8002efc <fdRevTick+0x1e0>
 8002d90:	f8d4 245c 	ldr.w	r2, [r4, #1116]	@ 0x45c
 8002d94:	f022 427f 	bic.w	r2, r2, #4278190080	@ 0xff000000
 8002d98:	f022 02ff 	bic.w	r2, r2, #255	@ 0xff
 8002d9c:	2a00      	cmp	r2, #0
 8002d9e:	f040 80ad 	bne.w	8002efc <fdRevTick+0x1e0>
 8002da2:	f8d4 2450 	ldr.w	r2, [r4, #1104]	@ 0x450
 8002da6:	f8df c25c 	ldr.w	ip, [pc, #604]	@ 8003004 <fdRevTick+0x2e8>
 8002daa:	1aba      	subs	r2, r7, r2
 8002dac:	4562      	cmp	r2, ip
 8002dae:	f0c0 80a5 	bcc.w	8002efc <fdRevTick+0x1e0>
 8002db2:	2902      	cmp	r1, #2
 8002db4:	f040 80a5 	bne.w	8002f02 <fdRevTick+0x1e6>
 8002db8:	f894 2440 	ldrb.w	r2, [r4, #1088]	@ 0x440
 8002dbc:	b96a      	cbnz	r2, 8002dda <fdRevTick+0xbe>
 8002dbe:	f894 246a 	ldrb.w	r2, [r4, #1130]	@ 0x46a
 8002dc2:	b152      	cbz	r2, 8002dda <fdRevTick+0xbe>
 8002dc4:	f894 c469 	ldrb.w	ip, [r4, #1129]	@ 0x469
 8002dc8:	eba7 0e08 	sub.w	lr, r7, r8
 8002dcc:	f1bc 0f00 	cmp.w	ip, #0
 8002dd0:	d103      	bne.n	8002dda <fdRevTick+0xbe>
 8002dd2:	4a8d      	ldr	r2, [pc, #564]	@ (8003008 <fdRevTick+0x2ec>)
 8002dd4:	4596      	cmp	lr, r2
 8002dd6:	f080 8191 	bcs.w	80030fc <fdRevTick+0x3e0>
 8002dda:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8002dde:	f8d4 c448 	ldr.w	ip, [r4, #1096]	@ 0x448
 8002de2:	1aba      	subs	r2, r7, r2
 8002de4:	4562      	cmp	r2, ip
 8002de6:	f0c0 8149 	bcc.w	800307c <fdRevTick+0x360>
 8002dea:	28ff      	cmp	r0, #255	@ 0xff
 8002dec:	bf1c      	itt	ne
 8002dee:	3001      	addne	r0, #1
 8002df0:	f884 0442 	strbne.w	r0, [r4, #1090]	@ 0x442
 8002df4:	4885      	ldr	r0, [pc, #532]	@ (800300c <fdRevTick+0x2f0>)
 8002df6:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8002dfa:	2200      	movs	r2, #0
 8002dfc:	4584      	cmp	ip, r0
 8002dfe:	f8a4 245d 	strh.w	r2, [r4, #1117]	@ 0x45d
 8002e02:	f8c4 2454 	str.w	r2, [r4, #1108]	@ 0x454
 8002e06:	f8c4 246c 	str.w	r2, [r4, #1132]	@ 0x46c
 8002e0a:	bf04      	itt	eq
 8002e0c:	4a80      	ldreq	r2, [pc, #512]	@ (8003010 <fdRevTick+0x2f4>)
 8002e0e:	f8c4 2448 	streq.w	r2, [r4, #1096]	@ 0x448
 8002e12:	f894 2469 	ldrb.w	r2, [r4, #1129]	@ 0x469
 8002e16:	2a00      	cmp	r2, #0
 8002e18:	f000 8270 	beq.w	80032fc <fdRevTick+0x5e0>
 8002e1c:	3a01      	subs	r2, #1
 8002e1e:	b2d2      	uxtb	r2, r2
 8002e20:	f884 2469 	strb.w	r2, [r4, #1129]	@ 0x469
 8002e24:	2a00      	cmp	r2, #0
 8002e26:	f000 8348 	beq.w	80034ba <fdRevTick+0x79e>
 8002e2a:	2a06      	cmp	r2, #6
 8002e2c:	bf28      	it	cs
 8002e2e:	2206      	movcs	r2, #6
 8002e30:	4878      	ldr	r0, [pc, #480]	@ (8003014 <fdRevTick+0x2f8>)
 8002e32:	3a01      	subs	r2, #1
 8002e34:	f830 2012 	ldrh.w	r2, [r0, r2, lsl #1]
 8002e38:	eb02 0282 	add.w	r2, r2, r2, lsl #2
 8002e3c:	0152      	lsls	r2, r2, #5
 8002e3e:	2901      	cmp	r1, #1
 8002e40:	f8c4 2448 	str.w	r2, [r4, #1096]	@ 0x448
 8002e44:	d002      	beq.n	8002e4c <fdRevTick+0x130>
 8002e46:	2201      	movs	r2, #1
 8002e48:	f8c4 20ec 	str.w	r2, [r4, #236]	@ 0xec
 8002e4c:	4a72      	ldr	r2, [pc, #456]	@ (8003018 <fdRevTick+0x2fc>)
 8002e4e:	6911      	ldr	r1, [r2, #16]
 8002e50:	6990      	ldr	r0, [r2, #24]
 8002e52:	b968      	cbnz	r0, 8002e70 <fdRevTick+0x154>
 8002e54:	2917      	cmp	r1, #23
 8002e56:	d80b      	bhi.n	8002e70 <fdRevTick+0x154>
 8002e58:	68d0      	ldr	r0, [r2, #12]
 8002e5a:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 8002e5e:	eb02 0181 	add.w	r1, r2, r1, lsl #2
 8002e62:	1a38      	subs	r0, r7, r0
 8002e64:	6248      	str	r0, [r1, #36]	@ 0x24
 8002e66:	2003      	movs	r0, #3
 8002e68:	6588      	str	r0, [r1, #88]	@ 0x58
 8002e6a:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8002e6e:	6111      	str	r1, [r2, #16]
 8002e70:	6b99      	ldr	r1, [r3, #56]	@ 0x38
 8002e72:	f894 247e 	ldrb.w	r2, [r4, #1150]	@ 0x47e
 8002e76:	3101      	adds	r1, #1
 8002e78:	6399      	str	r1, [r3, #56]	@ 0x38
 8002e7a:	3201      	adds	r2, #1
 8002e7c:	2300      	movs	r3, #0
 8002e7e:	2001      	movs	r0, #1
 8002e80:	f884 247e 	strb.w	r2, [r4, #1150]	@ 0x47e
 8002e84:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 8002e88:	f005 f8fc 	bl	8008084 <adcZcdResetAcquisition>
 8002e8c:	f7ff fa18 	bl	80022c0 <stuckRotorGuardInput>
 8002e90:	2800      	cmp	r0, #0
 8002e92:	f040 82ee 	bne.w	8003472 <fdRevTick+0x756>
 8002e96:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 8002e9a:	2300      	movs	r3, #0
 8002e9c:	6253      	str	r3, [r2, #36]	@ 0x24
 8002e9e:	f7ff fca1 	bl	80027e4 <commutate>
 8002ea2:	2300      	movs	r3, #0
 8002ea4:	f884 30de 	strb.w	r3, [r4, #222]	@ 0xde
 8002ea8:	f385 8810 	msr	PRIMASK, r5
 8002eac:	b002      	add	sp, #8
 8002eae:	e8bd 81f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, pc}
 8002eb2:	4953      	ldr	r1, [pc, #332]	@ (8003000 <fdRevTick+0x2e4>)
 8002eb4:	4c51      	ldr	r4, [pc, #324]	@ (8002ffc <fdRevTick+0x2e0>)
 8002eb6:	6848      	ldr	r0, [r1, #4]
 8002eb8:	f894 147c 	ldrb.w	r1, [r4, #1148]	@ 0x47c
 8002ebc:	b9b9      	cbnz	r1, 8002eee <fdRevTick+0x1d2>
 8002ebe:	f894 10c2 	ldrb.w	r1, [r4, #194]	@ 0xc2
 8002ec2:	2900      	cmp	r1, #0
 8002ec4:	f040 8083 	bne.w	8002fce <fdRevTick+0x2b2>
 8002ec8:	f8b4 1470 	ldrh.w	r1, [r4, #1136]	@ 0x470
 8002ecc:	bb21      	cbnz	r1, 8002f18 <fdRevTick+0x1fc>
 8002ece:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8002ed2:	2900      	cmp	r1, #0
 8002ed4:	f040 8082 	bne.w	8002fdc <fdRevTick+0x2c0>
 8002ed8:	2100      	movs	r1, #0
 8002eda:	f884 1474 	strb.w	r1, [r4, #1140]	@ 0x474
 8002ede:	f8a4 1430 	strh.w	r1, [r4, #1072]	@ 0x430
 8002ee2:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002ee6:	2101      	movs	r1, #1
 8002ee8:	f8c4 1432 	str.w	r1, [r4, #1074]	@ 0x432
 8002eec:	e728      	b.n	8002d40 <fdRevTick+0x24>
 8002eee:	2002      	movs	r0, #2
 8002ef0:	9201      	str	r2, [sp, #4]
 8002ef2:	f7fe ffa7 	bl	8001e44 <fdBenchStop>
 8002ef6:	9a01      	ldr	r2, [sp, #4]
 8002ef8:	4b3f      	ldr	r3, [pc, #252]	@ (8002ff8 <fdRevTick+0x2dc>)
 8002efa:	e721      	b.n	8002d40 <fdRevTick+0x24>
 8002efc:	2902      	cmp	r1, #2
 8002efe:	f000 80e7 	beq.w	80030d0 <fdRevTick+0x3b4>
 8002f02:	1e4a      	subs	r2, r1, #1
 8002f04:	2a04      	cmp	r2, #4
 8002f06:	f200 80b9 	bhi.w	800307c <fdRevTick+0x360>
 8002f0a:	e8df f012 	tbh	[pc, r2, lsl #1]
 8002f0e:	00bb      	.short	0x00bb
 8002f10:	008700ee 	.word	0x008700ee
 8002f14:	00b000b7 	.word	0x00b000b7
 8002f18:	f894 147d 	ldrb.w	r1, [r4, #1149]	@ 0x47d
 8002f1c:	2900      	cmp	r1, #0
 8002f1e:	d064      	beq.n	8002fea <fdRevTick+0x2ce>
 8002f20:	f894 10cc 	ldrb.w	r1, [r4, #204]	@ 0xcc
 8002f24:	f894 c475 	ldrb.w	ip, [r4, #1141]	@ 0x475
 8002f28:	ea51 010c 	orrs.w	r1, r1, ip
 8002f2c:	d05d      	beq.n	8002fea <fdRevTick+0x2ce>
 8002f2e:	f894 1478 	ldrb.w	r1, [r4, #1144]	@ 0x478
 8002f32:	2900      	cmp	r1, #0
 8002f34:	d159      	bne.n	8002fea <fdRevTick+0x2ce>
 8002f36:	f894 e45c 	ldrb.w	lr, [r4, #1116]	@ 0x45c
 8002f3a:	f8d4 1428 	ldr.w	r1, [r4, #1064]	@ 0x428
 8002f3e:	f1be 0f04 	cmp.w	lr, #4
 8002f42:	eba0 0c01 	sub.w	ip, r0, r1
 8002f46:	f000 81b8 	beq.w	80032ba <fdRevTick+0x59e>
 8002f4a:	f894 1440 	ldrb.w	r1, [r4, #1088]	@ 0x440
 8002f4e:	2900      	cmp	r1, #0
 8002f50:	f040 81c9 	bne.w	80032e6 <fdRevTick+0x5ca>
 8002f54:	f894 1442 	ldrb.w	r1, [r4, #1090]	@ 0x442
 8002f58:	2902      	cmp	r1, #2
 8002f5a:	d908      	bls.n	8002f6e <fdRevTick+0x252>
 8002f5c:	f8d4 145c 	ldr.w	r1, [r4, #1116]	@ 0x45c
 8002f60:	f021 417f 	bic.w	r1, r1, #4278190080	@ 0xff000000
 8002f64:	f021 01ff 	bic.w	r1, r1, #255	@ 0xff
 8002f68:	2900      	cmp	r1, #0
 8002f6a:	f000 82c0 	beq.w	80034ee <fdRevTick+0x7d2>
 8002f6e:	f1be 0f02 	cmp.w	lr, #2
 8002f72:	f000 82a8 	beq.w	80034c6 <fdRevTick+0x7aa>
 8002f76:	4924      	ldr	r1, [pc, #144]	@ (8003008 <fdRevTick+0x2ec>)
 8002f78:	458c      	cmp	ip, r1
 8002f7a:	f080 81b8 	bcs.w	80032ee <fdRevTick+0x5d2>
 8002f7e:	f1be 0f01 	cmp.w	lr, #1
 8002f82:	f000 825d 	beq.w	8003440 <fdRevTick+0x724>
 8002f86:	4821      	ldr	r0, [pc, #132]	@ (800300c <fdRevTick+0x2f0>)
 8002f88:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 8002f8c:	4584      	cmp	ip, r0
 8002f8e:	f0c0 828e 	bcc.w	80034ae <fdRevTick+0x792>
 8002f92:	2900      	cmp	r1, #0
 8002f94:	bf14      	ite	ne
 8002f96:	2100      	movne	r1, #0
 8002f98:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 8002f9c:	f1be 0f05 	cmp.w	lr, #5
 8002fa0:	bf08      	it	eq
 8002fa2:	2100      	moveq	r1, #0
 8002fa4:	f894 40c3 	ldrb.w	r4, [r4, #195]	@ 0xc3
 8002fa8:	88d8      	ldrh	r0, [r3, #6]
 8002faa:	b13c      	cbz	r4, 8002fbc <fdRevTick+0x2a0>
 8002fac:	f9b3 4008 	ldrsh.w	r4, [r3, #8]
 8002fb0:	ea24 74e4 	bic.w	r4, r4, r4, asr #31
 8002fb4:	b2a4      	uxth	r4, r4
 8002fb6:	42a0      	cmp	r0, r4
 8002fb8:	bf28      	it	cs
 8002fba:	4620      	movcs	r0, r4
 8002fbc:	4281      	cmp	r1, r0
 8002fbe:	bf28      	it	cs
 8002fc0:	4601      	movcs	r1, r0
 8002fc2:	b289      	uxth	r1, r1
 8002fc4:	f8c3 1678 	str.w	r1, [r3, #1656]	@ 0x678
 8002fc8:	2800      	cmp	r0, #0
 8002fca:	f47f aeb9 	bne.w	8002d40 <fdRevTick+0x24>
 8002fce:	2003      	movs	r0, #3
 8002fd0:	9201      	str	r2, [sp, #4]
 8002fd2:	f7fe ff37 	bl	8001e44 <fdBenchStop>
 8002fd6:	9a01      	ldr	r2, [sp, #4]
 8002fd8:	4b07      	ldr	r3, [pc, #28]	@ (8002ff8 <fdRevTick+0x2dc>)
 8002fda:	e6b1      	b.n	8002d40 <fdRevTick+0x24>
 8002fdc:	2007      	movs	r0, #7
 8002fde:	9201      	str	r2, [sp, #4]
 8002fe0:	f7fe fecc 	bl	8001d7c <fdBenchFinish.part.0>
 8002fe4:	4b04      	ldr	r3, [pc, #16]	@ (8002ff8 <fdRevTick+0x2dc>)
 8002fe6:	9a01      	ldr	r2, [sp, #4]
 8002fe8:	e776      	b.n	8002ed8 <fdRevTick+0x1bc>
 8002fea:	2004      	movs	r0, #4
 8002fec:	9201      	str	r2, [sp, #4]
 8002fee:	f7fe ff29 	bl	8001e44 <fdBenchStop>
 8002ff2:	9a01      	ldr	r2, [sp, #4]
 8002ff4:	4b00      	ldr	r3, [pc, #0]	@ (8002ff8 <fdRevTick+0x2dc>)
 8002ff6:	e6a3      	b.n	8002d40 <fdRevTick+0x24>
 8002ff8:	20000000 	.word	0x20000000
 8002ffc:	20001184 	.word	0x20001184
 8003000:	e0001000 	.word	0xe0001000
 8003004:	00a4cb80 	.word	0x00a4cb80
 8003008:	03b82600 	.word	0x03b82600
 800300c:	00493e00 	.word	0x00493e00
 8003010:	0036ee80 	.word	0x0036ee80
 8003014:	08009eb8 	.word	0x08009eb8
 8003018:	20000b90 	.word	0x20000b90
 800301c:	f8d4 2450 	ldr.w	r2, [r4, #1104]	@ 0x450
 8003020:	f8d4 1458 	ldr.w	r1, [r4, #1112]	@ 0x458
 8003024:	1aba      	subs	r2, r7, r2
 8003026:	428a      	cmp	r2, r1
 8003028:	d328      	bcc.n	800307c <fdRevTick+0x360>
 800302a:	f894 245e 	ldrb.w	r2, [r4, #1118]	@ 0x45e
 800302e:	2a05      	cmp	r2, #5
 8003030:	f200 818a 	bhi.w	8003348 <fdRevTick+0x62c>
 8003034:	4ab9      	ldr	r2, [pc, #740]	@ (800331c <fdRevTick+0x600>)
 8003036:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 800303a:	2301      	movs	r3, #1
 800303c:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8003040:	6913      	ldr	r3, [r2, #16]
 8003042:	6990      	ldr	r0, [r2, #24]
 8003044:	2102      	movs	r1, #2
 8003046:	f8a4 145c 	strh.w	r1, [r4, #1116]	@ 0x45c
 800304a:	2800      	cmp	r0, #0
 800304c:	f47f af23 	bne.w	8002e96 <fdRevTick+0x17a>
 8003050:	2b17      	cmp	r3, #23
 8003052:	f63f af20 	bhi.w	8002e96 <fdRevTick+0x17a>
 8003056:	68d0      	ldr	r0, [r2, #12]
 8003058:	ebc3 1303 	rsb	r3, r3, r3, lsl #4
 800305c:	eb02 0383 	add.w	r3, r2, r3, lsl #2
 8003060:	1a38      	subs	r0, r7, r0
 8003062:	6258      	str	r0, [r3, #36]	@ 0x24
 8003064:	6599      	str	r1, [r3, #88]	@ 0x58
 8003066:	f04f 33ff 	mov.w	r3, #4294967295	@ 0xffffffff
 800306a:	6113      	str	r3, [r2, #16]
 800306c:	e713      	b.n	8002e96 <fdRevTick+0x17a>
 800306e:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8003072:	1aba      	subs	r2, r7, r2
 8003074:	49aa      	ldr	r1, [pc, #680]	@ (8003320 <fdRevTick+0x604>)
 8003076:	428a      	cmp	r2, r1
 8003078:	f080 809d 	bcs.w	80031b6 <fdRevTick+0x49a>
 800307c:	2301      	movs	r3, #1
 800307e:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 8003082:	e711      	b.n	8002ea8 <fdRevTick+0x18c>
 8003084:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8003088:	1aba      	subs	r2, r7, r2
 800308a:	f894 1443 	ldrb.w	r1, [r4, #1091]	@ 0x443
 800308e:	2901      	cmp	r1, #1
 8003090:	f000 80d0 	beq.w	8003234 <fdRevTick+0x518>
 8003094:	4ba3      	ldr	r3, [pc, #652]	@ (8003324 <fdRevTick+0x608>)
 8003096:	429a      	cmp	r2, r3
 8003098:	f4ff af06 	bcc.w	8002ea8 <fdRevTick+0x18c>
 800309c:	4b9f      	ldr	r3, [pc, #636]	@ (800331c <fdRevTick+0x600>)
 800309e:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 80030a2:	3901      	subs	r1, #1
 80030a4:	f884 1443 	strb.w	r1, [r4, #1091]	@ 0x443
 80030a8:	6919      	ldr	r1, [r3, #16]
 80030aa:	699a      	ldr	r2, [r3, #24]
 80030ac:	2a00      	cmp	r2, #0
 80030ae:	f47f aef2 	bne.w	8002e96 <fdRevTick+0x17a>
 80030b2:	2917      	cmp	r1, #23
 80030b4:	f63f aeef 	bhi.w	8002e96 <fdRevTick+0x17a>
 80030b8:	68da      	ldr	r2, [r3, #12]
 80030ba:	203c      	movs	r0, #60	@ 0x3c
 80030bc:	fb00 3101 	mla	r1, r0, r1, r3
 80030c0:	1aba      	subs	r2, r7, r2
 80030c2:	624a      	str	r2, [r1, #36]	@ 0x24
 80030c4:	2201      	movs	r2, #1
 80030c6:	658a      	str	r2, [r1, #88]	@ 0x58
 80030c8:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 80030cc:	611a      	str	r2, [r3, #16]
 80030ce:	e6e2      	b.n	8002e96 <fdRevTick+0x17a>
 80030d0:	f894 2440 	ldrb.w	r2, [r4, #1088]	@ 0x440
 80030d4:	f894 c441 	ldrb.w	ip, [r4, #1089]	@ 0x441
 80030d8:	ea52 020c 	orrs.w	r2, r2, ip
 80030dc:	f47f ae7d 	bne.w	8002dda <fdRevTick+0xbe>
 80030e0:	f894 246a 	ldrb.w	r2, [r4, #1130]	@ 0x46a
 80030e4:	2a00      	cmp	r2, #0
 80030e6:	f47f ae6d 	bne.w	8002dc4 <fdRevTick+0xa8>
 80030ea:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 80030ee:	f8d4 c448 	ldr.w	ip, [r4, #1096]	@ 0x448
 80030f2:	1aba      	subs	r2, r7, r2
 80030f4:	4594      	cmp	ip, r2
 80030f6:	f67f ae78 	bls.w	8002dea <fdRevTick+0xce>
 80030fa:	e7bf      	b.n	800307c <fdRevTick+0x360>
 80030fc:	4a8a      	ldr	r2, [pc, #552]	@ (8003328 <fdRevTick+0x60c>)
 80030fe:	4596      	cmp	lr, r2
 8003100:	f4bf aeff 	bcs.w	8002f02 <fdRevTick+0x1e6>
 8003104:	f8d4 2444 	ldr.w	r2, [r4, #1092]	@ 0x444
 8003108:	f8d4 1448 	ldr.w	r1, [r4, #1096]	@ 0x448
 800310c:	1aba      	subs	r2, r7, r2
 800310e:	428a      	cmp	r2, r1
 8003110:	d210      	bcs.n	8003134 <fdRevTick+0x418>
 8003112:	2101      	movs	r1, #1
 8003114:	2904      	cmp	r1, #4
 8003116:	d8b1      	bhi.n	800307c <fdRevTick+0x360>
 8003118:	a001      	add	r0, pc, #4	@ (adr r0, 8003120 <fdRevTick+0x404>)
 800311a:	f850 f021 	ldr.w	pc, [r0, r1, lsl #2]
 800311e:	bf00      	nop
 8003120:	0800308b 	.word	0x0800308b
 8003124:	0800307d 	.word	0x0800307d
 8003128:	0800301d 	.word	0x0800301d
 800312c:	0800307d 	.word	0x0800307d
 8003130:	08003075 	.word	0x08003075
 8003134:	2301      	movs	r3, #1
 8003136:	f8c4 30ec 	str.w	r3, [r4, #236]	@ 0xec
 800313a:	f884 3440 	strb.w	r3, [r4, #1088]	@ 0x440
 800313e:	2305      	movs	r3, #5
 8003140:	f8a4 345c 	strh.w	r3, [r4, #1116]	@ 0x45c
 8003144:	4b75      	ldr	r3, [pc, #468]	@ (800331c <fdRevTick+0x600>)
 8003146:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 800314a:	6919      	ldr	r1, [r3, #16]
 800314c:	699a      	ldr	r2, [r3, #24]
 800314e:	f884 c45e 	strb.w	ip, [r4, #1118]	@ 0x45e
 8003152:	f8c4 c46c 	str.w	ip, [r4, #1132]	@ 0x46c
 8003156:	f8c4 c454 	str.w	ip, [r4, #1108]	@ 0x454
 800315a:	b96a      	cbnz	r2, 8003178 <fdRevTick+0x45c>
 800315c:	2917      	cmp	r1, #23
 800315e:	d80b      	bhi.n	8003178 <fdRevTick+0x45c>
 8003160:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 8003164:	eb03 0281 	add.w	r2, r3, r1, lsl #2
 8003168:	68d9      	ldr	r1, [r3, #12]
 800316a:	1a79      	subs	r1, r7, r1
 800316c:	6251      	str	r1, [r2, #36]	@ 0x24
 800316e:	2107      	movs	r1, #7
 8003170:	6591      	str	r1, [r2, #88]	@ 0x58
 8003172:	f04f 32ff 	mov.w	r2, #4294967295	@ 0xffffffff
 8003176:	611a      	str	r2, [r3, #16]
 8003178:	f8d4 3428 	ldr.w	r3, [r4, #1064]	@ 0x428
 800317c:	1afb      	subs	r3, r7, r3
 800317e:	f8c4 3410 	str.w	r3, [r4, #1040]	@ 0x410
 8003182:	f006 fbb9 	bl	80098f8 <allOff>
 8003186:	4a69      	ldr	r2, [pc, #420]	@ (800332c <fdRevTick+0x610>)
 8003188:	2300      	movs	r3, #0
 800318a:	f8a4 30d0 	strh.w	r3, [r4, #208]	@ 0xd0
 800318e:	f8a4 30d4 	strh.w	r3, [r4, #212]	@ 0xd4
 8003192:	f8a4 30d2 	strh.w	r3, [r4, #210]	@ 0xd2
 8003196:	f8a4 30ce 	strh.w	r3, [r4, #206]	@ 0xce
 800319a:	2004      	movs	r0, #4
 800319c:	6353      	str	r3, [r2, #52]	@ 0x34
 800319e:	6393      	str	r3, [r2, #56]	@ 0x38
 80031a0:	63d3      	str	r3, [r2, #60]	@ 0x3c
 80031a2:	6413      	str	r3, [r2, #64]	@ 0x40
 80031a4:	f8c4 30d8 	str.w	r3, [r4, #216]	@ 0xd8
 80031a8:	f004 ff6c 	bl	8008084 <adcZcdResetAcquisition>
 80031ac:	4b60      	ldr	r3, [pc, #384]	@ (8003330 <fdRevTick+0x614>)
 80031ae:	685b      	ldr	r3, [r3, #4]
 80031b0:	f8c4 3444 	str.w	r3, [r4, #1092]	@ 0x444
 80031b4:	e678      	b.n	8002ea8 <fdRevTick+0x18c>
 80031b6:	2238      	movs	r2, #56	@ 0x38
 80031b8:	2100      	movs	r1, #0
 80031ba:	485e      	ldr	r0, [pc, #376]	@ (8003334 <fdRevTick+0x618>)
 80031bc:	f006 fe2e 	bl	8009e1c <memset>
 80031c0:	2101      	movs	r1, #1
 80031c2:	4a56      	ldr	r2, [pc, #344]	@ (800331c <fdRevTick+0x600>)
 80031c4:	f884 145c 	strb.w	r1, [r4, #1116]	@ 0x45c
 80031c8:	f884 1440 	strb.w	r1, [r4, #1088]	@ 0x440
 80031cc:	4955      	ldr	r1, [pc, #340]	@ (8003324 <fdRevTick+0x608>)
 80031ce:	6910      	ldr	r0, [r2, #16]
 80031d0:	f8c4 1448 	str.w	r1, [r4, #1096]	@ 0x448
 80031d4:	f8d2 c018 	ldr.w	ip, [r2, #24]
 80031d8:	4b57      	ldr	r3, [pc, #348]	@ (8003338 <fdRevTick+0x61c>)
 80031da:	f8c4 843c 	str.w	r8, [r4, #1084]	@ 0x43c
 80031de:	21ff      	movs	r1, #255	@ 0xff
 80031e0:	f884 1467 	strb.w	r1, [r4, #1127]	@ 0x467
 80031e4:	2102      	movs	r1, #2
 80031e6:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 80031ea:	f884 1443 	strb.w	r1, [r4, #1091]	@ 0x443
 80031ee:	f1bc 0f00 	cmp.w	ip, #0
 80031f2:	d10d      	bne.n	8003210 <fdRevTick+0x4f4>
 80031f4:	2817      	cmp	r0, #23
 80031f6:	d80b      	bhi.n	8003210 <fdRevTick+0x4f4>
 80031f8:	68d1      	ldr	r1, [r2, #12]
 80031fa:	f04f 0c3c 	mov.w	ip, #60	@ 0x3c
 80031fe:	fb0c 2000 	mla	r0, ip, r0, r2
 8003202:	1a79      	subs	r1, r7, r1
 8003204:	6241      	str	r1, [r0, #36]	@ 0x24
 8003206:	2108      	movs	r1, #8
 8003208:	6581      	str	r1, [r0, #88]	@ 0x58
 800320a:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 800320e:	6111      	str	r1, [r2, #16]
 8003210:	f8d4 2428 	ldr.w	r2, [r4, #1064]	@ 0x428
 8003214:	2106      	movs	r1, #6
 8003216:	1aba      	subs	r2, r7, r2
 8003218:	f8c4 2414 	str.w	r2, [r4, #1044]	@ 0x414
 800321c:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 8003220:	2200      	movs	r2, #0
 8003222:	f242 7110 	movw	r1, #10000	@ 0x2710
 8003226:	2004      	movs	r0, #4
 8003228:	6119      	str	r1, [r3, #16]
 800322a:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 800322e:	f004 ff29 	bl	8008084 <adcZcdResetAcquisition>
 8003232:	e630      	b.n	8002e96 <fdRevTick+0x17a>
 8003234:	4841      	ldr	r0, [pc, #260]	@ (800333c <fdRevTick+0x620>)
 8003236:	4282      	cmp	r2, r0
 8003238:	f4ff ae36 	bcc.w	8002ea8 <fdRevTick+0x18c>
 800323c:	4a37      	ldr	r2, [pc, #220]	@ (800331c <fdRevTick+0x600>)
 800323e:	f8c4 10ec 	str.w	r1, [r4, #236]	@ 0xec
 8003242:	2000      	movs	r0, #0
 8003244:	f884 0443 	strb.w	r0, [r4, #1091]	@ 0x443
 8003248:	6910      	ldr	r0, [r2, #16]
 800324a:	f8d2 c018 	ldr.w	ip, [r2, #24]
 800324e:	f8c4 7444 	str.w	r7, [r4, #1092]	@ 0x444
 8003252:	f04f 0e02 	mov.w	lr, #2
 8003256:	f8c4 7438 	str.w	r7, [r4, #1080]	@ 0x438
 800325a:	f884 e45c 	strb.w	lr, [r4, #1116]	@ 0x45c
 800325e:	f1bc 0f00 	cmp.w	ip, #0
 8003262:	d10f      	bne.n	8003284 <fdRevTick+0x568>
 8003264:	2817      	cmp	r0, #23
 8003266:	d80d      	bhi.n	8003284 <fdRevTick+0x568>
 8003268:	f8d2 c00c 	ldr.w	ip, [r2, #12]
 800326c:	f04f 0e3c 	mov.w	lr, #60	@ 0x3c
 8003270:	fb0e 2000 	mla	r0, lr, r0, r2
 8003274:	eba7 0c0c 	sub.w	ip, r7, ip
 8003278:	f8c0 c024 	str.w	ip, [r0, #36]	@ 0x24
 800327c:	6581      	str	r1, [r0, #88]	@ 0x58
 800327e:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8003282:	6111      	str	r1, [r2, #16]
 8003284:	f006 fb38 	bl	80098f8 <allOff>
 8003288:	4929      	ldr	r1, [pc, #164]	@ (8003330 <fdRevTick+0x614>)
 800328a:	4b2b      	ldr	r3, [pc, #172]	@ (8003338 <fdRevTick+0x61c>)
 800328c:	6848      	ldr	r0, [r1, #4]
 800328e:	684a      	ldr	r2, [r1, #4]
 8003290:	1a12      	subs	r2, r2, r0
 8003292:	f5b2 7f48 	cmp.w	r2, #800	@ 0x320
 8003296:	d3fa      	bcc.n	800328e <fdRevTick+0x572>
 8003298:	f894 2440 	ldrb.w	r2, [r4, #1088]	@ 0x440
 800329c:	2101      	movs	r1, #1
 800329e:	f883 1719 	strb.w	r1, [r3, #1817]	@ 0x719
 80032a2:	2a00      	cmp	r2, #0
 80032a4:	f43f adf7 	beq.w	8002e96 <fdRevTick+0x17a>
 80032a8:	f893 2718 	ldrb.w	r2, [r3, #1816]	@ 0x718
 80032ac:	2a00      	cmp	r2, #0
 80032ae:	bf0c      	ite	eq
 80032b0:	2204      	moveq	r2, #4
 80032b2:	2202      	movne	r2, #2
 80032b4:	f883 2719 	strb.w	r2, [r3, #1817]	@ 0x719
 80032b8:	e5ed      	b.n	8002e96 <fdRevTick+0x17a>
 80032ba:	f8d4 1450 	ldr.w	r1, [r4, #1104]	@ 0x450
 80032be:	eba0 0e01 	sub.w	lr, r0, r1
 80032c2:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 80032c6:	460f      	mov	r7, r1
 80032c8:	2900      	cmp	r1, #0
 80032ca:	f000 80d5 	beq.w	8003478 <fdRevTick+0x75c>
 80032ce:	4914      	ldr	r1, [pc, #80]	@ (8003320 <fdRevTick+0x604>)
 80032d0:	458e      	cmp	lr, r1
 80032d2:	d306      	bcc.n	80032e2 <fdRevTick+0x5c6>
 80032d4:	f8d4 1460 	ldr.w	r1, [r4, #1120]	@ 0x460
 80032d8:	1a40      	subs	r0, r0, r1
 80032da:	4919      	ldr	r1, [pc, #100]	@ (8003340 <fdRevTick+0x624>)
 80032dc:	4288      	cmp	r0, r1
 80032de:	f080 8134 	bcs.w	800354a <fdRevTick+0x82e>
 80032e2:	2100      	movs	r1, #0
 80032e4:	e65e      	b.n	8002fa4 <fdRevTick+0x288>
 80032e6:	4917      	ldr	r1, [pc, #92]	@ (8003344 <fdRevTick+0x628>)
 80032e8:	458c      	cmp	ip, r1
 80032ea:	f4ff ae48 	bcc.w	8002f7e <fdRevTick+0x262>
 80032ee:	2001      	movs	r0, #1
 80032f0:	9201      	str	r2, [sp, #4]
 80032f2:	f7fe fda7 	bl	8001e44 <fdBenchStop>
 80032f6:	9a01      	ldr	r2, [sp, #4]
 80032f8:	4b0f      	ldr	r3, [pc, #60]	@ (8003338 <fdRevTick+0x61c>)
 80032fa:	e521      	b.n	8002d40 <fdRevTick+0x24>
 80032fc:	f894 246a 	ldrb.w	r2, [r4, #1130]	@ 0x46a
 8003300:	b942      	cbnz	r2, 8003314 <fdRevTick+0x5f8>
 8003302:	f894 2468 	ldrb.w	r2, [r4, #1128]	@ 0x468
 8003306:	3201      	adds	r2, #1
 8003308:	b2d2      	uxtb	r2, r2
 800330a:	2a02      	cmp	r2, #2
 800330c:	f884 2468 	strb.w	r2, [r4, #1128]	@ 0x468
 8003310:	f000 8110 	beq.w	8003534 <fdRevTick+0x818>
 8003314:	2901      	cmp	r1, #1
 8003316:	f47f ad96 	bne.w	8002e46 <fdRevTick+0x12a>
 800331a:	e597      	b.n	8002e4c <fdRevTick+0x130>
 800331c:	20000b90 	.word	0x20000b90
 8003320:	0030d400 	.word	0x0030d400
 8003324:	00493e00 	.word	0x00493e00
 8003328:	03e8fa00 	.word	0x03e8fa00
 800332c:	40012c00 	.word	0x40012c00
 8003330:	e0001000 	.word	0xe0001000
 8003334:	200015bc 	.word	0x200015bc
 8003338:	20000000 	.word	0x20000000
 800333c:	00927c00 	.word	0x00927c00
 8003340:	016e3600 	.word	0x016e3600
 8003344:	07d1f400 	.word	0x07d1f400
 8003348:	4ab0      	ldr	r2, [pc, #704]	@ (800360c <fdRevTick+0x8f0>)
 800334a:	f8c4 7460 	str.w	r7, [r4, #1120]	@ 0x460
 800334e:	2101      	movs	r1, #1
 8003350:	f8c4 10ec 	str.w	r1, [r4, #236]	@ 0xec
 8003354:	6911      	ldr	r1, [r2, #16]
 8003356:	f8d2 c018 	ldr.w	ip, [r2, #24]
 800335a:	2004      	movs	r0, #4
 800335c:	f884 045c 	strb.w	r0, [r4, #1116]	@ 0x45c
 8003360:	f1bc 0f00 	cmp.w	ip, #0
 8003364:	d10f      	bne.n	8003386 <fdRevTick+0x66a>
 8003366:	2917      	cmp	r1, #23
 8003368:	d80d      	bhi.n	8003386 <fdRevTick+0x66a>
 800336a:	f8d2 c00c 	ldr.w	ip, [r2, #12]
 800336e:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 8003372:	eb02 0181 	add.w	r1, r2, r1, lsl #2
 8003376:	eba7 0c0c 	sub.w	ip, r7, ip
 800337a:	f8c1 c024 	str.w	ip, [r1, #36]	@ 0x24
 800337e:	6588      	str	r0, [r1, #88]	@ 0x58
 8003380:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 8003384:	6111      	str	r1, [r2, #16]
 8003386:	6991      	ldr	r1, [r2, #24]
 8003388:	b9a1      	cbnz	r1, 80033b4 <fdRevTick+0x698>
 800338a:	6911      	ldr	r1, [r2, #16]
 800338c:	6990      	ldr	r0, [r2, #24]
 800338e:	b968      	cbnz	r0, 80033ac <fdRevTick+0x690>
 8003390:	2917      	cmp	r1, #23
 8003392:	d80b      	bhi.n	80033ac <fdRevTick+0x690>
 8003394:	68d0      	ldr	r0, [r2, #12]
 8003396:	ebc1 1101 	rsb	r1, r1, r1, lsl #4
 800339a:	eb02 0181 	add.w	r1, r2, r1, lsl #2
 800339e:	1a38      	subs	r0, r7, r0
 80033a0:	6248      	str	r0, [r1, #36]	@ 0x24
 80033a2:	2005      	movs	r0, #5
 80033a4:	6588      	str	r0, [r1, #88]	@ 0x58
 80033a6:	f04f 31ff 	mov.w	r1, #4294967295	@ 0xffffffff
 80033aa:	6111      	str	r1, [r2, #16]
 80033ac:	2100      	movs	r1, #0
 80033ae:	61d1      	str	r1, [r2, #28]
 80033b0:	2101      	movs	r1, #1
 80033b2:	6191      	str	r1, [r2, #24]
 80033b4:	f8d4 2428 	ldr.w	r2, [r4, #1064]	@ 0x428
 80033b8:	4995      	ldr	r1, [pc, #596]	@ (8003610 <fdRevTick+0x8f4>)
 80033ba:	f8d4 046c 	ldr.w	r0, [r4, #1132]	@ 0x46c
 80033be:	1aba      	subs	r2, r7, r2
 80033c0:	f8c4 241c 	str.w	r2, [r4, #1052]	@ 0x41c
 80033c4:	6a9a      	ldr	r2, [r3, #40]	@ 0x28
 80033c6:	1aba      	subs	r2, r7, r2
 80033c8:	631a      	str	r2, [r3, #48]	@ 0x30
 80033ca:	2200      	movs	r2, #0
 80033cc:	f884 2474 	strb.w	r2, [r4, #1140]	@ 0x474
 80033d0:	f884 24fe 	strb.w	r2, [r4, #1278]	@ 0x4fe
 80033d4:	fba1 2000 	umull	r2, r0, r1, r0
 80033d8:	f8d4 2454 	ldr.w	r2, [r4, #1108]	@ 0x454
 80033dc:	fba1 c202 	umull	ip, r2, r1, r2
 80033e0:	0992      	lsrs	r2, r2, #6
 80033e2:	611a      	str	r2, [r3, #16]
 80033e4:	691a      	ldr	r2, [r3, #16]
 80033e6:	f8a4 2510 	strh.w	r2, [r4, #1296]	@ 0x510
 80033ea:	2206      	movs	r2, #6
 80033ec:	f8c4 20d8 	str.w	r2, [r4, #216]	@ 0xd8
 80033f0:	691a      	ldr	r2, [r3, #16]
 80033f2:	f8a4 2504 	strh.w	r2, [r4, #1284]	@ 0x504
 80033f6:	691a      	ldr	r2, [r3, #16]
 80033f8:	f8a4 2506 	strh.w	r2, [r4, #1286]	@ 0x506
 80033fc:	691a      	ldr	r2, [r3, #16]
 80033fe:	f8a4 2508 	strh.w	r2, [r4, #1288]	@ 0x508
 8003402:	691a      	ldr	r2, [r3, #16]
 8003404:	f8a4 250a 	strh.w	r2, [r4, #1290]	@ 0x50a
 8003408:	691a      	ldr	r2, [r3, #16]
 800340a:	f8a4 250c 	strh.w	r2, [r4, #1292]	@ 0x50c
 800340e:	691a      	ldr	r2, [r3, #16]
 8003410:	f8a4 250e 	strh.w	r2, [r4, #1294]	@ 0x50e
 8003414:	691a      	ldr	r2, [r3, #16]
 8003416:	f8c4 2500 	str.w	r2, [r4, #1280]	@ 0x500
 800341a:	691b      	ldr	r3, [r3, #16]
 800341c:	f8c4 3514 	str.w	r3, [r4, #1300]	@ 0x514
 8003420:	0980      	lsrs	r0, r0, #6
 8003422:	f004 fe17 	bl	8008054 <adcZcdSeedHandoff>
 8003426:	f8d4 3450 	ldr.w	r3, [r4, #1104]	@ 0x450
 800342a:	4979      	ldr	r1, [pc, #484]	@ (8003610 <fdRevTick+0x8f4>)
 800342c:	1afb      	subs	r3, r7, r3
 800342e:	f04f 4280 	mov.w	r2, #1073741824	@ 0x40000000
 8003432:	fba1 1303 	umull	r1, r3, r1, r3
 8003436:	099b      	lsrs	r3, r3, #6
 8003438:	6253      	str	r3, [r2, #36]	@ 0x24
 800343a:	f7ff f9d3 	bl	80027e4 <commutate>
 800343e:	e533      	b.n	8002ea8 <fdRevTick+0x18c>
 8003440:	f894 1443 	ldrb.w	r1, [r4, #1091]	@ 0x443
 8003444:	2901      	cmp	r1, #1
 8003446:	d02f      	beq.n	80034a8 <fdRevTick+0x78c>
 8003448:	f8d4 1444 	ldr.w	r1, [r4, #1092]	@ 0x444
 800344c:	1a40      	subs	r0, r0, r1
 800344e:	4971      	ldr	r1, [pc, #452]	@ (8003614 <fdRevTick+0x8f8>)
 8003450:	4288      	cmp	r0, r1
 8003452:	d829      	bhi.n	80034a8 <fdRevTick+0x78c>
 8003454:	496e      	ldr	r1, [pc, #440]	@ (8003610 <fdRevTick+0x8f4>)
 8003456:	fba1 0100 	umull	r0, r1, r1, r0
 800345a:	09c9      	lsrs	r1, r1, #7
 800345c:	486e      	ldr	r0, [pc, #440]	@ (8003618 <fdRevTick+0x8fc>)
 800345e:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 8003462:	fb0c f101 	mul.w	r1, ip, r1
 8003466:	fba0 0101 	umull	r0, r1, r0, r1
 800346a:	0b49      	lsrs	r1, r1, #13
 800346c:	31d2      	adds	r1, #210	@ 0xd2
 800346e:	b289      	uxth	r1, r1
 8003470:	e598      	b.n	8002fa4 <fdRevTick+0x288>
 8003472:	f7fe fd5d 	bl	8001f30 <fdBenchService>
 8003476:	e517      	b.n	8002ea8 <fdRevTick+0x18c>
 8003478:	f8d4 1460 	ldr.w	r1, [r4, #1120]	@ 0x460
 800347c:	1a40      	subs	r0, r0, r1
 800347e:	4967      	ldr	r1, [pc, #412]	@ (800361c <fdRevTick+0x900>)
 8003480:	4288      	cmp	r0, r1
 8003482:	d21c      	bcs.n	80034be <fdRevTick+0x7a2>
 8003484:	4966      	ldr	r1, [pc, #408]	@ (8003620 <fdRevTick+0x904>)
 8003486:	458e      	cmp	lr, r1
 8003488:	f0c0 80a3 	bcc.w	80035d2 <fdRevTick+0x8b6>
 800348c:	f8df e194 	ldr.w	lr, [pc, #404]	@ 8003624 <fdRevTick+0x908>
 8003490:	8819      	ldrh	r1, [r3, #0]
 8003492:	4570      	cmp	r0, lr
 8003494:	d25a      	bcs.n	800354c <fdRevTick+0x830>
 8003496:	4864      	ldr	r0, [pc, #400]	@ (8003628 <fdRevTick+0x90c>)
 8003498:	4584      	cmp	ip, r0
 800349a:	bf38      	it	cc
 800349c:	f44f 71b4 	movcc.w	r1, #360	@ 0x168
 80034a0:	e580      	b.n	8002fa4 <fdRevTick+0x288>
 80034a2:	2f00      	cmp	r7, #0
 80034a4:	f47f af1d 	bne.w	80032e2 <fdRevTick+0x5c6>
 80034a8:	f44f 71b4 	mov.w	r1, #360	@ 0x168
 80034ac:	e57a      	b.n	8002fa4 <fdRevTick+0x288>
 80034ae:	2900      	cmp	r1, #0
 80034b0:	bf14      	ite	ne
 80034b2:	2100      	movne	r1, #0
 80034b4:	f44f 71b4 	moveq.w	r1, #360	@ 0x168
 80034b8:	e570      	b.n	8002f9c <fdRevTick+0x280>
 80034ba:	4a5c      	ldr	r2, [pc, #368]	@ (800362c <fdRevTick+0x910>)
 80034bc:	e4bf      	b.n	8002e3e <fdRevTick+0x122>
 80034be:	2101      	movs	r1, #1
 80034c0:	f884 146b 	strb.w	r1, [r4, #1131]	@ 0x46b
 80034c4:	e70d      	b.n	80032e2 <fdRevTick+0x5c6>
 80034c6:	f894 1441 	ldrb.w	r1, [r4, #1089]	@ 0x441
 80034ca:	b1d9      	cbz	r1, 8003504 <fdRevTick+0x7e8>
 80034cc:	4958      	ldr	r1, [pc, #352]	@ (8003630 <fdRevTick+0x914>)
 80034ce:	458c      	cmp	ip, r1
 80034d0:	f4bf af0d 	bcs.w	80032ee <fdRevTick+0x5d2>
 80034d4:	4954      	ldr	r1, [pc, #336]	@ (8003628 <fdRevTick+0x90c>)
 80034d6:	f894 046b 	ldrb.w	r0, [r4, #1131]	@ 0x46b
 80034da:	458c      	cmp	ip, r1
 80034dc:	bf34      	ite	cc
 80034de:	f44f 71b4 	movcc.w	r1, #360	@ 0x168
 80034e2:	f44f 71c8 	movcs.w	r1, #400	@ 0x190
 80034e6:	2800      	cmp	r0, #0
 80034e8:	bf18      	it	ne
 80034ea:	2100      	movne	r1, #0
 80034ec:	e55a      	b.n	8002fa4 <fdRevTick+0x288>
 80034ee:	f8d4 1450 	ldr.w	r1, [r4, #1104]	@ 0x450
 80034f2:	4e50      	ldr	r6, [pc, #320]	@ (8003634 <fdRevTick+0x918>)
 80034f4:	1a41      	subs	r1, r0, r1
 80034f6:	42b1      	cmp	r1, r6
 80034f8:	f4ff ad39 	bcc.w	8002f6e <fdRevTick+0x252>
 80034fc:	f1be 0f02 	cmp.w	lr, #2
 8003500:	f47f ad39 	bne.w	8002f76 <fdRevTick+0x25a>
 8003504:	f894 146a 	ldrb.w	r1, [r4, #1130]	@ 0x46a
 8003508:	2900      	cmp	r1, #0
 800350a:	d040      	beq.n	800358e <fdRevTick+0x872>
 800350c:	f894 1469 	ldrb.w	r1, [r4, #1129]	@ 0x469
 8003510:	2900      	cmp	r1, #0
 8003512:	d153      	bne.n	80035bc <fdRevTick+0x8a0>
 8003514:	4946      	ldr	r1, [pc, #280]	@ (8003630 <fdRevTick+0x914>)
 8003516:	458c      	cmp	ip, r1
 8003518:	f4ff ad35 	bcc.w	8002f86 <fdRevTick+0x26a>
 800351c:	4946      	ldr	r1, [pc, #280]	@ (8003638 <fdRevTick+0x91c>)
 800351e:	458c      	cmp	ip, r1
 8003520:	f4bf aee5 	bcs.w	80032ee <fdRevTick+0x5d2>
 8003524:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 8003528:	2900      	cmp	r1, #0
 800352a:	bf14      	ite	ne
 800352c:	2100      	movne	r1, #0
 800352e:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 8003532:	e537      	b.n	8002fa4 <fdRevTick+0x288>
 8003534:	4a41      	ldr	r2, [pc, #260]	@ (800363c <fdRevTick+0x920>)
 8003536:	f8c4 2448 	str.w	r2, [r4, #1096]	@ 0x448
 800353a:	2901      	cmp	r1, #1
 800353c:	f44f 7283 	mov.w	r2, #262	@ 0x106
 8003540:	f8a4 2469 	strh.w	r2, [r4, #1129]	@ 0x469
 8003544:	f47f ac7f 	bne.w	8002e46 <fdRevTick+0x12a>
 8003548:	e480      	b.n	8002e4c <fdRevTick+0x130>
 800354a:	8819      	ldrh	r1, [r3, #0]
 800354c:	f8df e0f0 	ldr.w	lr, [pc, #240]	@ 8003640 <fdRevTick+0x924>
 8003550:	f1a0 70b7 	sub.w	r0, r0, #23986176	@ 0x16e0000
 8003554:	f5a0 5058 	sub.w	r0, r0, #13824	@ 0x3600
 8003558:	4570      	cmp	r0, lr
 800355a:	d24d      	bcs.n	80035f8 <fdRevTick+0x8dc>
 800355c:	885e      	ldrh	r6, [r3, #2]
 800355e:	428e      	cmp	r6, r1
 8003560:	46b0      	mov	r8, r6
 8003562:	d223      	bcs.n	80035ac <fdRevTick+0x890>
 8003564:	4e30      	ldr	r6, [pc, #192]	@ (8003628 <fdRevTick+0x90c>)
 8003566:	45b4      	cmp	ip, r6
 8003568:	d39b      	bcc.n	80034a2 <fdRevTick+0x786>
 800356a:	2f00      	cmp	r7, #0
 800356c:	f47f aeb9 	bne.w	80032e2 <fdRevTick+0x5c6>
 8003570:	eba1 0c08 	sub.w	ip, r1, r8
 8003574:	ebae 0e00 	sub.w	lr, lr, r0
 8003578:	21a0      	movs	r1, #160	@ 0xa0
 800357a:	4832      	ldr	r0, [pc, #200]	@ (8003644 <fdRevTick+0x928>)
 800357c:	fbbe f1f1 	udiv	r1, lr, r1
 8003580:	fb0c f101 	mul.w	r1, ip, r1
 8003584:	fbb1 f1f0 	udiv	r1, r1, r0
 8003588:	4441      	add	r1, r8
 800358a:	b289      	uxth	r1, r1
 800358c:	e50a      	b.n	8002fa4 <fdRevTick+0x288>
 800358e:	4928      	ldr	r1, [pc, #160]	@ (8003630 <fdRevTick+0x914>)
 8003590:	458c      	cmp	ip, r1
 8003592:	f4bf aeac 	bcs.w	80032ee <fdRevTick+0x5d2>
 8003596:	4824      	ldr	r0, [pc, #144]	@ (8003628 <fdRevTick+0x90c>)
 8003598:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 800359c:	4584      	cmp	ip, r0
 800359e:	d312      	bcc.n	80035c6 <fdRevTick+0x8aa>
 80035a0:	2900      	cmp	r1, #0
 80035a2:	bf14      	ite	ne
 80035a4:	2100      	movne	r1, #0
 80035a6:	f44f 71c8 	moveq.w	r1, #400	@ 0x190
 80035aa:	e4fb      	b.n	8002fa4 <fdRevTick+0x288>
 80035ac:	481e      	ldr	r0, [pc, #120]	@ (8003628 <fdRevTick+0x90c>)
 80035ae:	4584      	cmp	ip, r0
 80035b0:	f4ff af77 	bcc.w	80034a2 <fdRevTick+0x786>
 80035b4:	2f00      	cmp	r7, #0
 80035b6:	f43f acf5 	beq.w	8002fa4 <fdRevTick+0x288>
 80035ba:	e692      	b.n	80032e2 <fdRevTick+0x5c6>
 80035bc:	491c      	ldr	r1, [pc, #112]	@ (8003630 <fdRevTick+0x914>)
 80035be:	458c      	cmp	ip, r1
 80035c0:	f4ff ace1 	bcc.w	8002f86 <fdRevTick+0x26a>
 80035c4:	e693      	b.n	80032ee <fdRevTick+0x5d2>
 80035c6:	2900      	cmp	r1, #0
 80035c8:	bf14      	ite	ne
 80035ca:	2100      	movne	r1, #0
 80035cc:	f44f 71b4 	moveq.w	r1, #360	@ 0x168
 80035d0:	e4e8      	b.n	8002fa4 <fdRevTick+0x288>
 80035d2:	4915      	ldr	r1, [pc, #84]	@ (8003628 <fdRevTick+0x90c>)
 80035d4:	458c      	cmp	ip, r1
 80035d6:	f4ff af67 	bcc.w	80034a8 <fdRevTick+0x78c>
 80035da:	f06f 0127 	mvn.w	r1, #39	@ 0x27
 80035de:	fb01 f10e 	mul.w	r1, r1, lr
 80035e2:	f101 61f4 	add.w	r1, r1, #127926272	@ 0x7a00000
 80035e6:	480e      	ldr	r0, [pc, #56]	@ (8003620 <fdRevTick+0x904>)
 80035e8:	f501 3190 	add.w	r1, r1, #73728	@ 0x12000
 80035ec:	fbb1 f1f0 	udiv	r1, r1, r0
 80035f0:	f501 71b4 	add.w	r1, r1, #360	@ 0x168
 80035f4:	b289      	uxth	r1, r1
 80035f6:	e4d5      	b.n	8002fa4 <fdRevTick+0x288>
 80035f8:	490b      	ldr	r1, [pc, #44]	@ (8003628 <fdRevTick+0x90c>)
 80035fa:	458c      	cmp	ip, r1
 80035fc:	f4bf ae71 	bcs.w	80032e2 <fdRevTick+0x5c6>
 8003600:	f894 146b 	ldrb.w	r1, [r4, #1131]	@ 0x46b
 8003604:	2900      	cmp	r1, #0
 8003606:	f43f af4f 	beq.w	80034a8 <fdRevTick+0x78c>
 800360a:	e66a      	b.n	80032e2 <fdRevTick+0x5c6>
 800360c:	20000b90 	.word	0x20000b90
 8003610:	cccccccd 	.word	0xcccccccd
 8003614:	00186a9f 	.word	0x00186a9f
 8003618:	d1b71759 	.word	0xd1b71759
 800361c:	09896800 	.word	0x09896800
 8003620:	0030d400 	.word	0x0030d400
 8003624:	016e3600 	.word	0x016e3600
 8003628:	00493e00 	.word	0x00493e00
 800362c:	0036ee80 	.word	0x0036ee80
 8003630:	03b82600 	.word	0x03b82600
 8003634:	00a4cb80 	.word	0x00a4cb80
 8003638:	03e8fa00 	.word	0x03e8fa00
 800363c:	00271000 	.word	0x00271000
 8003640:	02dc6c00 	.word	0x02dc6c00
 8003644:	000493e0 	.word	0x000493e0
