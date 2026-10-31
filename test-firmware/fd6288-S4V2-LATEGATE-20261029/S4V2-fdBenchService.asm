
obj-startup-S4J-alignnative-hold-acqstats-launch-settle-reposition-safe-fullretry-adjacent-stale-late/AM32_CUSTOM_FD6288_G431_2.20.elf:     file format elf32-littlearm


Disassembly of section .text:

08001fc4 <fdBenchService>:
 8001fc4:	f3ef 8210 	mrs	r2, PRIMASK
 8001fc8:	b672      	cpsid	i
 8001fca:	4ba0      	ldr	r3, [pc, #640]	@ (800224c <fdBenchService+0x288>)
 8001fcc:	f8d3 1674 	ldr.w	r1, [r3, #1652]	@ 0x674
 8001fd0:	b319      	cbz	r1, 800201a <fdBenchService+0x56>
 8001fd2:	f8d3 166c 	ldr.w	r1, [r3, #1644]	@ 0x66c
 8001fd6:	bb01      	cbnz	r1, 800201a <fdBenchService+0x56>
 8001fd8:	b5f0      	push	{r4, r5, r6, r7, lr}
 8001fda:	489d      	ldr	r0, [pc, #628]	@ (8002250 <fdBenchService+0x28c>)
 8001fdc:	499d      	ldr	r1, [pc, #628]	@ (8002254 <fdBenchService+0x290>)
 8001fde:	6840      	ldr	r0, [r0, #4]
 8001fe0:	4606      	mov	r6, r0
 8001fe2:	f891 047c 	ldrb.w	r0, [r1, #1148]	@ 0x47c
 8001fe6:	b083      	sub	sp, #12
 8001fe8:	b9d0      	cbnz	r0, 8002020 <fdBenchService+0x5c>
 8001fea:	f891 00c2 	ldrb.w	r0, [r1, #194]	@ 0xc2
 8001fee:	2800      	cmp	r0, #0
 8001ff0:	d14b      	bne.n	800208a <fdBenchService+0xc6>
 8001ff2:	f8b1 0470 	ldrh.w	r0, [r1, #1136]	@ 0x470
 8001ff6:	2800      	cmp	r0, #0
 8001ff8:	d14d      	bne.n	8002096 <fdBenchService+0xd2>
 8001ffa:	f8d3 0674 	ldr.w	r0, [r3, #1652]	@ 0x674
 8001ffe:	2800      	cmp	r0, #0
 8002000:	d177      	bne.n	80020f2 <fdBenchService+0x12e>
 8002002:	2000      	movs	r0, #0
 8002004:	f04f 0c01 	mov.w	ip, #1
 8002008:	f881 0474 	strb.w	r0, [r1, #1140]	@ 0x474
 800200c:	f8a1 0468 	strh.w	r0, [r1, #1128]	@ 0x468
 8002010:	f8c1 c46a 	str.w	ip, [r1, #1130]	@ 0x46a
 8002014:	f8c3 0678 	str.w	r0, [r3, #1656]	@ 0x678
 8002018:	e007      	b.n	800202a <fdBenchService+0x66>
 800201a:	f382 8810 	msr	PRIMASK, r2
 800201e:	4770      	bx	lr
 8002020:	2002      	movs	r0, #2
 8002022:	9201      	str	r2, [sp, #4]
 8002024:	f7ff ff58 	bl	8001ed8 <fdBenchStop>
 8002028:	9a01      	ldr	r2, [sp, #4]
 800202a:	f382 8810 	msr	PRIMASK, r2
 800202e:	b003      	add	sp, #12
 8002030:	bdf0      	pop	{r4, r5, r6, r7, pc}
 8002032:	f8d1 0420 	ldr.w	r0, [r1, #1056]	@ 0x420
 8002036:	f891 543b 	ldrb.w	r5, [r1, #1083]	@ 0x43b
 800203a:	1a30      	subs	r0, r6, r0
 800203c:	462f      	mov	r7, r5
 800203e:	2d00      	cmp	r5, #0
 8002040:	f000 8095 	beq.w	800216e <fdBenchService+0x1aa>
 8002044:	f8df e210 	ldr.w	lr, [pc, #528]	@ 8002258 <fdBenchService+0x294>
 8002048:	4570      	cmp	r0, lr
 800204a:	d307      	bcc.n	800205c <fdBenchService+0x98>
 800204c:	f8d1 0430 	ldr.w	r0, [r1, #1072]	@ 0x430
 8002050:	eba6 0e00 	sub.w	lr, r6, r0
 8002054:	4881      	ldr	r0, [pc, #516]	@ (800225c <fdBenchService+0x298>)
 8002056:	4586      	cmp	lr, r0
 8002058:	f080 80b2 	bcs.w	80021c0 <fdBenchService+0x1fc>
 800205c:	2000      	movs	r0, #0
 800205e:	f891 10c3 	ldrb.w	r1, [r1, #195]	@ 0xc3
 8002062:	f8b3 c006 	ldrh.w	ip, [r3, #6]
 8002066:	b139      	cbz	r1, 8002078 <fdBenchService+0xb4>
 8002068:	f9b3 1008 	ldrsh.w	r1, [r3, #8]
 800206c:	ea21 71e1 	bic.w	r1, r1, r1, asr #31
 8002070:	b289      	uxth	r1, r1
 8002072:	458c      	cmp	ip, r1
 8002074:	bf28      	it	cs
 8002076:	468c      	movcs	ip, r1
 8002078:	4560      	cmp	r0, ip
 800207a:	bf28      	it	cs
 800207c:	4660      	movcs	r0, ip
 800207e:	b280      	uxth	r0, r0
 8002080:	f8c3 0678 	str.w	r0, [r3, #1656]	@ 0x678
 8002084:	f1bc 0f00 	cmp.w	ip, #0
 8002088:	d1cf      	bne.n	800202a <fdBenchService+0x66>
 800208a:	2003      	movs	r0, #3
 800208c:	9201      	str	r2, [sp, #4]
 800208e:	f7ff ff23 	bl	8001ed8 <fdBenchStop>
 8002092:	9a01      	ldr	r2, [sp, #4]
 8002094:	e7c9      	b.n	800202a <fdBenchService+0x66>
 8002096:	f891 047d 	ldrb.w	r0, [r1, #1149]	@ 0x47d
 800209a:	2800      	cmp	r0, #0
 800209c:	d031      	beq.n	8002102 <fdBenchService+0x13e>
 800209e:	f891 00cc 	ldrb.w	r0, [r1, #204]	@ 0xcc
 80020a2:	f891 c475 	ldrb.w	ip, [r1, #1141]	@ 0x475
 80020a6:	ea50 000c 	orrs.w	r0, r0, ip
 80020aa:	d02a      	beq.n	8002102 <fdBenchService+0x13e>
 80020ac:	f891 0478 	ldrb.w	r0, [r1, #1144]	@ 0x478
 80020b0:	bb38      	cbnz	r0, 8002102 <fdBenchService+0x13e>
 80020b2:	f891 442c 	ldrb.w	r4, [r1, #1068]	@ 0x42c
 80020b6:	f8d1 0460 	ldr.w	r0, [r1, #1120]	@ 0x460
 80020ba:	2c04      	cmp	r4, #4
 80020bc:	eba6 0c00 	sub.w	ip, r6, r0
 80020c0:	d0b7      	beq.n	8002032 <fdBenchService+0x6e>
 80020c2:	f891 0410 	ldrb.w	r0, [r1, #1040]	@ 0x410
 80020c6:	bb10      	cbnz	r0, 800210e <fdBenchService+0x14a>
 80020c8:	4865      	ldr	r0, [pc, #404]	@ (8002260 <fdBenchService+0x29c>)
 80020ca:	4584      	cmp	ip, r0
 80020cc:	d322      	bcc.n	8002114 <fdBenchService+0x150>
 80020ce:	4865      	ldr	r0, [pc, #404]	@ (8002264 <fdBenchService+0x2a0>)
 80020d0:	4584      	cmp	ip, r0
 80020d2:	d208      	bcs.n	80020e6 <fdBenchService+0x122>
 80020d4:	4630      	mov	r0, r6
 80020d6:	9201      	str	r2, [sp, #4]
 80020d8:	f7ff fe50 	bl	8001d7c <fdRevRetryEligible.constprop.0>
 80020dc:	9a01      	ldr	r2, [sp, #4]
 80020de:	4b5b      	ldr	r3, [pc, #364]	@ (800224c <fdBenchService+0x288>)
 80020e0:	495c      	ldr	r1, [pc, #368]	@ (8002254 <fdBenchService+0x290>)
 80020e2:	2800      	cmp	r0, #0
 80020e4:	d15e      	bne.n	80021a4 <fdBenchService+0x1e0>
 80020e6:	2001      	movs	r0, #1
 80020e8:	9201      	str	r2, [sp, #4]
 80020ea:	f7ff fef5 	bl	8001ed8 <fdBenchStop>
 80020ee:	9a01      	ldr	r2, [sp, #4]
 80020f0:	e79b      	b.n	800202a <fdBenchService+0x66>
 80020f2:	2007      	movs	r0, #7
 80020f4:	9201      	str	r2, [sp, #4]
 80020f6:	f7ff fe8b 	bl	8001e10 <fdBenchFinish.part.0>
 80020fa:	4956      	ldr	r1, [pc, #344]	@ (8002254 <fdBenchService+0x290>)
 80020fc:	4b53      	ldr	r3, [pc, #332]	@ (800224c <fdBenchService+0x288>)
 80020fe:	9a01      	ldr	r2, [sp, #4]
 8002100:	e77f      	b.n	8002002 <fdBenchService+0x3e>
 8002102:	2004      	movs	r0, #4
 8002104:	9201      	str	r2, [sp, #4]
 8002106:	f7ff fee7 	bl	8001ed8 <fdBenchStop>
 800210a:	9a01      	ldr	r2, [sp, #4]
 800210c:	e78d      	b.n	800202a <fdBenchService+0x66>
 800210e:	4856      	ldr	r0, [pc, #344]	@ (8002268 <fdBenchService+0x2a4>)
 8002110:	4584      	cmp	ip, r0
 8002112:	d2e8      	bcs.n	80020e6 <fdBenchService+0x122>
 8002114:	2c01      	cmp	r4, #1
 8002116:	d00e      	beq.n	8002136 <fdBenchService+0x172>
 8002118:	f8df e150 	ldr.w	lr, [pc, #336]	@ 800226c <fdBenchService+0x2a8>
 800211c:	f891 043b 	ldrb.w	r0, [r1, #1083]	@ 0x43b
 8002120:	45f4      	cmp	ip, lr
 8002122:	d243      	bcs.n	80021ac <fdBenchService+0x1e8>
 8002124:	2800      	cmp	r0, #0
 8002126:	bf14      	ite	ne
 8002128:	2000      	movne	r0, #0
 800212a:	f44f 70b4 	moveq.w	r0, #360	@ 0x168
 800212e:	2c05      	cmp	r4, #5
 8002130:	bf08      	it	eq
 8002132:	2000      	moveq	r0, #0
 8002134:	e793      	b.n	800205e <fdBenchService+0x9a>
 8002136:	f891 0413 	ldrb.w	r0, [r1, #1043]	@ 0x413
 800213a:	2801      	cmp	r0, #1
 800213c:	d02f      	beq.n	800219e <fdBenchService+0x1da>
 800213e:	f8d1 0414 	ldr.w	r0, [r1, #1044]	@ 0x414
 8002142:	f8df c12c 	ldr.w	ip, [pc, #300]	@ 8002270 <fdBenchService+0x2ac>
 8002146:	1a30      	subs	r0, r6, r0
 8002148:	4560      	cmp	r0, ip
 800214a:	d828      	bhi.n	800219e <fdBenchService+0x1da>
 800214c:	f8df c124 	ldr.w	ip, [pc, #292]	@ 8002274 <fdBenchService+0x2b0>
 8002150:	fbac c000 	umull	ip, r0, ip, r0
 8002154:	09c0      	lsrs	r0, r0, #7
 8002156:	f04f 0c96 	mov.w	ip, #150	@ 0x96
 800215a:	fb0c f000 	mul.w	r0, ip, r0
 800215e:	f8df c118 	ldr.w	ip, [pc, #280]	@ 8002278 <fdBenchService+0x2b4>
 8002162:	fbac c000 	umull	ip, r0, ip, r0
 8002166:	0b40      	lsrs	r0, r0, #13
 8002168:	30d2      	adds	r0, #210	@ 0xd2
 800216a:	b280      	uxth	r0, r0
 800216c:	e777      	b.n	800205e <fdBenchService+0x9a>
 800216e:	f8d1 5430 	ldr.w	r5, [r1, #1072]	@ 0x430
 8002172:	4c42      	ldr	r4, [pc, #264]	@ (800227c <fdBenchService+0x2b8>)
 8002174:	eba6 0e05 	sub.w	lr, r6, r5
 8002178:	45a6      	cmp	lr, r4
 800217a:	d21d      	bcs.n	80021b8 <fdBenchService+0x1f4>
 800217c:	4c36      	ldr	r4, [pc, #216]	@ (8002258 <fdBenchService+0x294>)
 800217e:	42a0      	cmp	r0, r4
 8002180:	d351      	bcc.n	8002226 <fdBenchService+0x262>
 8002182:	4c36      	ldr	r4, [pc, #216]	@ (800225c <fdBenchService+0x298>)
 8002184:	8818      	ldrh	r0, [r3, #0]
 8002186:	45a6      	cmp	lr, r4
 8002188:	d21b      	bcs.n	80021c2 <fdBenchService+0x1fe>
 800218a:	f8df e0e0 	ldr.w	lr, [pc, #224]	@ 800226c <fdBenchService+0x2a8>
 800218e:	45f4      	cmp	ip, lr
 8002190:	bf38      	it	cc
 8002192:	f44f 70b4 	movcc.w	r0, #360	@ 0x168
 8002196:	e762      	b.n	800205e <fdBenchService+0x9a>
 8002198:	2f00      	cmp	r7, #0
 800219a:	f47f af5f 	bne.w	800205c <fdBenchService+0x98>
 800219e:	f44f 70b4 	mov.w	r0, #360	@ 0x168
 80021a2:	e75c      	b.n	800205e <fdBenchService+0x9a>
 80021a4:	2c01      	cmp	r4, #1
 80021a6:	d0c6      	beq.n	8002136 <fdBenchService+0x172>
 80021a8:	f891 043b 	ldrb.w	r0, [r1, #1083]	@ 0x43b
 80021ac:	2800      	cmp	r0, #0
 80021ae:	bf14      	ite	ne
 80021b0:	2000      	movne	r0, #0
 80021b2:	f44f 70c8 	moveq.w	r0, #400	@ 0x190
 80021b6:	e7ba      	b.n	800212e <fdBenchService+0x16a>
 80021b8:	2001      	movs	r0, #1
 80021ba:	f881 043b 	strb.w	r0, [r1, #1083]	@ 0x43b
 80021be:	e74d      	b.n	800205c <fdBenchService+0x98>
 80021c0:	8818      	ldrh	r0, [r3, #0]
 80021c2:	4c2f      	ldr	r4, [pc, #188]	@ (8002280 <fdBenchService+0x2bc>)
 80021c4:	f1ae 7eb7 	sub.w	lr, lr, #23986176	@ 0x16e0000
 80021c8:	f5ae 5e58 	sub.w	lr, lr, #13824	@ 0x3600
 80021cc:	45a6      	cmp	lr, r4
 80021ce:	d221      	bcs.n	8002214 <fdBenchService+0x250>
 80021d0:	885d      	ldrh	r5, [r3, #2]
 80021d2:	4285      	cmp	r5, r0
 80021d4:	d216      	bcs.n	8002204 <fdBenchService+0x240>
 80021d6:	4c25      	ldr	r4, [pc, #148]	@ (800226c <fdBenchService+0x2a8>)
 80021d8:	45a4      	cmp	ip, r4
 80021da:	d3dd      	bcc.n	8002198 <fdBenchService+0x1d4>
 80021dc:	2f00      	cmp	r7, #0
 80021de:	f47f af3d 	bne.w	800205c <fdBenchService+0x98>
 80021e2:	f1ce 7c37 	rsb	ip, lr, #47972352	@ 0x2dc0000
 80021e6:	eba0 0e05 	sub.w	lr, r0, r5
 80021ea:	4c26      	ldr	r4, [pc, #152]	@ (8002284 <fdBenchService+0x2c0>)
 80021ec:	f50c 4cd8 	add.w	ip, ip, #27648	@ 0x6c00
 80021f0:	20a0      	movs	r0, #160	@ 0xa0
 80021f2:	fbbc f0f0 	udiv	r0, ip, r0
 80021f6:	fb0e f000 	mul.w	r0, lr, r0
 80021fa:	fbb0 f0f4 	udiv	r0, r0, r4
 80021fe:	4428      	add	r0, r5
 8002200:	b280      	uxth	r0, r0
 8002202:	e72c      	b.n	800205e <fdBenchService+0x9a>
 8002204:	f8df e064 	ldr.w	lr, [pc, #100]	@ 800226c <fdBenchService+0x2a8>
 8002208:	45f4      	cmp	ip, lr
 800220a:	d3c5      	bcc.n	8002198 <fdBenchService+0x1d4>
 800220c:	2f00      	cmp	r7, #0
 800220e:	f47f af25 	bne.w	800205c <fdBenchService+0x98>
 8002212:	e724      	b.n	800205e <fdBenchService+0x9a>
 8002214:	4815      	ldr	r0, [pc, #84]	@ (800226c <fdBenchService+0x2a8>)
 8002216:	4584      	cmp	ip, r0
 8002218:	f4bf af20 	bcs.w	800205c <fdBenchService+0x98>
 800221c:	f891 043b 	ldrb.w	r0, [r1, #1083]	@ 0x43b
 8002220:	2800      	cmp	r0, #0
 8002222:	d0bc      	beq.n	800219e <fdBenchService+0x1da>
 8002224:	e71a      	b.n	800205c <fdBenchService+0x98>
 8002226:	f8df e044 	ldr.w	lr, [pc, #68]	@ 800226c <fdBenchService+0x2a8>
 800222a:	45f4      	cmp	ip, lr
 800222c:	d3b7      	bcc.n	800219e <fdBenchService+0x1da>
 800222e:	f06f 0c27 	mvn.w	ip, #39	@ 0x27
 8002232:	fb0c f000 	mul.w	r0, ip, r0
 8002236:	f100 60f4 	add.w	r0, r0, #127926272	@ 0x7a00000
 800223a:	4c07      	ldr	r4, [pc, #28]	@ (8002258 <fdBenchService+0x294>)
 800223c:	f500 3090 	add.w	r0, r0, #73728	@ 0x12000
 8002240:	fbb0 f0f4 	udiv	r0, r0, r4
 8002244:	f500 70b4 	add.w	r0, r0, #360	@ 0x168
 8002248:	b280      	uxth	r0, r0
 800224a:	e708      	b.n	800205e <fdBenchService+0x9a>
 800224c:	20000000 	.word	0x20000000
 8002250:	e0001000 	.word	0xe0001000
 8002254:	20001184 	.word	0x20001184
 8002258:	0030d400 	.word	0x0030d400
 800225c:	016e3600 	.word	0x016e3600
 8002260:	03b82600 	.word	0x03b82600
 8002264:	03e8fa00 	.word	0x03e8fa00
 8002268:	07d1f400 	.word	0x07d1f400
 800226c:	00493e00 	.word	0x00493e00
 8002270:	00186a9f 	.word	0x00186a9f
 8002274:	cccccccd 	.word	0xcccccccd
 8002278:	d1b71759 	.word	0xd1b71759
 800227c:	09896800 	.word	0x09896800
 8002280:	02dc6c00 	.word	0x02dc6c00
 8002284:	000493e0 	.word	0x000493e0
