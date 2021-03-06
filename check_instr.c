#include <check.h>

#include "emu.h"
#include "test.h"

#define	PC_START		0

/* Make it harder to forget to add tests to suite. */
#pragma GCC diagnostic error "-Wunused-function"

static void
install_words(uint16_t *code, uint32_t addr, size_t sz)
{

	memcpy(&flash[addr], code, sz);
}

static void
setup_machine(void)
{

	// zero regs/mem, clear symbols
	init();
}

static void
setup_machine22(void)
{

	// zero regs/mem, clear symbols
	init();
	pc22 = true;
}

static void
setup_machine64(void)
{

	// zero regs/mem, clear symbols
	init();
	pc_mem_max_64k = true;
}

static void
setup_machine256(void)
{

	// zero regs/mem, clear symbols
	init();
	pc_mem_max_64k = true;
	pc_mem_max_256b = true;
}

static void
teardown_machine(void)
{

	destroy();
}

START_TEST(test_nop)
{
	uint16_t code[] = {
		0x0000,
		0x0000,
	};

	install_words(code, PC_START, sizeof(code));
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
}
END_TEST

START_TEST(test_mov)
{
	uint16_t code[] = {
		0x2c00 | /*r_rrrr*/ 0x20f | /*ddddd*/ 0x1e0,	/* mov r30,r31 */
		0x2c00 | /*r_rrrr*/ 0x20e | /*ddddd*/ 0x050,	/* mov r5,r30 */
		0x2c00 | /*r_rrrr*/ 0x005 | /*ddddd*/ 0x000,	/* mov r0,r5 */
	};

	install_words(code, PC_START, sizeof(code));
	memory[31] = 0xab;
	ck_assert_uint_eq(memory[30], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[30], 0xab);
	ck_assert_uint_eq(memory[5], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[5], 0xab);
	ck_assert_uint_eq(memory[0], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[0], 0xab);
}
END_TEST

START_TEST(test_or)
{
	uint16_t code[] = {
		0x2800 | /*r_rrrr*/ 0x20f | /*ddddd*/ 0x1e0,	/* or r30,r31 */
		0x2800 | /*r_rrrr*/ 0x20d | /*ddddd*/ 0x050,	/* or r5,r29 */
		0x2800 | /*r_rrrr*/ 0x005 | /*ddddd*/ 0x000,	/* or r0,r5 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x0;
	memory[30] = 0x0;

	memory[29] = 0xaa;
	memory[5] = 0x55;

	memory[0] = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	// OR(0, 0) = 0
	ck_assert_uint_eq(memory[30], 0x0);
	ck_assert_uint_eq(memory[SREG], SREG_Z);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	// OR(0xaa, 0x55) = 0xff
	ck_assert_uint_eq(memory[5], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	// OR(0xff, 0xff) = 0xff
	ck_assert_uint_eq(memory[0], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);
}
END_TEST

START_TEST(test_ori)
{
	uint16_t code[] = {
		0x6000 | /*KKKK_KKKK*/ 0xf0f | /*dddd*/ 0x00,	/* ori r16, $0xff */
		0x6000 | /*KKKK_KKKK*/ 0x000 | /*dddd*/ 0x50,	/* ori r21, $0x0 */
		0x6000 | /*KKKK_KKKK*/ 0xa0a | /*dddd*/ 0xf0,	/* ori r31, $0xaa */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x55;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	// OR(0, 0xff) = 0xff
	ck_assert_uint_eq(memory[16], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	// OR(0, 0) = 0
	ck_assert_uint_eq(memory[21], 0);
	ck_assert_uint_eq(memory[SREG], SREG_Z);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	// OR(0xaa, 0x55) = 0xff
	ck_assert_uint_eq(memory[31], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);
}
END_TEST

START_TEST(test_out)
{
	uint16_t code[] = {
		0xb800 | /*AA_AAAA*/ 0x60e | /*rrrrr*/ 0x1f0,	/* out $3e, r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[IO_BASE + 0x3e], 0xab);
}
END_TEST

START_TEST(test_in)
{
	uint16_t code[] = {
		0xb000 | /*AA_AAAA*/ 0x60e | /*rrrrr*/ 0x1f0,	/* in r31, $3e */
	};

	install_words(code, PC_START, sizeof(code));

	memory[IO_BASE + 0x3e] = 0xab;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0xab);
}
END_TEST

START_TEST(test_push)
{
	uint16_t code[] = {
		0x920f | /*rrrrr*/ 0x1f0,	/* push r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;

	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(getsp(), 0xfffe);
	ck_assert_uint_eq(memory[0xffff], 0xab);
}
END_TEST

START_TEST(test_movw)
{
	uint16_t code[] = {
		0x0100 | /*dddd*/ 0xe0 | /*rrrr*/ 0xf,	/* mov r29:28, r31:r30 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;
	memory[30] = 0xcd;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[29], memory[31]);
	ck_assert_uint_eq(memory[28], memory[30]);
}
END_TEST

START_TEST(test_call)
{
	uint16_t code[] = {
		0x940e | /*kkkkk_k*/ 0x1f1,	/* call 0x3fdead */
		0xdead
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0xdead);
	ck_assert_uint_eq(getsp(), 0xfffd);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], (PC_START + 2) >> 8);
}
END_TEST

START_TEST(test_call22)
{
	uint16_t code[] = {
		0x940e | /*kkkkk_k*/ 0x1f1,	/* call 0x3fbeef */
		0xbeef
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0x3fbeef);
	ck_assert_uint_eq(getsp(), 0xfffc);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], ((PC_START + 2) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffd], (PC_START + 2) >> 16);
}
END_TEST

START_TEST(test_mul)
{
	uint16_t code[] = {
		0x9c00 | 0x3fe,		/* mul r31,r30 */
		0x9c00 | 0x001,		/* mul r0, r1 */
		0x9c00 | 0x045,		/* mul r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xff;
	memory[30] = 0xff;
	memory[4] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xff * 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0xfe * 0x01);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_muls)
{
	uint16_t code[] = {
		0x0200 | 0xfe,		/* muls r31,r30 */
		0x0200 | 0x45,		/* muls r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xff;
	memory[30] = 0x2;
	memory[20] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xfffe);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_mulsu)
{
	uint16_t code[] = {
		0x0300 | 0x76,		/* mulsu r23, r22 */
		0x0300 | 0x45,		/* mulsu r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[23] = 0xff;
	memory[22] = 0xff;
	memory[20] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xff01);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_adc)
{
	uint16_t code[] = {
		0x1c00 | 0x01,		/* adc r0, r1 */
		0x1c00 | 0x23,		/* adc r2, r3 */
		0x1c00 | 0x45,		/* adc r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[0] = 0xf0;
	memory[1] = 0x0f;
	sreg_start = SREG_I | SREG_T;
	memory[SREG] = sreg_start | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z | SREG_H);

	memory[2] = 0x40;
	memory[3] = 0x40;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[2], 0x81 /* Carry from previous instr */);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[4], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);
}
END_TEST

START_TEST(test_add)
{
	uint16_t code[] = {
		0x0c00 | 0x01,		/* add r0, r1 */
		0x0c00 | 0x23,		/* add r2, r3 */
		0x0c00 | 0x45,		/* add r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[0] = 0xf1;
	memory[1] = 0x0f;
	memory[SREG] = sreg_start = SREG_I | SREG_T;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z | SREG_H);

	memory[2] = 0x40;
	memory[3] = 0x40;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[2], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[4], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);
}
END_TEST

START_TEST(test_adiw)
{
	uint16_t code[] = {
		0x9600 | 0x00 | 0x5,		/* addiw r25:24, $5 */
		0x9600 | 0x30 | 0xcf,		/* addiw r31:30, $3f */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[24] = 0xfb;
	memory[25] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[24], 0);
	ck_assert_uint_eq(memory[25], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z);

	memory[30] = 0xff;
	memory[31] = 0x7f;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[30], 0x3e);
	ck_assert_uint_eq(memory[31], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);
}
END_TEST

