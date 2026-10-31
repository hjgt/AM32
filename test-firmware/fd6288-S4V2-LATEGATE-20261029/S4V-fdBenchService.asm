
test-firmware/fd6288-S4V-STALE-20261028/S4V_STALE.elf:     file format elf32-littlearm


Disassembly of section .text:

08001f30 <fdBenchService>:
 8001f30:	f3ef 8210 	mrs	r2, PRIMASK
 8001f34:	b672      	cpsid	i
 8001f36:	4bad      	ldr	r3, [pc, #692]	@ (80021ec <fdBenchService+0x2bc>)
 8001f38:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8001f3c:	b311      	cbz	r1, 8001f84 <fdBenchService+0x54>
 8001f3e:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8001f42:	b9f9      	cbnz	r1, 8001f84 <fdBenchService+0x54>
 8001f44:	b570      	push	{r4, r5, r6, lr}
 8001f46:	48aa      	ldr	r0, [pc, #680]	@ (80021f0 <fdBenchService+0x2c0>)
 8001f48:	49aa      	ldr	r1, [pc, #680]	@ (80021f4 <fdBenchService+0x2c4>)
 8001f4a:	f8d0 c004 	ldr.w	ip, [r0, #4]
 8001f4e:	f891 047c 	ldrb.w	r0, [r1, #1148]	@ 0x47c
 8001f52:	b082      	sub	sp, #8
 8001f54:	b9c8      	cbnz	r0, 8001f8a <fdBenchService+0x5a>
 8001f56:	f891 00c2 	ldrb.w	r0, [r1, #194]	@ 0xc2
 8001f5a:	2800      	cmp	r0, #0
 8001f5c:	d177      	bne.n	800204e <fdBenchService+0x11e>
 8001f5e:	f8b1 0470 	ldrh.w	r0, [r1, #1136]	@ 0x470
 8001f62:	b9d8      	cbnz	r0, 8001f9c <fdBenchService+0x6c>
 8001f64:	f8d3 0674 	ldr.w	r0, [r3, #1652]	@ 0x674
 8001f68:	2800      	cmp	r0, #0
 8001f6a:	d176      	bne.n	800205a <fdBenchService+0x12a>
 8001f6c:	2000      	movs	r0, #0
 8001f6e:	f04f 0c01 	mov.w	ip, #1
 8001f72:	f881 0474 	strb.w	r0, [r1, #1140]	@ 0x474
 8001f76:	f8a1 0430 	strh.w	r0, [r1, #1072]	@ 0x430
 8001f7a:	f8c1 c432 	str.w	ip, [r1, #1074]	@ 0x432
 8001f7e:	f8c3 0678 	str.w	r0, [r3, #1656]	@ 0x678
 8001f82:	e007      	b.n	8001f94 <fdBenchService+0x64>
 8001f84:	f382 8810 	msr	PRIMASK, r2
 8001f88:	4770      	bx	lr
 8001f8a:	2002      	movs	r0, #2
 8001f8c:	9201      	str	r2, [sp, #4]
 8001f8e:	f7ff ff59 	bl	8001e44 <fdBenchStop>
 8001f92:	9a01      	ldr	r2, [sp, #4]
 8001f94:	f382 8810 	msr	PRIMASK, r2
 8001f98:	b002      	add	sp, #8
 8001f9a:	bd70      	pop	{r4, r5, r6, pc}
 8001f9c:	f891 047d 	ldrb.w	r0, [r1, #1149]	@ 0x47d
 8001fa0:	2800      	cmp	r0, #0
 8001fa2:	d062      	beq.n	800206a <fdBenchService+0x13a>
 8001fa4:	f891 00cc 	ldrb.w	r0, [r1, #204]	@ 0xcc
 8001fa8:	f891 e475 	ldrb.w	lr, [r1, #1141]	@ 0x475
 8001fac:	ea50 000e 	orrs.w	r0, r0, lr
 8001fb0:	d05b      	beq.n	800206a <fdBenchService+0x13a>
 8001fb2:	f891 0478 	ldrb.w	r0, [r1, #1144]	@ 0x478
 8001fb6:	2800      	cmp	r0, #0
 8001fb8:	d157      	bne.n	800206a <fdBenchService+0x13a>
 8001fba:	f891 e45c 	ldrb.w	lr, [r1, #1116]	@ 0x45c
 8001fbe:	f8d1 0428 	ldr.w	r0, [r1, #1064]	@ 0x428
 8001fc2:	f1be 0f04 	cmp.w	lr, #4
 8001fc6:	ebac 0500 	sub.w	r5, ip, r0
 8001fca:	d054      	beq.n	8002076 <fdBenchService+0x146>
 8001fcc:	f891 0440 	ldrb.w	r0, [r1, #1088]	@ 0x440
 8001fd0:	2800      	cmp	r0, #0
 8001fd2:	d166      	bne.n	80020a2 <fdBenchService+0x172>
 8001fd4:	f891 0442 	ldrb.w	r0, [r1, #1090]	@ 0x442
 8001fd8:	2802      	cmp	r0, #2
 8001fda:	d908      	bls.n	8001fee <fdBenchService+0xbe>
 8001fdc:	f8d1 045c 	ldr.w	r0, [r1, #1116]	@ 0x45c
 8001fe0:	f020 407f 	bic.w	r0, r0, #4278190080	@ 0xff000000
 8001fe4:	f020 00ff 	bic.w	r0, r0, #255	@ 0xff
 8001fe8:	2800      	cmp	r0, #0
 8001fea:	f000 80b8 	beq.w	800215e <fdBenchService+0x22e>
 8001fee:	f1be 0f02 	cmp.w	lr, #2
 8001ff2:	f000 80a0 	beq.w	8002136 <fdBenchService+0x206>
 8001ff6:	4880      	ldr	r0, [pc, #512]	@ (80021f8 <fdBenchService+0x2c8>)
 8001ff8:	4285      	cmp	r5, r0
 8001ffa:	d255      	bcs.n	80020a8 <fdBenchService+0x178>
 8001ffc:	f1be 0f01 	cmp.w	lr, #1
 8002000:	d058      	beq.n	80020b4 <fdBenchService+0x184>
 8002002:	f8df c1f8 	ldr.w	ip, [pc, #504]	@ 80021fc <fdBenchService+0x2cc>
 8002006:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 800200a:	4565      	cmp	r5, ip
 800200c:	f0c0 8089 	bcc.w	8002122 <fdBenchService+0x1f2>
 8002010:	2800      	cmp	r0, #0
 8002012:	bf14      	ite	ne
 8002014:	2000      	movne	r0, #0
 8002016:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 800201a:	f1be 0f05 	cmp.w	lr, #5
 800201e:	bf08      	it	eq
 8002020:	2000      	moveq	r0, #0
 8002022:	f891 10c3 	ldrb.w	r1, [r1, #195]	@ 0xc3
 8002026:	f8b3 c006 	ldrh.w	ip, [r3, #6]
 800202a:	b139      	cbz	r1, 800203c <fdBenchService+0x10c>
 800202c:	f9b3 1008 	ldrsh.w	r1, [r3, #8]
 8002030:	ea21 71e1 	bic.w	r1, r1, r1, asr #31
 8002034:	b289      	uxth	r1, r1
 8002036:	458c      	cmp	ip, r1
 8002038:	bf28      	it	cs
 800203a:	468c      	movcs	ip, r1
 800203c:	4560      	cmp	r0, ip
 800203e:	bf28      	it	cs
 8002040:	4660      	movcs	r0, ip
 8002042:	b280      	uxth	r0, r0
 8002044:	f8c3 0678 	str.w	r0, [r3, #1656]	@ 0x678
 8002048:	f1bc 0f00 	cmp.w	ip, #0
 800204c:	d1a2      	bne.n	8001f94 <fdBenchService+0x64>
 800204e:	2003      	movs	r0, #3
 8002050:	9201      	str	r2, [sp, #4]
 8002052:	f7ff fef7 	bl	8001e44 <fdBenchStop>
 8002056:	9a01      	ldr	r2, [sp, #4]
 8002058:	e79c      	b.n	8001f94 <fdBenchService+0x64>
 800205a:	2007      	movs	r0, #7
 800205c:	9201      	str	r2, [sp, #4]
 800205e:	f7ff fe8d 	bl	8001d7c <fdBenchFinish.part.0>
 8002062:	4964      	ldr	r1, [pc, #400]	@ (80021f4 <fdBenchService+0x2c4>)
 8002064:	4b61      	ldr	r3, [pc, #388]	@ (80021ec <fdBenchService+0x2bc>)
 8002066:	9a01      	ldr	r2, [sp, #4]
 8002068:	e780      	b.n	8001f6c <fdBenchService+0x3c>
 800206a:	2004      	movs	r0, #4
 800206c:	9201      	str	r2, [sp, #4]
 800206e:	f7ff fee9 	bl	8001e44 <fdBenchStop>
 8002072:	9a01      	ldr	r2, [sp, #4]
 8002074:	e78e      	b.n	8001f94 <fdBenchService+0x64>
 8002076:	f8d1 0450 	ldr.w	r0, [r1, #1104]	@ 0x450
 800207a:	f891 e46b 	ldrb.w	lr, [r1, #1131]	@ 0x46b
 800207e:	ebac 0000 	sub.w	r0, ip, r0
 8002082:	f1be 0f00 	cmp.w	lr, #0
 8002086:	d030      	beq.n	80020ea <fdBenchService+0x1ba>
 8002088:	4c5d      	ldr	r4, [pc, #372]	@ (8002200 <fdBenchService+0x2d0>)
 800208a:	42a0      	cmp	r0, r4
 800208c:	d307      	bcc.n	800209e <fdBenchService+0x16e>
 800208e:	f8d1 0460 	ldr.w	r0, [r1, #1120]	@ 0x460
 8002092:	ebac 0c00 	sub.w	ip, ip, r0
 8002096:	485b      	ldr	r0, [pc, #364]	@ (8002204 <fdBenchService+0x2d4>)
 8002098:	4584      	cmp	ip, r0
 800209a:	f080 8083 	bcs.w	80021a4 <fdBenchService+0x274>
 800209e:	2000      	movs	r0, #0
 80020a0:	e7bf      	b.n	8002022 <fdBenchService+0xf2>
 80020a2:	4859      	ldr	r0, [pc, #356]	@ (8002208 <fdBenchService+0x2d8>)
 80020a4:	4285      	cmp	r5, r0
 80020a6:	d3a9      	bcc.n	8001ffc <fdBenchService+0xcc>
 80020a8:	2001      	movs	r0, #1
 80020aa:	9201      	str	r2, [sp, #4]
 80020ac:	f7ff feca 	bl	8001e44 <fdBenchStop>
 80020b0:	9a01      	ldr	r2, [sp, #4]
 80020b2:	e76f      	b.n	8001f94 <fdBenchService+0x64>
 80020b4:	f891 0443 	ldrb.w	r0, [r1, #1091]	@ 0x443
 80020b8:	2801      	cmp	r0, #1
 80020ba:	d02f      	beq.n	800211c <fdBenchService+0x1ec>
 80020bc:	f8d1 0444 	ldr.w	r0, [r1, #1092]	@ 0x444
 80020c0:	ebac 0c00 	sub.w	ip, ip, r0
 80020c4:	4851      	ldr	r0, [pc, #324]	@ (800220c <fdBenchService+0x2dc>)
 80020c6:	4584      	cmp	ip, r0
 80020c8:	d828      	bhi.n	800211c <fdBenchService+0x1ec>
 80020ca:	4851      	ldr	r0, [pc, #324]	@ (8002210 <fdBenchService+0x2e0>)
 80020cc:	fba0 c00c 	umull	ip, r0, r0, ip
 80020d0:	09c0      	lsrs	r0, r0, #7
 80020d2:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 80020d6:	fb0c f000 	mul.w	r0, ip, r0
 80020da:	f8df c138 	ldr.w	ip, [pc, #312]	@ 8002214 <fdBenchService+0x2e4>
 80020de:	fbac c000 	umull	ip, r0, ip, r0
 80020e2:	0b40      	lsrs	r0, r0, #13
 80020e4:	30d2      	adds	r0, #210	@ 0xd2
 80020e6:	b280      	uxth	r0, r0
 80020e8:	e79b      	b.n	8002022 <fdBenchService+0xf2>
 80020ea:	f8d1 4460 	ldr.w	r4, [r1, #1120]	@ 0x460
 80020ee:	ebac 0c04 	sub.w	ip, ip, r4
 80020f2:	4c49      	ldr	r4, [pc, #292]	@ (8002218 <fdBenchService+0x2e8>)
 80020f4:	45a4      	cmp	ip, r4
 80020f6:	d21a      	bcs.n	800212e <fdBenchService+0x1fe>
 80020f8:	4c41      	ldr	r4, [pc, #260]	@ (8002200 <fdBenchService+0x2d0>)
 80020fa:	42a0      	cmp	r0, r4
 80020fc:	f0c0 80bb 	bcc.w	8002276 <fdBenchService+0x346>
 8002100:	4c40      	ldr	r4, [pc, #256]	@ (8002204 <fdBenchService+0x2d4>)
 8002102:	8818      	ldrh	r0, [r3, #0]
 8002104:	45a4      	cmp	ip, r4
 8002106:	d24e      	bcs.n	80021a6 <fdBenchService+0x276>
 8002108:	f8df c0f0 	ldr.w	ip, [pc, #240]	@ 80021fc <fdBenchService+0x2cc>
 800210c:	4565      	cmp	r5, ip
 800210e:	bf38      	it	cc
 8002110:	f44f 70b4 	movcc.w	r0, #360	@ 0x168
 8002114:	e785      	b.n	8002022 <fdBenchService+0xf2>
 8002116:	f1be 0f00 	cmp.w	lr, #0
 800211a:	d1c0      	bne.n	800209e <fdBenchService+0x16e>
 800211c:	f44f 70b4 	mov.w	r0, #360	@ 0x168
 8002120:	e77f      	b.n	8002022 <fdBenchService+0xf2>
 8002122:	2800      	cmp	r0, #0
 8002124:	bf14      	ite	ne
 8002126:	2000      	movne	r0, #0
 8002128:	f44f 70b4 	moveq.w	r0, #360	@ 0x168
 800212c:	e775      	b.n	800201a <fdBenchService+0xea>
 800212e:	2001      	movs	r0, #1
 8002130:	f881 046b 	strb.w	r0, [r1, #1131]	@ 0x46b
 8002134:	e7b3      	b.n	800209e <fdBenchService+0x16e>
 8002136:	f891 0441 	ldrb.w	r0, [r1, #1089]	@ 0x441
 800213a:	b1e0      	cbz	r0, 8002176 <fdBenchService+0x246>
 800213c:	482e      	ldr	r0, [pc, #184]	@ (80021f8 <fdBenchService+0x2c8>)
 800213e:	4285      	cmp	r5, r0
 8002140:	d2b2      	bcs.n	80020a8 <fdBenchService+0x178>
 8002142:	482e      	ldr	r0, [pc, #184]	@ (80021fc <fdBenchService+0x2cc>)
 8002144:	f891 c46b 	ldrb.w	ip, [r1, #1131]	@ 0x46b
 8002148:	4285      	cmp	r5, r0
 800214a:	bf34      	ite	cc
 800214c:	f44f 70b4 	movcc.w	r0, #360	@ 0x168
 8002150:	f44f 70c8 	movcs.w	r0, #400	@ 0x190
 8002154:	f1bc 0f00 	cmp.w	ip, #0
 8002158:	bf18      	it	ne
 800215a:	2000      	movne	r0, #0
 800215c:	e761      	b.n	8002022 <fdBenchService+0xf2>
 800215e:	f8d1 0450 	ldr.w	r0, [r1, #1104]	@ 0x450
 8002162:	4c2e      	ldr	r4, [pc, #184]	@ (800221c <fdBenchService+0x2ec>)
 8002164:	ebac 0000 	sub.w	r0, ip, r0
 8002168:	42a0      	cmp	r0, r4
 800216a:	f4ff af40 	bcc.w	8001fee <fdBenchService+0xbe>
 800216e:	f1be 0f02 	cmp.w	lr, #2
 8002172:	f47f af40 	bne.w	8001ff6 <fdBenchService+0xc6>
 8002176:	f891 046a 	ldrb.w	r0, [r1, #1130]	@ 0x46a
 800217a:	2800      	cmp	r0, #0
 800217c:	d056      	beq.n	800222c <fdBenchService+0x2fc>
 800217e:	f891 0469 	ldrb.w	r0, [r1, #1129]	@ 0x469
 8002182:	2800      	cmp	r0, #0
 8002184:	d16c      	bne.n	8002260 <fdBenchService+0x330>
 8002186:	481c      	ldr	r0, [pc, #112]	@ (80021f8 <fdBenchService+0x2c8>)
 8002188:	4285      	cmp	r5, r0
 800218a:	f4ff af3a 	bcc.w	8002002 <fdBenchService+0xd2>
 800218e:	4824      	ldr	r0, [pc, #144]	@ (8002220 <fdBenchService+0x2f0>)
 8002190:	4285      	cmp	r5, r0
 8002192:	d289      	bcs.n	80020a8 <fdBenchService+0x178>
 8002194:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 8002198:	2800      	cmp	r0, #0
 800219a:	bf14      	ite	ne
 800219c:	2000      	movne	r0, #0
 800219e:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 80021a2:	e73e      	b.n	8002022 <fdBenchService+0xf2>
 80021a4:	8818      	ldrh	r0, [r3, #0]
 80021a6:	4c1f      	ldr	r4, [pc, #124]	@ (8002224 <fdBenchService+0x2f4>)
 80021a8:	f1ac 7cb7 	sub.w	ip, ip, #23986176	@ 0x16e0000
 80021ac:	f5ac 5c58 	sub.w	ip, ip, #13824	@ 0x3600
 80021b0:	45a4      	cmp	ip, r4
 80021b2:	d274      	bcs.n	800229e <fdBenchService+0x36e>
 80021b4:	885c      	ldrh	r4, [r3, #2]
 80021b6:	4284      	cmp	r4, r0
 80021b8:	d248      	bcs.n	800224c <fdBenchService+0x31c>
 80021ba:	4e10      	ldr	r6, [pc, #64]	@ (80021fc <fdBenchService+0x2cc>)
 80021bc:	42b5      	cmp	r5, r6
 80021be:	d3aa      	bcc.n	8002116 <fdBenchService+0x1e6>
 80021c0:	f1be 0f00 	cmp.w	lr, #0
 80021c4:	f47f af6b 	bne.w	800209e <fdBenchService+0x16e>
 80021c8:	eba0 0e04 	sub.w	lr, r0, r4
 80021cc:	f1cc 7c37 	rsb	ip, ip, #47972352	@ 0x2dc0000
 80021d0:	4d15      	ldr	r5, [pc, #84]	@ (8002228 <fdBenchService+0x2f8>)
 80021d2:	f50c 4cd8 	add.w	ip, ip, #27648	@ 0x6c00
 80021d6:	20a0      	movs	r0, #160	@ 0xa0
 80021d8:	fbbc f0f0 	udiv	r0, ip, r0
 80021dc:	fb0e f000 	mul.w	r0, lr, r0
 80021e0:	fbb0 f0f5 	udiv	r0, r0, r5
 80021e4:	4420      	add	r0, r4
 80021e6:	b280      	uxth	r0, r0
 80021e8:	e71b      	b.n	8002022 <fdBenchService+0xf2>
 80021ea:	bf00      	nop
 80021ec:	20000000 	.word	0x20000000
 80021f0:	e0001000 	.word	0xe0001000
 80021f4:	20001184 	.word	0x20001184
 80021f8:	03b82600 	.word	0x03b82600
 80021fc:	00493e00 	.word	0x00493e00
 8002200:	0030d400 	.word	0x0030d400
 8002204:	016e3600 	.word	0x016e3600
 8002208:	07d1f400 	.word	0x07d1f400
 800220c:	00186a9f 	.word	0x00186a9f
 8002210:	cccccccd 	.word	0xcccccccd
 8002214:	d1b71759 	.word	0xd1b71759
 8002218:	09896800 	.word	0x09896800
 800221c:	00a4cb80 	.word	0x00a4cb80
 8002220:	03e8fa00 	.word	0x03e8fa00
 8002224:	02dc6c00 	.word	0x02dc6c00
 8002228:	000493e0 	.word	0x000493e0
 800222c:	4821      	ldr	r0, [pc, #132]	@ (80022b4 <fdBenchService+0x384>)
 800222e:	4285      	cmp	r5, r0
 8002230:	f4bf af3a 	bcs.w	80020a8 <fdBenchService+0x178>
 8002234:	f8df c080 	ldr.w	ip, [pc, #128]	@ 80022b8 <fdBenchService+0x388>
 8002238:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 800223c:	4565      	cmp	r5, ip
 800223e:	d314      	bcc.n	800226a <fdBenchService+0x33a>
 8002240:	2800      	cmp	r0, #0
 8002242:	bf14      	ite	ne
 8002244:	2000      	movne	r0, #0
 8002246:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 800224a:	e6ea      	b.n	8002022 <fdBenchService+0xf2>
 800224c:	f8df c068 	ldr.w	ip, [pc, #104]	@ 80022b8 <fdBenchService+0x388>
 8002250:	4565      	cmp	r5, ip
 8002252:	f4ff af60 	bcc.w	8002116 <fdBenchService+0x1e6>
 8002256:	f1be 0f00 	cmp.w	lr, #0
 800225a:	f47f af20 	bne.w	800209e <fdBenchService+0x16e>
 800225e:	e6e0      	b.n	8002022 <fdBenchService+0xf2>
 8002260:	4814      	ldr	r0, [pc, #80]	@ (80022b4 <fdBenchService+0x384>)
 8002262:	4285      	cmp	r5, r0
 8002264:	f4ff aecd 	bcc.w	8002002 <fdBenchService+0xd2>
 8002268:	e71e      	b.n	80020a8 <fdBenchService+0x178>
 800226a:	2800      	cmp	r0, #0
 800226c:	bf14      	ite	ne
 800226e:	2000      	movne	r0, #0
 8002270:	f44f 70b4 	moveq.w	r0, #360	@ 0x168
 8002274:	e6d5      	b.n	8002022 <fdBenchService+0xf2>
 8002276:	f8df c040 	ldr.w	ip, [pc, #64]	@ 80022b8 <fdBenchService+0x388>
 800227a:	4565      	cmp	r5, ip
 800227c:	f4ff af4e 	bcc.w	800211c <fdBenchService+0x1ec>
 8002280:	f06f 0c27 	mvn.w	ip, #39	@ 0x27
 8002284:	fb0c f000 	mul.w	r0, ip, r0
 8002288:	f100 60f4 	add.w	r0, r0, #127926272	@ 0x7a00000
 800228c:	4c0b      	ldr	r4, [pc, #44]	@ (80022bc <fdBenchService+0x38c>)
 800228e:	f500 3090 	add.w	r0, r0, #73728	@ 0x12000
 8002292:	fbb0 f0f4 	udiv	r0, r0, r4
 8002296:	f500 70b4 	add.w	r0, r0, #360	@ 0x168
 800229a:	b280      	uxth	r0, r0
 800229c:	e6c1      	b.n	8002022 <fdBenchService+0xf2>
 800229e:	4806      	ldr	r0, [pc, #24]	@ (80022b8 <fdBenchService+0x388>)
 80022a0:	4285      	cmp	r5, r0
 80022a2:	f4bf aefc 	bcs.w	800209e <fdBenchService+0x16e>
 80022a6:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 80022aa:	2800      	cmp	r0, #0
 80022ac:	f43f af36 	beq.w	800211c <fdBenchService+0x1ec>
 80022b0:	e6f5      	b.n	800209e <fdBenchService+0x16e>
 80022b2:	bf00      	nop
 80022b4:	03b82600 	.word	0x03b82600
 80022b8:	00493e00 	.word	0x00493e00
 80022bc:	0030d400 	.word	0x0030d400
