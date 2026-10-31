
test-firmware/fd6288-S4U-ADJACENT-20261028/S4U_ADJACENT.elf:     file format elf32-littlearm


Disassembly of section .text:

08001f30 <fdBenchService>:
 8001f30:	f3ef 8210 	mrs	r2, PRIMASK
 8001f34:	b672      	cpsid	i
 8001f36:	4bab      	ldr	r3, [pc, #684]	@ (80021e4 <fdBenchService+0x2b4>)
 8001f38:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8001f3c:	b311      	cbz	r1, 8001f84 <fdBenchService+0x54>
 8001f3e:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8001f42:	b9f9      	cbnz	r1, 8001f84 <fdBenchService+0x54>
 8001f44:	b570      	push	{r4, r5, r6, lr}
 8001f46:	48a8      	ldr	r0, [pc, #672]	@ (80021e8 <fdBenchService+0x2b8>)
 8001f48:	49a8      	ldr	r1, [pc, #672]	@ (80021ec <fdBenchService+0x2bc>)
 8001f4a:	f8d0 c004 	ldr.w	ip, [r0, #4]
 8001f4e:	f891 047c 	ldrb.w	r0, [r1, #1148]	@ 0x47c
 8001f52:	b082      	sub	sp, #8
 8001f54:	b9c8      	cbnz	r0, 8001f8a <fdBenchService+0x5a>
 8001f56:	f891 00c2 	ldrb.w	r0, [r1, #194]	@ 0xc2
 8001f5a:	2800      	cmp	r0, #0
 8001f5c:	d169      	bne.n	8002032 <fdBenchService+0x102>
 8001f5e:	f8b1 0470 	ldrh.w	r0, [r1, #1136]	@ 0x470
 8001f62:	b9d8      	cbnz	r0, 8001f9c <fdBenchService+0x6c>
 8001f64:	f8d3 0674 	ldr.w	r0, [r3, #1652]	@ 0x674
 8001f68:	2800      	cmp	r0, #0
 8001f6a:	d168      	bne.n	800203e <fdBenchService+0x10e>
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
 8001fa2:	d054      	beq.n	800204e <fdBenchService+0x11e>
 8001fa4:	f891 00cc 	ldrb.w	r0, [r1, #204]	@ 0xcc
 8001fa8:	f891 e475 	ldrb.w	lr, [r1, #1141]	@ 0x475
 8001fac:	ea50 000e 	orrs.w	r0, r0, lr
 8001fb0:	d04d      	beq.n	800204e <fdBenchService+0x11e>
 8001fb2:	f891 0478 	ldrb.w	r0, [r1, #1144]	@ 0x478
 8001fb6:	2800      	cmp	r0, #0
 8001fb8:	d149      	bne.n	800204e <fdBenchService+0x11e>
 8001fba:	f891 e45c 	ldrb.w	lr, [r1, #1116]	@ 0x45c
 8001fbe:	f8d1 0428 	ldr.w	r0, [r1, #1064]	@ 0x428
 8001fc2:	f1be 0f04 	cmp.w	lr, #4
 8001fc6:	ebac 0500 	sub.w	r5, ip, r0
 8001fca:	d046      	beq.n	800205a <fdBenchService+0x12a>
 8001fcc:	f891 0440 	ldrb.w	r0, [r1, #1088]	@ 0x440
 8001fd0:	2800      	cmp	r0, #0
 8001fd2:	d158      	bne.n	8002086 <fdBenchService+0x156>
 8001fd4:	f1be 0f02 	cmp.w	lr, #2
 8001fd8:	d079      	beq.n	80020ce <fdBenchService+0x19e>
 8001fda:	4885      	ldr	r0, [pc, #532]	@ (80021f0 <fdBenchService+0x2c0>)
 8001fdc:	4285      	cmp	r5, r0
 8001fde:	d255      	bcs.n	800208c <fdBenchService+0x15c>
 8001fe0:	f1be 0f01 	cmp.w	lr, #1
 8001fe4:	d058      	beq.n	8002098 <fdBenchService+0x168>
 8001fe6:	f8df c20c 	ldr.w	ip, [pc, #524]	@ 80021f4 <fdBenchService+0x2c4>
 8001fea:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 8001fee:	4565      	cmp	r5, ip
 8001ff0:	f0c0 80a6 	bcc.w	8002140 <fdBenchService+0x210>
 8001ff4:	2800      	cmp	r0, #0
 8001ff6:	bf14      	ite	ne
 8001ff8:	2000      	movne	r0, #0
 8001ffa:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 8001ffe:	f1be 0f05 	cmp.w	lr, #5
 8002002:	bf08      	it	eq
 8002004:	2000      	moveq	r0, #0
 8002006:	f891 10c3 	ldrb.w	r1, [r1, #195]	@ 0xc3
 800200a:	f8b3 c006 	ldrh.w	ip, [r3, #6]
 800200e:	b139      	cbz	r1, 8002020 <fdBenchService+0xf0>
 8002010:	f9b3 1008 	ldrsh.w	r1, [r3, #8]
 8002014:	ea21 71e1 	bic.w	r1, r1, r1, asr #31
 8002018:	b289      	uxth	r1, r1
 800201a:	458c      	cmp	ip, r1
 800201c:	bf28      	it	cs
 800201e:	468c      	movcs	ip, r1
 8002020:	4560      	cmp	r0, ip
 8002022:	bf28      	it	cs
 8002024:	4660      	movcs	r0, ip
 8002026:	b280      	uxth	r0, r0
 8002028:	f8c3 0678 	str.w	r0, [r3, #1656]	@ 0x678
 800202c:	f1bc 0f00 	cmp.w	ip, #0
 8002030:	d1b0      	bne.n	8001f94 <fdBenchService+0x64>
 8002032:	2003      	movs	r0, #3
 8002034:	9201      	str	r2, [sp, #4]
 8002036:	f7ff ff05 	bl	8001e44 <fdBenchStop>
 800203a:	9a01      	ldr	r2, [sp, #4]
 800203c:	e7aa      	b.n	8001f94 <fdBenchService+0x64>
 800203e:	2007      	movs	r0, #7
 8002040:	9201      	str	r2, [sp, #4]
 8002042:	f7ff fe9b 	bl	8001d7c <fdBenchFinish.part.0>
 8002046:	4969      	ldr	r1, [pc, #420]	@ (80021ec <fdBenchService+0x2bc>)
 8002048:	4b66      	ldr	r3, [pc, #408]	@ (80021e4 <fdBenchService+0x2b4>)
 800204a:	9a01      	ldr	r2, [sp, #4]
 800204c:	e78e      	b.n	8001f6c <fdBenchService+0x3c>
 800204e:	2004      	movs	r0, #4
 8002050:	9201      	str	r2, [sp, #4]
 8002052:	f7ff fef7 	bl	8001e44 <fdBenchStop>
 8002056:	9a01      	ldr	r2, [sp, #4]
 8002058:	e79c      	b.n	8001f94 <fdBenchService+0x64>
 800205a:	f8d1 0450 	ldr.w	r0, [r1, #1104]	@ 0x450
 800205e:	f891 e46b 	ldrb.w	lr, [r1, #1131]	@ 0x46b
 8002062:	ebac 0000 	sub.w	r0, ip, r0
 8002066:	f1be 0f00 	cmp.w	lr, #0
 800206a:	d045      	beq.n	80020f8 <fdBenchService+0x1c8>
 800206c:	4c62      	ldr	r4, [pc, #392]	@ (80021f8 <fdBenchService+0x2c8>)
 800206e:	42a0      	cmp	r0, r4
 8002070:	d307      	bcc.n	8002082 <fdBenchService+0x152>
 8002072:	f8d1 0460 	ldr.w	r0, [r1, #1120]	@ 0x460
 8002076:	ebac 0c00 	sub.w	ip, ip, r0
 800207a:	4860      	ldr	r0, [pc, #384]	@ (80021fc <fdBenchService+0x2cc>)
 800207c:	4584      	cmp	ip, r0
 800207e:	f080 8087 	bcs.w	8002190 <fdBenchService+0x260>
 8002082:	2000      	movs	r0, #0
 8002084:	e7bf      	b.n	8002006 <fdBenchService+0xd6>
 8002086:	485e      	ldr	r0, [pc, #376]	@ (8002200 <fdBenchService+0x2d0>)
 8002088:	4285      	cmp	r5, r0
 800208a:	d3a9      	bcc.n	8001fe0 <fdBenchService+0xb0>
 800208c:	2001      	movs	r0, #1
 800208e:	9201      	str	r2, [sp, #4]
 8002090:	f7ff fed8 	bl	8001e44 <fdBenchStop>
 8002094:	9a01      	ldr	r2, [sp, #4]
 8002096:	e77d      	b.n	8001f94 <fdBenchService+0x64>
 8002098:	f891 0442 	ldrb.w	r0, [r1, #1090]	@ 0x442
 800209c:	2801      	cmp	r0, #1
 800209e:	d04c      	beq.n	800213a <fdBenchService+0x20a>
 80020a0:	f8d1 0444 	ldr.w	r0, [r1, #1092]	@ 0x444
 80020a4:	ebac 0c00 	sub.w	ip, ip, r0
 80020a8:	4856      	ldr	r0, [pc, #344]	@ (8002204 <fdBenchService+0x2d4>)
 80020aa:	4584      	cmp	ip, r0
 80020ac:	d845      	bhi.n	800213a <fdBenchService+0x20a>
 80020ae:	4856      	ldr	r0, [pc, #344]	@ (8002208 <fdBenchService+0x2d8>)
 80020b0:	fba0 c00c 	umull	ip, r0, r0, ip
 80020b4:	09c0      	lsrs	r0, r0, #7
 80020b6:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 80020ba:	fb0c f000 	mul.w	r0, ip, r0
 80020be:	f8df c14c 	ldr.w	ip, [pc, #332]	@ 800220c <fdBenchService+0x2dc>
 80020c2:	fbac c000 	umull	ip, r0, ip, r0
 80020c6:	0b40      	lsrs	r0, r0, #13
 80020c8:	30d2      	adds	r0, #210	@ 0xd2
 80020ca:	b280      	uxth	r0, r0
 80020cc:	e79b      	b.n	8002006 <fdBenchService+0xd6>
 80020ce:	f891 0441 	ldrb.w	r0, [r1, #1089]	@ 0x441
 80020d2:	2800      	cmp	r0, #0
 80020d4:	d03a      	beq.n	800214c <fdBenchService+0x21c>
 80020d6:	4846      	ldr	r0, [pc, #280]	@ (80021f0 <fdBenchService+0x2c0>)
 80020d8:	4285      	cmp	r5, r0
 80020da:	d2d7      	bcs.n	800208c <fdBenchService+0x15c>
 80020dc:	4845      	ldr	r0, [pc, #276]	@ (80021f4 <fdBenchService+0x2c4>)
 80020de:	4285      	cmp	r5, r0
 80020e0:	bf34      	ite	cc
 80020e2:	f44f 70b4 	movcc.w	r0, #360	@ 0x168
 80020e6:	f44f 70c8 	movcs.w	r0, #400	@ 0x190
 80020ea:	f891 c46b 	ldrb.w	ip, [r1, #1131]	@ 0x46b
 80020ee:	f1bc 0f00 	cmp.w	ip, #0
 80020f2:	bf18      	it	ne
 80020f4:	2000      	movne	r0, #0
 80020f6:	e786      	b.n	8002006 <fdBenchService+0xd6>
 80020f8:	f8d1 4460 	ldr.w	r4, [r1, #1120]	@ 0x460
 80020fc:	ebac 0c04 	sub.w	ip, ip, r4
 8002100:	4c43      	ldr	r4, [pc, #268]	@ (8002210 <fdBenchService+0x2e0>)
 8002102:	45a4      	cmp	ip, r4
 8002104:	d231      	bcs.n	800216a <fdBenchService+0x23a>
 8002106:	4c3c      	ldr	r4, [pc, #240]	@ (80021f8 <fdBenchService+0x2c8>)
 8002108:	42a0      	cmp	r0, r4
 800210a:	f080 8092 	bcs.w	8002232 <fdBenchService+0x302>
 800210e:	f8df c0e4 	ldr.w	ip, [pc, #228]	@ 80021f4 <fdBenchService+0x2c4>
 8002112:	4565      	cmp	r5, ip
 8002114:	d311      	bcc.n	800213a <fdBenchService+0x20a>
 8002116:	f06f 0c27 	mvn.w	ip, #39	@ 0x27
 800211a:	fb0c f000 	mul.w	r0, ip, r0
 800211e:	f100 60f4 	add.w	r0, r0, #127926272	@ 0x7a00000
 8002122:	4c35      	ldr	r4, [pc, #212]	@ (80021f8 <fdBenchService+0x2c8>)
 8002124:	f500 3090 	add.w	r0, r0, #73728	@ 0x12000
 8002128:	fbb0 f0f4 	udiv	r0, r0, r4
 800212c:	f500 70b4 	add.w	r0, r0, #360	@ 0x168
 8002130:	b280      	uxth	r0, r0
 8002132:	e768      	b.n	8002006 <fdBenchService+0xd6>
 8002134:	f1be 0f00 	cmp.w	lr, #0
 8002138:	d1a3      	bne.n	8002082 <fdBenchService+0x152>
 800213a:	f44f 70b4 	mov.w	r0, #360	@ 0x168
 800213e:	e762      	b.n	8002006 <fdBenchService+0xd6>
 8002140:	2800      	cmp	r0, #0
 8002142:	bf14      	ite	ne
 8002144:	2000      	movne	r0, #0
 8002146:	f44f 70b4 	moveq.w	r0, #360	@ 0x168
 800214a:	e758      	b.n	8001ffe <fdBenchService+0xce>
 800214c:	f891 046a 	ldrb.w	r0, [r1, #1130]	@ 0x46a
 8002150:	b178      	cbz	r0, 8002172 <fdBenchService+0x242>
 8002152:	f891 0469 	ldrb.w	r0, [r1, #1129]	@ 0x469
 8002156:	b960      	cbnz	r0, 8002172 <fdBenchService+0x242>
 8002158:	4825      	ldr	r0, [pc, #148]	@ (80021f0 <fdBenchService+0x2c0>)
 800215a:	4285      	cmp	r5, r0
 800215c:	d30c      	bcc.n	8002178 <fdBenchService+0x248>
 800215e:	482d      	ldr	r0, [pc, #180]	@ (8002214 <fdBenchService+0x2e4>)
 8002160:	4285      	cmp	r5, r0
 8002162:	d293      	bcs.n	800208c <fdBenchService+0x15c>
 8002164:	f44f 70c8 	mov.w	r0, #400	@ 0x190
 8002168:	e7bf      	b.n	80020ea <fdBenchService+0x1ba>
 800216a:	2001      	movs	r0, #1
 800216c:	f881 046b 	strb.w	r0, [r1, #1131]	@ 0x46b
 8002170:	e787      	b.n	8002082 <fdBenchService+0x152>
 8002172:	481f      	ldr	r0, [pc, #124]	@ (80021f0 <fdBenchService+0x2c0>)
 8002174:	4285      	cmp	r5, r0
 8002176:	d289      	bcs.n	800208c <fdBenchService+0x15c>
 8002178:	f8df c078 	ldr.w	ip, [pc, #120]	@ 80021f4 <fdBenchService+0x2c4>
 800217c:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 8002180:	4565      	cmp	r5, ip
 8002182:	d328      	bcc.n	80021d6 <fdBenchService+0x2a6>
 8002184:	2800      	cmp	r0, #0
 8002186:	bf14      	ite	ne
 8002188:	2000      	movne	r0, #0
 800218a:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 800218e:	e73a      	b.n	8002006 <fdBenchService+0xd6>
 8002190:	8818      	ldrh	r0, [r3, #0]
 8002192:	4c21      	ldr	r4, [pc, #132]	@ (8002218 <fdBenchService+0x2e8>)
 8002194:	f1ac 7cb7 	sub.w	ip, ip, #23986176	@ 0x16e0000
 8002198:	f5ac 5c58 	sub.w	ip, ip, #13824	@ 0x3600
 800219c:	45a4      	cmp	ip, r4
 800219e:	d253      	bcs.n	8002248 <fdBenchService+0x318>
 80021a0:	885c      	ldrh	r4, [r3, #2]
 80021a2:	4284      	cmp	r4, r0
 80021a4:	d23c      	bcs.n	8002220 <fdBenchService+0x2f0>
 80021a6:	4e13      	ldr	r6, [pc, #76]	@ (80021f4 <fdBenchService+0x2c4>)
 80021a8:	42b5      	cmp	r5, r6
 80021aa:	d3c3      	bcc.n	8002134 <fdBenchService+0x204>
 80021ac:	f1be 0f00 	cmp.w	lr, #0
 80021b0:	f47f af67 	bne.w	8002082 <fdBenchService+0x152>
 80021b4:	eba0 0e04 	sub.w	lr, r0, r4
 80021b8:	f1cc 7c37 	rsb	ip, ip, #47972352	@ 0x2dc0000
 80021bc:	4d17      	ldr	r5, [pc, #92]	@ (800221c <fdBenchService+0x2ec>)
 80021be:	f50c 4cd8 	add.w	ip, ip, #27648	@ 0x6c00
 80021c2:	20a0      	movs	r0, #160	@ 0xa0
 80021c4:	fbbc f0f0 	udiv	r0, ip, r0
 80021c8:	fb0e f000 	mul.w	r0, lr, r0
 80021cc:	fbb0 f0f5 	udiv	r0, r0, r5
 80021d0:	4420      	add	r0, r4
 80021d2:	b280      	uxth	r0, r0
 80021d4:	e717      	b.n	8002006 <fdBenchService+0xd6>
 80021d6:	2800      	cmp	r0, #0
 80021d8:	bf14      	ite	ne
 80021da:	2000      	movne	r0, #0
 80021dc:	f44f 70b4 	moveq.w	r0, #360	@ 0x168
 80021e0:	e711      	b.n	8002006 <fdBenchService+0xd6>
 80021e2:	bf00      	nop
 80021e4:	20000000 	.word	0x20000000
 80021e8:	e0001000 	.word	0xe0001000
 80021ec:	20001184 	.word	0x20001184
 80021f0:	03b82600 	.word	0x03b82600
 80021f4:	00493e00 	.word	0x00493e00
 80021f8:	0030d400 	.word	0x0030d400
 80021fc:	016e3600 	.word	0x016e3600
 8002200:	07d1f400 	.word	0x07d1f400
 8002204:	00186a9f 	.word	0x00186a9f
 8002208:	cccccccd 	.word	0xcccccccd
 800220c:	d1b71759 	.word	0xd1b71759
 8002210:	09896800 	.word	0x09896800
 8002214:	03e8fa00 	.word	0x03e8fa00
 8002218:	02dc6c00 	.word	0x02dc6c00
 800221c:	000493e0 	.word	0x000493e0
 8002220:	f8df c038 	ldr.w	ip, [pc, #56]	@ 800225c <fdBenchService+0x32c>
 8002224:	4565      	cmp	r5, ip
 8002226:	d385      	bcc.n	8002134 <fdBenchService+0x204>
 8002228:	f1be 0f00 	cmp.w	lr, #0
 800222c:	f47f af29 	bne.w	8002082 <fdBenchService+0x152>
 8002230:	e6e9      	b.n	8002006 <fdBenchService+0xd6>
 8002232:	4c0b      	ldr	r4, [pc, #44]	@ (8002260 <fdBenchService+0x330>)
 8002234:	8818      	ldrh	r0, [r3, #0]
 8002236:	45a4      	cmp	ip, r4
 8002238:	d2ab      	bcs.n	8002192 <fdBenchService+0x262>
 800223a:	f8df c020 	ldr.w	ip, [pc, #32]	@ 800225c <fdBenchService+0x32c>
 800223e:	4565      	cmp	r5, ip
 8002240:	bf38      	it	cc
 8002242:	f44f 70b4 	movcc.w	r0, #360	@ 0x168
 8002246:	e6de      	b.n	8002006 <fdBenchService+0xd6>
 8002248:	4804      	ldr	r0, [pc, #16]	@ (800225c <fdBenchService+0x32c>)
 800224a:	4285      	cmp	r5, r0
 800224c:	f4bf af19 	bcs.w	8002082 <fdBenchService+0x152>
 8002250:	f891 046b 	ldrb.w	r0, [r1, #1131]	@ 0x46b
 8002254:	2800      	cmp	r0, #0
 8002256:	f43f af70 	beq.w	800213a <fdBenchService+0x20a>
 800225a:	e712      	b.n	8002082 <fdBenchService+0x152>
 800225c:	00493e00 	.word	0x00493e00
 8002260:	016e3600 	.word	0x016e3600