START_TEST(test_and)
{
	uint16_t code[] = {
		0x2000 | 0x01,		/* and r0,r1 */
		0x2000 | 0x45,		/* and r4,r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[1] = 0xff;
	memory[0] = 0xff;

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[4], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_andi)
{
	uint16_t code[] = {
		0x7000 | 0x00 | 0xf0f,	/* andi r16, $ff */
		0x7000 | 0x40 | 0,	/* andi r20, $0 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));
	memory[16] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[16], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	memory[20] = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[20], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_asr)
{
	uint16_t code[] = {
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));
	memory[31] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_C | SREG_S);

	memory[31] = 0x81;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[31], 0xc0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_C | SREG_S);

	memory[31] = 0x7f;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[31], 0x3f);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_V | SREG_S);

	memory[31] = 0x70;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[31], 0x38);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	memory[31] = 0x1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 5);
	ck_assert_uint_eq(memory[31], 0);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_C | SREG_Z | SREG_V | SREG_S);
}
END_TEST

START_TEST(test_bclr)
{
	uint16_t code[] = {
		0x9488 | 0x70,	/* bclr I ("CLI" psuedo) */
		0x9488 | 0x10,	/* bclr Z */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));
	memory[SREG] = sreg_start = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[SREG], sreg_start & ~SREG_I);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[SREG], sreg_start & ~(SREG_I | SREG_Z));
}
END_TEST

START_TEST(test_bset)
{
	uint16_t code[] = {
		0x9408 | 0x60,	/* bset T */
		0x9408 | 0x30,	/* bset V */
	};

	install_words(code, PC_START, sizeof(code));
	memory[SREG] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[SREG], SREG_T);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[SREG], SREG_T | SREG_V);
}
END_TEST

START_TEST(test_bld)
{
	uint16_t code[] = {
		0xf800 | 0x1f0 | 0x4,	/* bld r31, 4 */
		0xf800 | 0x1f0 | 0x4,	/* bld r31, 4 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = SREG_T;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 1 << 4);

	memory[SREG] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[31], 0);
}
END_TEST

START_TEST(test_bst)
{
	uint16_t code[] = {
		0xfa00 | 0x1f0 | 0x4,	/* bst r31, 4 */
		0xfa00 | 0x1f0 | 0x4,	/* bst r31, 4 */
	};

	install_words(code, PC_START, sizeof(code));
	memory[SREG] = 0;

	memory[31] = (1 << 4);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[SREG], SREG_T);

	memory[31] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[SREG], 0);
}
END_TEST

START_TEST(test_brb)
{
	uint16_t code[] = {
		0xf400 | 0x10 /*k=2*/ | 0x7 /*SREG_I*/,		/* brbc I, 2 */
		0 /* skipped */,
		0xf000 | 0x1f8 /*k=63*/ | 0x0 /*SREG_C*/,	/* brbs C, 63 */
		0xf000 | 0x3f0 /*k=-2*/ | 0x6 /*SREG_T*/,	/* brbs T, -2 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = SREG_T | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2 + 63 + 1);
}
END_TEST

START_TEST(test_cbisbi)
{
	uint16_t code[] = {
		0x9800 | 0xa8 /*A=0x15*/ | 0x3 /*bit 3*/,		/* cbi $0x15, 3 */
		0x9a00 | 0xa8 /*A=0x15*/ | 0x3 /*bit 3*/,		/* cbi $0x15, 3 */
	};

	install_words(code, PC_START, sizeof(code));
	memory[IO_BASE + 0x15] = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[IO_BASE + 0x15], 0xff & ~(1 << 3));

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[IO_BASE + 0x15], 0xff);
}
END_TEST

START_TEST(test_com)
{
	uint16_t code[] = {
		0x9400 | 0x1f0 /*d=31*/,		/* com r31 */
		0x9400 | 0x1f0 /*d=31*/,		/* com r31 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_V;
	memory[31] = 0x55;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0xaa);
	ck_assert_uint_eq(memory[SREG],
	    (sreg_start | SREG_C | SREG_N | SREG_S) & ~SREG_V);

	memory[31] = 0xff;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[31], 0);
	ck_assert_uint_eq(memory[SREG],
	    (sreg_start | SREG_C | SREG_Z) & ~SREG_V);
}
END_TEST

START_TEST(test_cpc)
{
	uint16_t code[] = {
		0x1400 | 0x01,		/* cp r0, r1 */
		0x0400 | 0x23,		/* cpc r2, r3 */
		0x1400 | 0x45,		/* cp r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[0] = 0xf1;
	memory[1] = 0x0f;
	memory[SREG] = sreg_start = SREG_I | SREG_T;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0xf1);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_H | SREG_N | SREG_S);

	memory[2] = 0x80;
	memory[3] = 0x7f;
	memory[SREG] |= SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[2], 0x80);
	/* CP, CPC does not set Z. */
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_V | SREG_S | SREG_H);

	memory[4] = 0x0;
	memory[5] = 0xff;
	memory[SREG] |= SREG_Z;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[4], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_H | SREG_C);
}
END_TEST

START_TEST(test_cpi)
{
	uint16_t code[] = {
		0x3000 | 0x70f | 0x10,		/* cpi r17, $7f */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[17] = 0x80;
	memory[SREG] = sreg_start = SREG_I | SREG_T;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[17], 0x80);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_V | SREG_S | SREG_H);
}
END_TEST

START_TEST(test_cpse)
{
	uint16_t code[] = {
		0x1000 | 0x01,		/* cpse r0, r1 */
		0x1000 | 0x02,		/* cpse r0, r2 */
		0x940e | /*kkkkk_k*/ 0x1f1,	/* call 0x3fbeef */
		0xbeef,
		0x1000 | 0x02,		/* cpse r0, r2 */
		0,			/* nop */
		0,			/* nop */
	};

	install_words(code, PC_START, sizeof(code));

	memory[1] = 1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(insns, 1);

	emulate1();
	/* Skips both words of 32-bit CALL imm16 */
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(insns, 2);

	emulate1();
	/* Skips only one word of NOP */
	ck_assert_uint_eq(pc, PC_START + 6);
	ck_assert_uint_eq(insns, 3);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 7);
	ck_assert_uint_eq(insns, 4);
}
END_TEST

START_TEST(test_dec)
{
	uint16_t code[] = {
		0x940a | 0x00,		/* dec r0 */
		0x940a | 0x00,		/* dec r0 */
		0x940a | 0x00,		/* dec r0 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;
	memory[0] = 0x81;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0], 0x7f);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_S);

	memory[0] = 0x1;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_eicall22)
{
	uint16_t code[] = {
		0x9509,		/* icall */
		0x9519,		/* eicall */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	memory[EIND] = 0x3f;
	memwriteword(REGP_Z, PC_START + 1);

	emulate1();
	ck_assert_uint_eq(getsp(), 0xfffc);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 1) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], ((PC_START + 1) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffd], (PC_START + 1) >> 16);
	ck_assert_uint_eq(pc, PC_START + 1);

	memory[EIND] = 0x5;
	memwriteword(REGP_Z, 0);
	emulate1();
	ck_assert_uint_eq(getsp(), 0xfff9);
	ck_assert_uint_eq(memory[0xfffc], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffb], ((PC_START + 2) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffa], (PC_START + 2) >> 16);
	ck_assert_uint_eq(pc, 0x50000);
}
END_TEST

START_TEST(test_icall)
{
	uint16_t code[] = {
		0x9509,		/* icall */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);
	memwriteword(REGP_Z, 0xffee);

	emulate1();
	ck_assert_uint_eq(getsp(), 0xfffd);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 1) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], (PC_START + 1) >> 8);
	ck_assert_uint_eq(pc, 0xffee);
}
END_TEST

START_TEST(test_eijump22)
{
	uint16_t code[] = {
		0x9409,		/* ijump */
		0x9419,		/* eijump */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	memory[EIND] = 0x3f;
	memwriteword(REGP_Z, PC_START + 1);
	emulate1();
	ck_assert_uint_eq(getsp(), 0xffff);
	ck_assert_uint_eq(pc, PC_START + 1);

	memory[EIND] = 0x5;
	memwriteword(REGP_Z, 0x1234);
	emulate1();
	ck_assert_uint_eq(getsp(), 0xffff);
	ck_assert_uint_eq(pc, 0x51234);
}
END_TEST

START_TEST(test_ijump)
{
	uint16_t code[] = {
		0x9409,		/* ijump */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);
	memwriteword(REGP_Z, 0xffee);

	emulate1();
	ck_assert_uint_eq(getsp(), 0xffff);
	ck_assert_uint_eq(pc, 0xffee);
}
END_TEST

START_TEST(test_elpm)
{
	uint16_t code[] = {
		0x95c8,		/* lpm */
		0x95d8,		/* elpm */
		/* Z is r31:r30, so don't trash it: */
		0x9004 | 0x1d0,		/* lpm r29, Z */
		0x9006 | 0x1d0,		/* elpm r29, Z */
		0x9005 | 0x1d0,		/* lpm r29, Z+ */
		0x9007 | 0x1d0,		/* elpm r29, Z+ */
	};

	install_words(code, PC_START, sizeof(code));
	memwriteword(REGP_Z, 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0xc8);
	ck_assert_uint_eq(memword(REGP_Z), 0);

	memory[RAMPZ] = 1;
	install_words(&code[1], 0x10000 / 2, sizeof(code[1]));

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0], 0xd8);

	memwriteword(REGP_Z, 4);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[29], 0xd4);
	ck_assert_uint_eq(memword(REGP_Z), 4);

	memory[RAMPZ] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[29], 0xd4);

	memory[RAMPZ] = 0x3f;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 5);
	ck_assert_uint_eq(memory[29], 0xd4);
	ck_assert_uint_eq(memword(REGP_Z), 0x5);

	memory[RAMPZ] = 0x3e;
	memwriteword(REGP_Z, 0xffff);
	memset(&flash[0x3efffe / 2], 0xb5, sizeof(flash[0]));
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 6);
	ck_assert_uint_eq(memory[29], 0xb5);
	ck_assert_uint_eq(memword(REGP_Z), 0);
	ck_assert_uint_eq(memory[RAMPZ], 0x3e);

	memset(&flash[0x3efffe / 2], 0, sizeof(flash[0]));
}
END_TEST

START_TEST(test_xor)
{
	uint16_t code[] = {
		0x2400 | /*r_rrrr*/ 0x20f | /*ddddd*/ 0x1e0,	/* xor r30,r31 */
		0x2400 | /*r_rrrr*/ 0x20d | /*ddddd*/ 0x050,	/* xor r5,r29 */
		0x2400 | /*r_rrrr*/ 0x005 | /*ddddd*/ 0x000,	/* xor r0,r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x0;
	memory[30] = 0x0;

	memory[29] = 0xaa;
	memory[5] = 0x55;

	memory[0] = 0xff;

	sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;
	memory[SREG] = sreg_start | SREG_V;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[30], 0x0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[5], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_fmul)
{
	uint16_t code[] = {
		0x0308 | 0x76,		/* fmul r23, r22 */
		0x0308 | 0x01,		/* fmul r16, r17 */
		0x0308 | 0x45,		/* fmul r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[23] = 0xff;
	memory[22] = 0xff;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), ((0xff * 0xff) << 1) & 0xffff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	memory[16] = 0x80;
	memory[17] = 0x80;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0x8000);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	memory[20] = 0x15;
	memory[21] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_fmuls)
{
	uint16_t code[] = {
		0x0380 | 0x76,		/* fmuls r23,r22 */
		0x0380 | 0x45,		/* fmuls r20, r21 */
		0x0380 | 0x45,		/* fmuls r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[23] = 0xff;
	memory[22] = 0x2;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xfffc);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	memory[20] = 0x80;
	memory[21] = 0x80;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0x8000);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	memory[21] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_fmulsu)
{
	uint16_t code[] = {
		0x0388 | 0x76,		/* fmulsu r23, r22 */
		0x0388 | 0x45,		/* fmulsu r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[23] = 0xff;
	memory[22] = 0xff;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xfe02);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	memory[20] = 0x15;
	memory[21] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_inc)
{
	uint16_t code[] = {
		0x9403 | 0x00,		/* inc r0 */
		0x9403 | 0x00,		/* inc r0 */
		0x9403 | 0x00,		/* inc r0 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;
	memory[0] = 0x7f;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_V);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0], 0x81);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	memory[0] = 0xff;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_jmp)
{
	uint16_t code[] = {
		0x940c | /*kkkkk_k*/ 0x1f1,	/* jmp 0x3fdead */
		0xdead
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0xdead);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_jmp22)
{
	uint16_t code[] = {
		0x940c | /*kkkkk_k*/ 0x1f1,	/* jmp 0x3fbeef */
		0xbeef
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0x3fbeef);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_ldx)
{
	uint16_t code[] = {
		0x900c | 0x50,	/* ld r5, X */
		0x900d | 0x50,	/* ld r5, X+ */
		0x900e | 0x50,	/* ld r5, -X */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_X, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_X), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPX] = 0x3f;
	memwriteword(REGP_X, 0);
	memset(&memory[0x3effff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_X), 0xffff);
	ck_assert_uint_eq(memory[RAMPX], 0x3e);
	ck_assert_uint_eq(memory[5], 0x55);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
}
END_TEST

START_TEST(test_ldx64)
{
	uint16_t code[] = {
		0x900c | 0x50,	/* ld r5, X */
		0x900d | 0x50,	/* ld r5, X+ */
		0x900e | 0x50,	/* ld r5, -X */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_X, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_X), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPX] = 0x3f;
	memwriteword(REGP_X, 0);
	memset(&memory[0xffff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_X), 0xffff);
	ck_assert_uint_eq(memory[RAMPX], 0x3f);
	ck_assert_uint_eq(memory[5], 0x55);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
	memset(&memory[RAMPX], 0, 1);
}
END_TEST

START_TEST(test_ldx256)
{
	uint16_t code[] = {
		0x900c | 0x50,	/* ld r5, X */
		0x900d | 0x50,	/* ld r5, X+ */
		0x900e | 0x50,	/* ld r5, -X */
	};

	install_words(code, PC_START, sizeof(code));

	memset(memory, 0xaa, 1);

	memwriteword(REGP_X, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	memwriteword(REGP_X, 0x1ff);
	memset(&memory[0xff], 0xbb, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	/* Only XLO is updated */
	ck_assert_uint_eq(memword(REGP_X), 0x100);
	ck_assert_uint_eq(memory[5], 0xbb);

	memory[RAMPX] = 0x3f;
	memwriteword(REGP_X, 0);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_X), 0x00ff);
	ck_assert_uint_eq(memory[RAMPX], 0x3f);
	ck_assert_uint_eq(memory[5], 0xbb);

	memset(&memory[0xff], 0, 1);
	memset(&memory[RAMPX], 0, 1);
}
END_TEST

START_TEST(test_ldy)
{
	uint16_t code[] = {
		0x8008 | 0x50,	/* ld r5, Y */
		0x9009 | 0x50,	/* ld r5, Y+ */
		0x900a | 0x50,	/* ld r5, -Y */
		0x8008 | 0x457,	/* ldd r5, Y+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_Y, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Y), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Y), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPY] = 0x3f;
	memwriteword(REGP_Y, 0);
	memset(&memory[0x3effff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Y), 0xffff);
	ck_assert_uint_eq(memory[RAMPY], 0x3e);
	ck_assert_uint_eq(memory[5], 0x55);

	memory[RAMPY] = 0;
	memwriteword(REGP_Y, 0xfff1);
	memset(&memory[0x10000], 0xba, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Y), 0xfff1);
	ck_assert_uint_eq(memory[RAMPY], 0);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
}
END_TEST

START_TEST(test_ldy64)
{
	uint16_t code[] = {
		0x8008 | 0x50,	/* ld r5, Y */
		0x9009 | 0x50,	/* ld r5, Y+ */
		0x900a | 0x50,	/* ld r5, -Y */
		0x8008 | 0x457,	/* ldd r5, Y+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_Y, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Y), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Y), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPY] = 0x3f;
	memwriteword(REGP_Y, 0);
	memset(&memory[0xffff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Y), 0xffff);
	ck_assert_uint_eq(memory[RAMPY], 0x3f);
	ck_assert_uint_eq(memory[5], 0x55);

	memory[RAMPY] = 0;
	memwriteword(REGP_Y, 0xfff1);
	memset(memory, 0xba, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Y), 0xfff1);
	ck_assert_uint_eq(memory[RAMPY], 0);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
	memset(&memory[RAMPY], 0, 1);
}
END_TEST

START_TEST(test_ldy256)
{
	uint16_t code[] = {
		0x8008 | 0x50,	/* ld r5, Y */
		0x9009 | 0x50,	/* ld r5, Y+ */
		0x900a | 0x50,	/* ld r5, -Y */
		0x8008 | 0x457,	/* ldd r5, Y+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(memory, 0xaa, 1);

	memwriteword(REGP_Y, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Y), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	memwriteword(REGP_Y, 0x1ff);
	memset(&memory[0xff], 0xbb, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	/* Only YLO is updated */
	ck_assert_uint_eq(memword(REGP_Y), 0x100);
	ck_assert_uint_eq(memory[5], 0xbb);

	memory[RAMPY] = 0x3f;
	memwriteword(REGP_Y, 0);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Y), 0x00ff);
	ck_assert_uint_eq(memory[RAMPY], 0x3f);
	ck_assert_uint_eq(memory[5], 0xbb);

	memwriteword(REGP_Y, 0x5f1);
	memset(memory, 0xba, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Y), 0x5f1);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0xff], 0, 1);
	memset(&memory[RAMPY], 0, 1);
}
END_TEST

START_TEST(test_ldz)
{
	uint16_t code[] = {
		0x8000 | 0x50,	/* ld r5, Z */
		0x9001 | 0x50,	/* ld r5, Z+ */
		0x9002 | 0x50,	/* ld r5, -Z */
		0x8000 | 0x457,	/* ldd r5, Z+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_Z, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Z), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Z), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPZ] = 0x3f;
	memwriteword(REGP_Z, 0);
	memset(&memory[0x3effff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Z), 0xffff);
	ck_assert_uint_eq(memory[RAMPZ], 0x3e);
	ck_assert_uint_eq(memory[5], 0x55);

	memory[RAMPZ] = 0;
	memwriteword(REGP_Z, 0xfff1);
	memory[0x10000] = 0xba;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Z), 0xfff1);
	ck_assert_uint_eq(memory[RAMPZ], 0);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
}
END_TEST

START_TEST(test_ldz64)
{
	uint16_t code[] = {
		0x8000 | 0x50,	/* ld r5, Z */
		0x9001 | 0x50,	/* ld r5, Z+ */
		0x9002 | 0x50,	/* ld r5, -Z */
		0x8000 | 0x457,	/* ldd r5, Z+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(&memory[0x100], 0xaa, 1);

	memwriteword(REGP_Z, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Z), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Z), 0x101);
	ck_assert_uint_eq(memory[5], 0xaa);

	memory[RAMPZ] = 0x3f;
	memwriteword(REGP_Z, 0);
	memset(&memory[0xffff], 0x55, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Z), 0xffff);
	ck_assert_uint_eq(memory[RAMPZ], 0x3f);
	ck_assert_uint_eq(memory[5], 0x55);

	memory[RAMPZ] = 0;
	memwriteword(REGP_Z, 0xfff1);
	memory[0] = 0xba;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Z), 0xfff1);
	ck_assert_uint_eq(memory[RAMPZ], 0);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0x3effff], 0, 1);
	memset(&memory[0x100], 0, 1);
	memset(&memory[RAMPZ], 0, 1);
}
END_TEST

START_TEST(test_ldz256)
{
	uint16_t code[] = {
		0x8000 | 0x50,	/* ld r5, Z */
		0x9001 | 0x50,	/* ld r5, Z+ */
		0x9002 | 0x50,	/* ld r5, -Z */
		0x8000 | 0x457,	/* ldd r5, Z+$f */
	};

	install_words(code, PC_START, sizeof(code));

	memset(memory, 0xaa, 1);

	memwriteword(REGP_Z, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Z), 0x100);
	ck_assert_uint_eq(memory[5], 0xaa);

	memwriteword(REGP_Z, 0x1ff);
	memset(&memory[0xff], 0xbb, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	/* Only ZLO is updated */
	ck_assert_uint_eq(memword(REGP_Z), 0x100);
	ck_assert_uint_eq(memory[5], 0xbb);

	memory[RAMPZ] = 0x3f;
	memwriteword(REGP_Z, 0);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Z), 0x00ff);
	ck_assert_uint_eq(memory[RAMPZ], 0x3f);
	ck_assert_uint_eq(memory[5], 0xbb);

	memory[RAMPZ] = 0;
	memwriteword(REGP_Z, 0x3ff1);
	memory[0] = 0xba;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Z), 0x3ff1);
	ck_assert_uint_eq(memory[RAMPZ], 0);
	ck_assert_uint_eq(memory[5], 0xba);

	memset(&memory[0xff], 0, 1);
	memset(&memory[RAMPZ], 0, 1);
}
END_TEST

START_TEST(test_ldi)
{
	uint16_t code[] = {
		0xe000 | /*KKKK_KKKK*/ 0xf0f | /*dddd*/ 0x10,	/* ldi r17, $0xff */
	};

	install_words(code, PC_START, sizeof(code));

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[17], 0xff);
}
END_TEST

START_TEST(test_lds)
{
	uint16_t code[] = {
		0x9000 | /*dddd*/ 0x10,	/* lds r1, $0xbeef */
		0xbeef,
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPD] = 0x1;
	memset(&memory[0x1beef], 0xc0, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[1], 0xc0);
	memset(&memory[0x1beef], 0, 1);
}
END_TEST

START_TEST(test_lds64)
{
	uint16_t code[] = {
		0x9000 | /*dddd*/ 0x10,	/* lds r1, $0xbeef */
		0xbeef,
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPD] = 0x1;
	memset(&memory[0xbeef], 0xc0, 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[1], 0xc0);
	memset(&memory[0xbeef], 0, 1);
}
END_TEST

START_TEST(test_lsr)
{
	uint16_t code[] = {
		0x9406 | /*dddd*/ 0x10,		/* lsr r1 */
		0x9406 | /*dddd*/ 0x10,		/* lsr r1 */
		0x9406 | /*dddd*/ 0x10,		/* lsr r1 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	sreg_start = SREG_I | SREG_T | SREG_H;
	memory[SREG] = sreg_start | SREG_N;

	memory[1] = 0xff;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[1], 0x7f);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_V | SREG_S);

	memory[1] = 0x1;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[1], 0);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_C | SREG_Z | SREG_V | SREG_S);

	memory[1] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[1], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_neg)
{
	uint16_t code[] = {
		0x9401 | 0x1f0 /*d=31*/,		/* neg r31 */
		0x9401 | 0x1f0 /*d=31*/,		/* neg r31 */
		0x9401 | 0x1f0 /*d=31*/,		/* neg r31 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T;
	memory[31] = 0x55;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], (0u - 0x55) & 0xff);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_C | SREG_N | SREG_S | SREG_H);

	memory[31] = 0x80;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[31], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_N | SREG_V);

	memory[31] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[31], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_pop)
{
	uint16_t code[] = {
		0x900f | /*rrrrr*/ 0x1f0,	/* pop r31 */
	};

	install_words(code, PC_START, sizeof(code));
	memory[0xffff] = 0xbe;
	setsp(0xfffe);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(getsp(), 0xffff);
	ck_assert_uint_eq(memory[31], 0xbe);
}
END_TEST

START_TEST(test_rcall)
{
	uint16_t code[] = {
		0xd000 | /*k*/ 0x7ff,	/* rcall PC + 1 + $7ff */
		0xd000 | /*k*/ 0xfff,	/* rcall PC + 1 + $-1 */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1 + 0x7ff);
	ck_assert_uint_eq(getsp(), 0xfffd);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 1) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], (PC_START + 1) >> 8);

	pc = PC_START + 1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2 - 1);
	ck_assert_uint_eq(getsp(), 0xfffb);
	ck_assert_uint_eq(memory[0xfffd], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffc], (PC_START + 2) >> 8);
}
END_TEST

START_TEST(test_rcall22)
{
	uint16_t code[] = {
		0xd000 | /*k*/ 0x7ff,	/* rcall PC + 1 + $7ff */
		0xd000 | /*k*/ 0xfff,	/* rcall PC + 1 + $-1 */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1 + 0x7ff);
	ck_assert_uint_eq(getsp(), 0xfffc);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 1) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], ((PC_START + 1) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffd], (PC_START + 1) >> 16);

	pc = PC_START + 1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2 - 1);
	ck_assert_uint_eq(getsp(), 0xfff9);
	ck_assert_uint_eq(memory[0xfffc], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffb], ((PC_START + 2) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffa], (PC_START + 2) >> 16);
}
END_TEST

START_TEST(test_rjmp)
{
	uint16_t code[] = {
		0xc000 | /*k*/ 0x7ff,	/* rjmp PC + 1 + $7ff */
		0xc000 | /*k*/ 0xfff,	/* rjmp PC + 1 + $-1 */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1 + 0x7ff);
	ck_assert_uint_eq(getsp(), 0xffff);

	pc = PC_START + 1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2 - 1);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_ret)
{
	uint16_t code[] = {
		0x9508,		/* ret */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xfffd);

	memory[0xfffe] = 0xab;
	memory[0xffff] = 0xcd;

	emulate1();
	ck_assert_uint_eq(pc, 0xabcd);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_ret22)
{
	uint16_t code[] = {
		0x9508,		/* ret */
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xfffc);

	memory[0xfffd] = 0xf0;
	memory[0xfffe] = 0xab;
	memory[0xffff] = 0xcd;

	emulate1();
	ck_assert_uint_eq(pc, 0x30abcd);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_reti)
{
	uint16_t code[] = {
		0x9518,		/* reti */
	};

	install_words(code, PC_START, sizeof(code));

	setsp(0xfffd);
	memory[0xfffe] = 0xab;
	memory[0xffff] = 0xcd;

	memory[SREG] = 0;

	emulate1();
	ck_assert_uint_eq(pc, 0xabcd);
	ck_assert_uint_eq(memory[SREG], SREG_I);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_reti22)
{
	uint16_t code[] = {
		0x9518,		/* reti */
	};

	install_words(code, PC_START, sizeof(code));

	setsp(0xfffc);
	memory[0xfffd] = 0xf0;
	memory[0xfffe] = 0xab;
	memory[0xffff] = 0xcd;

	memory[SREG] = 0;

	emulate1();
	ck_assert_uint_eq(pc, 0x30abcd);
	ck_assert_uint_eq(memory[SREG], SREG_I);
	ck_assert_uint_eq(getsp(), 0xffff);
}
END_TEST

START_TEST(test_ror)
{
	uint16_t code[] = {
		0x9407 | 0x50,	/* ror r5 */
		0x9407 | 0x50,	/* ror r5 */
		0x9407 | 0x50,	/* ror r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[5], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);

	memory[5] = 0xf1;
	memory[SREG] = sreg_start | SREG_C;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[5], 0xf8);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_N | SREG_S);

	memory[5] = 1;
	memory[SREG] = sreg_start;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[5], 0);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_C | SREG_Z | SREG_V | SREG_S);
}
END_TEST

START_TEST(test_sbc)
{
	uint16_t code[] = {
		0x0800 | 0x54,		/* sbc r5, r4 */
		0x0800 | 0x54,		/* sbc r5, r4 */
		0x0800 | 0x54,		/* sbc r5, r4 */
		0x0800 | 0x54,		/* sbc r5, r4 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[SREG] = sreg_start = SREG_I | SREG_T;

	memory[5] = 0;
	memory[4] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[5], 0);
	/* SREG_Z is never set; cleared except for zero result. */
	ck_assert_uint_eq(memory[SREG], sreg_start);

	memory[SREG] = sreg_start | SREG_Z | SREG_C;
	memory[5] = 1;
	memory[4] = 0;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[5], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);

	memory[SREG] = sreg_start;
	memory[5] = 0;
	memory[4] = 0xfe;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[5], 2);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_H | SREG_C);

	memory[SREG] = sreg_start;
	memory[5] = 0x80;
	memory[4] = 1;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[5], 0x7f);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_H | SREG_V | SREG_S);
}
END_TEST

START_TEST(test_sbci)
{
	uint16_t code[] = {
		0x4000 | 0xf0f | 0x10,		/* sbci r17, $ff */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[17] = 0;
	sreg_start = SREG_I | SREG_T;
	memory[SREG] = sreg_start | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[17], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z | SREG_C | SREG_H);
}
END_TEST

START_TEST(test_subi)
{
	uint16_t code[] = {
		0x5000 | 0x70f | 0x10,		/* subi r17, $7f */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[17] = 0x80;
	memory[SREG] = sreg_start = SREG_I | SREG_T;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[17], 1);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_S | SREG_H);
}
END_TEST

START_TEST(test_sbic)
{
	uint16_t code[] = {
		0x9900 | 0xf8 | 0x7,	/* sbic $1f, 7 */
		0x9900 | 0xf0 | 0x5,	/* sbic $1e, 5 */
		0x940e | 0x1f1,		/* call 0x3fbeef */
		0xbeef,
		0x9900,			/* sbic $0, 0 */
		0,			/* nop */
		0,			/* nop */
	};

	install_words(code, PC_START, sizeof(code));

	memory[IO_BASE + 0x1f] = 0x80;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(insns, 1);

	memory[IO_BASE + 0x1e] = 0xdf;
	emulate1();
	/* Skips both words of 32-bit CALL imm16 */
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(insns, 2);

	memory[IO_BASE + 0] = 0xfe;
	emulate1();
	/* Skips only one word of NOP */
	ck_assert_uint_eq(pc, PC_START + 6);
	ck_assert_uint_eq(insns, 3);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 7);
	ck_assert_uint_eq(insns, 4);
}
END_TEST

START_TEST(test_sbis)
{
	uint16_t code[] = {
		0x9b00,		/* sbis $0, 0 */
		0,		/* nop */
	};

	install_words(code, PC_START, sizeof(code));

	memory[IO_BASE + 0] = 1;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(insns, 1);
}
END_TEST

START_TEST(test_sbiw)
{
	uint16_t code[] = {
		0x9700 | 0xcf | 0x30,		/* sbiw r31:30, 63 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xff;
	memory[30] = 0x3e;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[30], 0xff);
	ck_assert_uint_eq(memory[31], 0xfe);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_S | SREG_N);
}
END_TEST

START_TEST(test_sbrc)
{
	uint16_t code[] = {
		0xfc00 | 0x1f0 | 0x7,	/* sbrc r31, 7 */
		0xfc00 | 0x1e0 | 0x5,	/* sbrc r30, 5 */
		0x940e | 0x1f1,		/* call 0x3fbeef */
		0xbeef,
		0xfc00,			/* sbrc r0, 0 */
		0,			/* nop */
		0,			/* nop */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x80;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(insns, 1);

	memory[30] = 0xdf;
	emulate1();
	/* Skips both words of 32-bit CALL imm16 */
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(insns, 2);

	memory[0] = 0xfe;
	emulate1();
	/* Skips only one word of NOP */
	ck_assert_uint_eq(pc, PC_START + 6);
	ck_assert_uint_eq(insns, 3);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 7);
	ck_assert_uint_eq(insns, 4);
}
END_TEST

START_TEST(test_sbrs)
{
	uint16_t code[] = {
		0xfe00,		/* sbrs r0, 0 */
		0,		/* nop */
	};

	install_words(code, PC_START, sizeof(code));

	memory[0] = 1;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(insns, 1);
}
END_TEST

START_TEST(test_stx)
{
	uint16_t code[] = {
		0x920c | 0x50,	/* st X, r5 */
		0x920d | 0x50,	/* st X+, r5 */
		0x920e | 0x50,	/* st -X, r5 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[5] = 0xaa;

	memwriteword(REGP_X, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0x100);
	ck_assert_uint_eq(memory[0x100], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_X), 0x101);

	memory[RAMPX] = 0x3f;
	memwriteword(REGP_X, 0);
	memory[5] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_X), 0xffff);
	ck_assert_uint_eq(memory[RAMPX], 0x3e);
	ck_assert_uint_eq(memory[0x3effff], 0x55);

	memory[0x3effff] = 0;
	memory[0x100] = 0;
}
END_TEST

START_TEST(test_stx64)
{
	uint16_t code[] = {
		0x920e | 0x50,	/* st -X, r5 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPX] = 0x1;
	memwriteword(REGP_X, 0);
	memory[5] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0xffff);
	ck_assert_uint_eq(memory[RAMPX], 0x1);
	ck_assert_uint_eq(memory[0xffff], 0x55);

	memory[0xffff] = 0;
}
END_TEST

START_TEST(test_stx256)
{
	uint16_t code[] = {
		0x920e | 0x50,	/* st -X, r5 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPX] = 0x12;
	memwriteword(REGP_X, 0x3400);
	memory[5] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_X), 0x34ff);
	ck_assert_uint_eq(memory[RAMPX], 0x12);
	ck_assert_uint_eq(memory[0xff], 0x55);

	memory[0xff] = 0;
}
END_TEST

START_TEST(test_sty)
{
	uint16_t code[] = {
		0x8208 | 0x50,			/* st Y, r5 */
		0x9209 | 0x50,			/* st Y+, r5 */
		0x920a | 0x50,			/* st -Y, r5 */
		0x8208 | 0x2c07 | 0x1f0,	/* std Y+63, r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[5] = 0xaa;
	memory[RAMPY] = 0;

	memwriteword(REGP_Y, 0x100);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Y), 0x100);
	ck_assert_uint_eq(memory[0x100], 0xaa);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Y), 0x101);

	memory[RAMPY] = 0x3f;
	memwriteword(REGP_Y, 0);
	memory[5] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Y), 0xffff);
	ck_assert_uint_eq(memory[RAMPY], 0x3e);
	ck_assert_uint_eq(memory[0x3effff], 0x55);

	memory[RAMPY] = 0xff;
	memwriteword(REGP_Y, 0xffff);
	memory[31] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Y), 0xffff);
	ck_assert_uint_eq(memory[RAMPY], 0xff);
	ck_assert_uint_eq(memory[0x3e], 0x55);

	memory[0x3e] = 0;
	memory[0x3effff] = 0;
	memory[0x100] = 0;
}
END_TEST

START_TEST(test_sty64)
{
	uint16_t code[] = {
		0x9209 | 0x50,			/* st Y+, r5 */
		0x8208 | 0x2c07 | 0x1f0,	/* std Y+63, r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[5] = 0xaa;
	memory[RAMPY] = 1;
	memwriteword(REGP_Y, 0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0xffff], 0xaa);
	ck_assert_uint_eq(memword(REGP_Y), 0);

	memory[RAMPY] = 5;
	memwriteword(REGP_Y, 0xffff);
	memory[31] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Y), 0xffff);
	ck_assert_uint_eq(memory[0x3e], 0x55);

	memory[0x3e] = 0;
	memory[0xffff] = 0;
}
END_TEST

START_TEST(test_styz256)
{
	uint16_t code[] = {
		0x920a | 0x50,			/* st -Y, r5 */
		0x8208 | 0x2c07 | 0x1f0,	/* std Y+63, r31 */
		0x9202 | 0x50,			/* st -Z, r5 */
		0x8200 | 0x2c07 | 0x1d0,	/* std Z+63, r29 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPY] = 0x1;
	memwriteword(REGP_Y, 0);
	memory[5] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(REGP_Y), 0x00ff);
	ck_assert_uint_eq(memory[RAMPY], 0x1);
	ck_assert_uint_eq(memory[0xff], 0x55);

	memwriteword(REGP_Y, 0x1ff);
	memory[31] = 0x55;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(REGP_Y), 0x1ff);
	ck_assert_uint_eq(memory[0x3e], 0x55);

	memory[RAMPZ] = 0x1;
	memwriteword(REGP_Z, 0);
	memory[5] = 0x44;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(REGP_Z), 0x00ff);
	ck_assert_uint_eq(memory[RAMPZ], 0x1);
	ck_assert_uint_eq(memory[0xff], 0x44);

	memwriteword(REGP_Z, 0x1ff);
	memory[29] = 0x66;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memword(REGP_Z), 0x1ff);
	ck_assert_uint_eq(memory[0x3e], 0x66);

	memory[0x3e] = 0;
	memory[0xff] = 0;
}
END_TEST

START_TEST(test_sts)
{
	uint16_t code[] = {
		0x9200 | 0x1f0,		/* sts 0x2345, r31 */
		0x2345
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPD] = 0x1;
	memory[31] = 0x1b;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0x12345], 0x1b);

	memory[0x12345] = 0;
}
END_TEST

START_TEST(test_sts64)
{
	uint16_t code[] = {
		0x9200 | 0x1f0,		/* sts 0x2345, r31 */
		0x2345
	};

	install_words(code, PC_START, sizeof(code));

	memory[RAMPD] = 0x1;
	memory[31] = 0x1b;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0x2345], 0x1b);

	memory[0x2345] = 0;
}
END_TEST

START_TEST(test_sts256)
{
	uint16_t code[] = {
		0x9200 | 0x1f0,		/* sts 0x2345, r31 */
		0x2345
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x1b;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[0x45], 0x1b);

	memory[0x45] = 0;
}
END_TEST

START_TEST(test_swap)
{
	uint16_t code[] = {
		0x9402 | 0x1f0,		/* swap r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xa5;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0x5a);
}
END_TEST

/*
 * XXX: May not be totally correct. I couldn't find an instruction manual that
 * actually mentions these instructions yet.
 *
 * In particular, it isn't clear if XMEGA will use the RAMPZ extension to the Z
 * register.
 */
START_TEST(test_xch)
{
	uint16_t code[] = {
		0x9204 | 0x1d0,		/* xch r29 */
		0x9205 | 0x1d0,		/* las r29 */
		0x9206 | 0x1d0,		/* lac r29 */
		0x9207 | 0x1d0,		/* lat r29 */
	};

	install_words(code, PC_START, sizeof(code));

	memwriteword(REGP_Z, 0xabcd);
	memory[0xabcd] = 0xfe;
	memory[29] = 0x12;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[29], 0xfe);
	ck_assert_uint_eq(memory[0xabcd], 0x12);

	memory[0xabcd] = 0xa5;
	memory[29] = 0x5a;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[29], 0xa5);
	ck_assert_uint_eq(memory[0xabcd], 0xff);

	memory[29] = 0xa5;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[29], 0xff);
	ck_assert_uint_eq(memory[0xabcd], 0x5a);

	memory[29] = 0xd2;
	memory[0xabcd] = 0x3c;
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[29], 0x3c);
	ck_assert_uint_eq(memory[0xabcd], 0xee);

	memory[0xabcd] = 0;
}
END_TEST

START_TEST(test_des)
{
	uint16_t code[] = {
		0x940b | 0x00,	/* DES(0) */
		0x940b | 0x10,	/* DES(1) */
		0x940b | 0x20,	/* DES(2) */
		0x940b | 0x30,	/* DES(3) */
		0x940b | 0x40,	/* DES(4) */
		0x940b | 0x50,	/* DES(5) */
		0x940b | 0x60,	/* DES(6) */
		0x940b | 0x70,	/* DES(7) */
		0x940b | 0x80,	/* DES(8) */
		0x940b | 0x90,	/* DES(9) */
		0x940b | 0xa0,	/* DES(10) */
		0x940b | 0xb0,	/* DES(11) */
		0x940b | 0xc0,	/* DES(12) */
		0x940b | 0xd0,	/* DES(13) */
		0x940b | 0xe0,	/* DES(14) */
		0x940b | 0xf0,	/* DES(15) */
	};
	struct des_trial {
		bool enc;
		uint8_t key[8];
		uint8_t inp[8];
		uint8_t exp[8];
	} cases[] = {
		/* Some randoml inputs; expected value from OpenSSL */
		/* Encrypt */
		{true,  { 0x07, 0x6e, 0xb0, 0x8a, 0x16, 0xec, 0x37, 0xce, },
			{ 0x1f, 0x40, 0xec, 0xd7, 0x46, 0x8e, 0xb3, 0x23, },
			{ 0xc3, 0x83, 0x83, 0xf7, 0x0f, 0x86, 0xa4, 0xe1, }
		},
		{true,  { 0xf1, 0x10, 0xfe, 0xc1, 0x5d, 0x62, 0x7c, 0xd3, },
			{ 0xf9, 0xa6, 0xab, 0x9b, 0x0b, 0xc6, 0x76, 0x7a, },
			{ 0x94, 0xda, 0x96, 0xd1, 0xc2, 0xf2, 0xb6, 0x94, }
		},
		{true,  { 0x6d, 0xc7, 0x4a, 0xd5, 0x16, 0xa7, 0x1a, 0x1a, },
			{ 0x3f, 0x4e, 0xe4, 0x13, 0xc6, 0x20, 0x98, 0x89, },
			{ 0x68, 0xc1, 0xe5, 0x18, 0x0c, 0x60, 0x5f, 0x49, }
		},
		{true,  { 0xb0, 0x64, 0x67, 0xd3, 0xdf, 0x7a, 0xb9, 0xd0, },
			{ 0x3c, 0x28, 0x83, 0x1b, 0xf2, 0x7b, 0xb6, 0xa3, },
			{ 0x11, 0x83, 0x95, 0xd1, 0x55, 0x87, 0x06, 0x7c, }
		},
		{true,  { 0x98, 0x86, 0xa4, 0xe6, 0xa7, 0xa8, 0xc2, 0x20, },
			{ 0xf4, 0x49, 0x53, 0x50, 0xb1, 0x46, 0x16, 0x4b, },
			{ 0xc8, 0xc8, 0xf6, 0x91, 0x5f, 0xb7, 0x27, 0xed, }
		},
		{true,  { 0x92, 0x64, 0xd9, 0xb0, 0xf4, 0x10, 0x4a, 0xce, },
			{ 0xa6, 0x58, 0x5c, 0x6f, 0x20, 0x51, 0xfb, 0xec, },
			{ 0x46, 0xe7, 0x54, 0x01, 0x81, 0xec, 0x64, 0x4c, }
		},
		{true,  { 0x7a, 0x02, 0x89, 0xae, 0x1f, 0x5d, 0xf4, 0x32, },
			{ 0xa4, 0xd3, 0xeb, 0x27, 0x5e, 0xd9, 0x48, 0x66, },
			{ 0x72, 0xcb, 0x22, 0x2a, 0xe2, 0x62, 0xf2, 0x2e, }
		},
		{true,  { 0x0b, 0xea, 0x9e, 0x52, 0x26, 0xa4, 0x57, 0x40, },
			{ 0x64, 0xb3, 0x32, 0x39, 0x86, 0xeb, 0x1e, 0x8b, },
			{ 0xbd, 0xc1, 0xdf, 0x35, 0x78, 0x75, 0x61, 0x25, }
		},
		/* Decrypt */
		{false, { 0xda, 0x98, 0xa1, 0xc7, 0x3b, 0xea, 0x45, 0xad, },
			{ 0xac, 0xc3, 0xd6, 0x88, 0x17, 0x4c, 0x75, 0xe1, },
			{ 0x54, 0x16, 0x69, 0x44, 0xea, 0x93, 0x0f, 0x1a, }
		},
		{false, { 0x1c, 0x26, 0xb9, 0xb9, 0xf7, 0xef, 0xc2, 0x40, },
			{ 0x91, 0x05, 0x89, 0xae, 0xc9, 0xe5, 0x26, 0x2f, },
			{ 0x8f, 0x31, 0xf5, 0xdc, 0xa8, 0x44, 0xca, 0x3f, }
		},
		{false, { 0x94, 0xdf, 0x1a, 0x32, 0xf2, 0x31, 0x6b, 0xbf, },
			{ 0xff, 0x75, 0x50, 0x3e, 0xee, 0xf9, 0xf2, 0xde, },
			{ 0x0d, 0xd4, 0xf3, 0xe8, 0x50, 0xb9, 0x4e, 0x83, }
		},
		{false, { 0x89, 0xbf, 0xe9, 0xc4, 0x10, 0x3d, 0xc1, 0xe6, },
			{ 0x3c, 0x1e, 0xd0, 0xbd, 0x88, 0xda, 0xc6, 0xc0, },
			{ 0x88, 0x12, 0xf7, 0x85, 0x09, 0xb9, 0x99, 0x75, }
		},
		{false, { 0x76, 0x6d, 0x16, 0x43, 0x25, 0x64, 0x43, 0x6e, },
			{ 0x48, 0x8c, 0x07, 0xb3, 0x06, 0xb9, 0xee, 0x7f, },
			{ 0xf9, 0xa1, 0x5c, 0x3b, 0xdf, 0x32, 0xca, 0x19, }
		},
		{false, { 0xa4, 0x76, 0x76, 0xb9, 0x37, 0x91, 0xa7, 0x3b, },
			{ 0x32, 0xf1, 0x51, 0x3c, 0x87, 0x53, 0xb1, 0xf3, },
			{ 0xd7, 0x27, 0x1e, 0x0f, 0xbf, 0xb2, 0xaa, 0xee, }
		},
		{false, { 0xf2, 0x16, 0xf8, 0xc1, 0xce, 0x8a, 0x37, 0xc7, },
			{ 0xac, 0x5e, 0xb1, 0x68, 0xf9, 0x46, 0xba, 0x12, },
			{ 0xc8, 0x79, 0x9b, 0xb8, 0xed, 0x32, 0xf3, 0x02, }
		},
		{false, { 0xd0, 0xa7, 0xdc, 0xdc, 0xf2, 0xc1, 0x20, 0x4a, },
			{ 0x50, 0x75, 0x0b, 0x81, 0x76, 0x40, 0xde, 0x01, },
			{ 0x0b, 0x3f, 0x6a, 0xca, 0xb6, 0xcd, 0x40, 0x73, }
		},
	};
	unsigned i, tr;

	install_words(code, PC_START, sizeof(code));

	for (tr = 0; tr < ARRAYLEN(cases); tr++) {
		memcpy(memory, cases[tr].inp, sizeof(cases[tr].inp));
		memcpy(&memory[8], cases[tr].key, sizeof(cases[tr].key));
		if (cases[tr].enc)
			memory[SREG] &= ~SREG_H;
		else
			memory[SREG] |= SREG_H;

		pc = PC_START;
		for (i = 0; i < 16; i++)
			emulate1();

		ck_assert_msg(
		    memcmp(memory, cases[tr].exp, sizeof(cases[tr].exp)) == 0,
		    "Assertion failed @%u", tr);
	}
}
END_TEST

Suite *
suite_instr(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("instr");

	t = tcase_create("nop");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_nop);
	suite_add_tcase(s, t);

	t = tcase_create("basic");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_and);
	tcase_add_test(t, test_andi);
	tcase_add_test(t, test_bclr);
	tcase_add_test(t, test_bld);
	tcase_add_test(t, test_bset);
	tcase_add_test(t, test_bst);
	tcase_add_test(t, test_call);
	tcase_add_test(t, test_cbisbi);
	tcase_add_test(t, test_in);
	tcase_add_test(t, test_jmp);
	tcase_add_test(t, test_ldi);
	tcase_add_test(t, test_lds);
	tcase_add_test(t, test_mov);
	tcase_add_test(t, test_or);
	tcase_add_test(t, test_ori);
	tcase_add_test(t, test_out);
	tcase_add_test(t, test_pop);
	tcase_add_test(t, test_push);
	tcase_add_test(t, test_elpm);
	tcase_add_test(t, test_ldx);
	tcase_add_test(t, test_ldy);
	tcase_add_test(t, test_ldz);
	tcase_add_test(t, test_ret);
	tcase_add_test(t, test_reti);
	tcase_add_test(t, test_stx);
	tcase_add_test(t, test_sty);
	tcase_add_test(t, test_sts);
	tcase_add_test(t, test_swap);
	tcase_add_test(t, test_xch);
	tcase_add_test(t, test_xor);
	suite_add_tcase(s, t);

	t = tcase_create("regpairs");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_movw);
	suite_add_tcase(s, t);

	t = tcase_create("22-bit pc");
	tcase_add_checked_fixture(t, setup_machine22, teardown_machine);
	tcase_add_test(t, test_call22);
	tcase_add_test(t, test_eicall22);
	tcase_add_test(t, test_eijump22);
	tcase_add_test(t, test_jmp22);
	tcase_add_test(t, test_rcall22);
	tcase_add_test(t, test_ret22);
	tcase_add_test(t, test_reti22);
	suite_add_tcase(s, t);

	t = tcase_create("256 byte RAM");
	tcase_add_checked_fixture(t, setup_machine256, teardown_machine);
	tcase_add_test(t, test_ldx256);
	tcase_add_test(t, test_ldy256);
	tcase_add_test(t, test_ldz256);
	tcase_add_test(t, test_sts256);
	tcase_add_test(t, test_stx256);
	tcase_add_test(t, test_styz256);
	suite_add_tcase(s, t);

	t = tcase_create("64 kB RAM");
	tcase_add_checked_fixture(t, setup_machine64, teardown_machine);
	tcase_add_test(t, test_lds64);
	tcase_add_test(t, test_ldx64);
	tcase_add_test(t, test_ldy64);
	tcase_add_test(t, test_ldz64);
	tcase_add_test(t, test_sts64);
	tcase_add_test(t, test_stx64);
	tcase_add_test(t, test_sty64);
	suite_add_tcase(s, t);

	t = tcase_create("math");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_adc);
	tcase_add_test(t, test_add);
	tcase_add_test(t, test_adiw);
	tcase_add_test(t, test_com);
	tcase_add_test(t, test_cpc);
	tcase_add_test(t, test_cpi);
	tcase_add_test(t, test_dec);
	tcase_add_test(t, test_des);
	tcase_add_test(t, test_fmul);
	tcase_add_test(t, test_fmuls);
	tcase_add_test(t, test_fmulsu);
	tcase_add_test(t, test_inc);
	tcase_add_test(t, test_mul);
	tcase_add_test(t, test_muls);
	tcase_add_test(t, test_mulsu);
	tcase_add_test(t, test_neg);
	tcase_add_test(t, test_sbc);
	tcase_add_test(t, test_sbci);
	tcase_add_test(t, test_subi);
	tcase_add_test(t, test_sbiw);
	suite_add_tcase(s, t);

	t = tcase_create("shifts");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_asr);
	tcase_add_test(t, test_lsr);
	tcase_add_test(t, test_ror);
	suite_add_tcase(s, t);

	t = tcase_create("branches");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_brb);
	tcase_add_test(t, test_cpse);
	tcase_add_test(t, test_icall);
	tcase_add_test(t, test_ijump);
	tcase_add_test(t, test_rcall);
	tcase_add_test(t, test_rjmp);
	tcase_add_test(t, test_sbic);
	tcase_add_test(t, test_sbis);
	tcase_add_test(t, test_sbrc);
	tcase_add_test(t, test_sbrs);
	suite_add_tcase(s, t);

	return (s);
}
