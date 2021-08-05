/* Fake6502 CPU emulator core v1.2 *******************
 * Current Maintainer: David MHS Webster             *
 * https://github.com/gek169                         *
 * Original Author: Mike Chambers                    *
 * License: CC0/Public Domain                        *
 *****************************************************
 * v1.2 - Fixed decimal mode along with several      *
 * other oddities that don't match up with tests.    *
 *                                                   *
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * LICENSE: This source code is released into the    *
 * public domain, but if you use it please do give   *
 * credit. I put a lot of effort into writing this!  *
 * Note by GEK: this is not a requirement.           *
 *****************************************************
 * Fake6502 is a MOS Technology 6502 CPU emulation   *
 * engine in C. It was written as part of a Nintendo *
 * Entertainment System emulator I've been writing.  *
 *                                                   *
 * A couple important things to know about are two   *
 * defines in the code. One is "UNDOCUMENTED" which, *
 * when defined, allows Fake6502 to compile with     *
 * full support for the more predictable             *
 * undocumented instructions of the 6502. If it is   *
 * undefined, undocumented opcodes just act as NOPs. *
 *                                                   *
 * The other define is "NES_CPU", which causes the   *
 * code to compile without support for binary-coded  *
 * decimal (BCD) support for the ADC and SBC         *
 * opcodes. The Ricoh 2A03 CPU in the NES does not   *
 * support BCD, but is otherwise identical to the    *
 * standard MOS 6502. (Note that this define is      *
 * enabled in this file if you haven't changed it    *
 * yourself. If you're not emulating a NES, you      *
 * should comment it out.)                           *
 *                                                   *
 * If you do discover an error in timing accuracy,   *
 * or operation in general please e-mail me at the   *
 * address above so that I can fix it. Thank you!    *
 *                                                   *
 *****************************************************
 * Usage:                                            *
 *                                                   *
 * Fake6502 requires you to provide two external     *
 * functions:                                        *
 *                                                   *
 * uint8 read6502(ushort address)                    *
 * void write6502(ushort address, uint8 value)       *
 *                                                   *
 * You may optionally pass Fake6502 the pointer to a *
 * function which you want to be called after every  *
 * emulated instruction. This function should be a   *
 * void with no parameters expected to be passed to  *
 * it.                                               *
 *                                                   *
 * This can be very useful. For example, in a NES    *
 * emulator, you check the number of clock ticks     *
 * that have passed so you can know when to handle   *
 * APU events.                                       *
 *                                                   *
 * To pass Fake6502 this pointer, use the            *
 * hookexternal(void *funcptr) function provided.    *
 *                                                   *
 * To disable the hook later, pass NULL to it.       *
 *****************************************************
 * Useful functions in this emulator:                *
 *                                                   *
 * void reset6502()                                  *
 *   - Call this once before you begin execution.    *
 *                                                   *
 * void exec6502(uint32 tickcount)                 *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step6502()                                   *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq6502()                                    *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi6502()                                    *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 * void hookexternal(void *funcptr)                  *
 *   - Pass a pointer to a void function taking no   *
 *     parameters. This will cause Fake6502 to call  *
 *     that function once after each emulated        *
 *     instruction.                                  *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint32 clockticks6502                           *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 * uint32 instructions                             *
 *   - A running total of the total emulated         *
 *     instruction count. This is not related to     *
 *     clock cycle timing.                           *
 *                                                   *
 *****************************************************/

#include <stdio.h>
#ifdef FAKE6502_USE_STDINT
#include <stdint.h>
typedef uint16_t ushort;
typedef unsigned char uint8;
typedef uint32_t uint32
#else
typedef unsigned short ushort ;
typedef unsigned char uint8;

#ifdef FAKE6502_USE_LONG
typedef unsigned long uint32;
#else
typedef unsigned int uint32;
#endif
#endif
/*
	when this is defined, undocumented opcodes are handled.
	otherwise, they're simply treated as NOPs.
*/
#define UNDOCUMENTED

/*
* #define NES_CPU
* when this is defined, the binary-coded decimal (BCD)
* status flag is not honored by ADC and SBC. the 2A03
* CPU in the Nintendo Entertainment System does not
* support BCD operation.
*/


#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

#define saveaccum(n) a_6502 = (uint8)((n) & 0x00FF)


/*flag modifier macros*/
#define setcarry() status_6502 |= FLAG_CARRY
#define clearcarry() status_6502 &= (~FLAG_CARRY)
#define setzero() status_6502 |= FLAG_ZERO
#define clearzero() status_6502 &= (~FLAG_ZERO)
#define setinterrupt() status_6502 |= FLAG_INTERRUPT
#define clearinterrupt() status_6502 &= (~FLAG_INTERRUPT)
#define setdecimal() status_6502 |= FLAG_DECIMAL
#define cleardecimal() status_6502 &= (~FLAG_DECIMAL)
#define setoverflow() status_6502 |= FLAG_OVERFLOW
#define clearoverflow() status_6502 &= (~FLAG_OVERFLOW)
#define setsign() status_6502 |= FLAG_SIGN
#define clearsign() status_6502 &= (~FLAG_SIGN)


/*flag calculation macros*/
#define zerocalc(n) {\
    if ((n) & 0x00FF) clearzero();\
        else setzero();\
}

#define signcalc(n) {\
    if ((n) & 0x0080) setsign();\
        else clearsign();\
}

#define carrycalc(n) {\
    if ((n) & 0xFF00) setcarry();\
        else clearcarry();\
}

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (ushort)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}


#ifdef FAKE6502_NOT_STATIC
/*6502 CPU registers*/
ushort pc_6502;
uint8 sp_6502, a_6502, x_6502, y_6502, status_6502;
/*helper variables*/
uint32 instructions6502 = 0; 
uint32 clockticks6502 = 0, clockgoal6502 = 0;
ushort oldpc_6502, ea_6502, reladdr_6502, value_6502, result_6502;
uint8 opcode6502, oldstatus6502;
#else
static ushort pc_6502;
static uint8 sp_6502, a_6502, x_6502, y_6502, status_6502;
static uint32 instructions6502 = 0; 
static uint32 clockticks6502 = 0, clockgoal6502 = 0;
static ushort oldpc_6502, ea_6502, reladdr_6502, value_6502, result_6502;
static uint8 opcode6502, oldstatus6502;
#endif
/*externally supplied functions*/
extern uint8 read6502(ushort address);
extern void write6502(ushort address, uint8 value);

/*a few general functions used by various other functions*/
void push_6502_16(ushort pushval) {
    write6502(BASE_STACK + sp_6502, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((sp_6502 - 1) & 0xFF), pushval & 0xFF);
    sp_6502 -= 2;
}

void push_6502_8(uint8 pushval) {
    write6502(BASE_STACK + sp_6502--, pushval);
}

ushort pull_6502_16() {
    ushort temp16;
    temp16 = read6502(BASE_STACK + ((sp_6502 + 1) & 0xFF)) | 
    		((ushort)read6502(BASE_STACK + ((sp_6502 + 2) & 0xFF)) << 8);
    sp_6502 += 2;
    return(temp16);
}

uint8 pull_6502_8() {
    return (read6502(BASE_STACK + ++sp_6502));
}

static ushort mem_6502_read16(ushort addr) {
    return ((ushort)read6502(addr) |
            ((ushort)read6502(addr + 1) << 8));
}

void reset6502() {
    read6502(0x00ff);
    read6502(0x00ff);
    read6502(0x00ff);
    read6502(0x0100);
    read6502(0x01ff);
    read6502(0x01fe);
    pc_6502 = mem_6502_read16( 0xfffc);
    sp_6502 = 0xfd;
    status_6502 |= FLAG_CONSTANT | FLAG_INTERRUPT;
}


static void (*addrtable[256])();
static void (*optable[256])();
uint8 penaltyop, penaltyaddr;

/*addressing mode functions, calculates effective addresses*/
static void imp() { 
}

/*addressing mode functions, calculates effective addresses*/
static void acc() { 
}

/*addressing mode functions, calculates effective addresses*/
static void imm() { 
    ea_6502 = pc_6502++;
}

static void zp() { /*zero-page*/
    ea_6502 = (ushort)read6502((ushort)pc_6502++);
}

static void zpx() { /*zero-page,X*/
    ea_6502 = ((ushort)read6502((ushort)pc_6502++) + (ushort)x_6502) & 0xFF; /*zero-page wraparound*/
}

static void zpy() { /*zero-page,Y*/
    ea_6502 = ((ushort)read6502((ushort)pc_6502++) + (ushort)y_6502) & 0xFF; /*zero-page wraparound*/
}

static void rel() { /*relative for branch ops (8-bit immediate value, sign-extended)*/
    reladdr_6502 = (ushort)read6502(pc_6502++);
    if (reladdr_6502 & 0x80) reladdr_6502 |= 0xFF00;
}

static void abso() { /*absolute*/
    ea_6502 = (ushort)read6502(pc_6502) | ((ushort)read6502(pc_6502+1) << 8);
    pc_6502 += 2;
}

static void absx() { /*absolute,X*/
    ushort startpage;
    ea_6502 = ((ushort)read6502(pc_6502) | ((ushort)read6502(pc_6502+1) << 8));
    startpage = ea_6502 & 0xFF00;
    ea_6502 += (ushort)x_6502;

    if (startpage != (ea_6502 & 0xFF00)) { /*one cycle penlty for page-crossing on some opcodes*/
        penaltyaddr = 1;
    }

    pc_6502 += 2;
}

static void absy() { /*absolute,Y*/
    ushort startpage;
    ea_6502 = ((ushort)read6502(pc_6502) | ((ushort)read6502(pc_6502+1) << 8));
    startpage = ea_6502 & 0xFF00;
    ea_6502 += (ushort)y_6502;

    if (startpage != (ea_6502 & 0xFF00)) { /*one cycle penlty for page-crossing on some opcodes*/
        penaltyaddr = 1;
    }

    pc_6502 += 2;
}



static void indx() { /* (indirect,X)*/
    ushort eahelp;
    eahelp = (ushort)(((ushort)read6502(pc_6502++) + (ushort)x_6502) & 0xFF); /*zero-page wraparound for table pointer*/
    ea_6502 = (ushort)read6502(eahelp & 0x00FF) | ((ushort)read6502((eahelp+1) & 0x00FF) << 8);
}

#ifndef CMOS6502
static void ind() { /*indirect addressing on the normal NMOS 6502*/
    ushort eahelp, eahelp2;
    eahelp = mem_6502_read16(pc_6502);
    eahelp2 =
        (eahelp & 0xFF00) |
        ((eahelp + 1) & 0x00FF);
	ea_6502 =
		(ushort)read6502(eahelp) | 
		((ushort)read6502(eahelp2) << 8);
    pc_6502 += 2;
}
#else
static void ind() { /*Fixed version from the CMOS chips.*/
    ushort eahelp;
    eahelp = mem_6502_read16(pc_6502);
    if ((eahelp&0x00ff)==0xff) clockticks6502++;
    ea_6502 = mem_6502_read16(eahelp);
    pc_6502 += 2;
}
#endif


static void indy() { /* (indirect),Y*/
    ushort eahelp, eahelp2, startpage;
    eahelp = (ushort)read6502(pc_6502++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); /*zero-page wraparound*/
    ea_6502 = (ushort)read6502(eahelp) | ((ushort)read6502(eahelp2) << 8);
    startpage = ea_6502 & 0xFF00;
    ea_6502 += (ushort)y_6502;

    if (startpage != (ea_6502 & 0xFF00)) { /*one cycle penlty for page-crossing on some opcodes*/
        penaltyaddr = 1;
    }
}

static ushort getvalue() {

    if (addrtable[opcode6502] == acc) 
    	return((ushort)a_6502);
	else 
    	return((ushort)read6502(ea_6502));
}

static ushort getvalue16() {
    return((ushort)read6502(ea_6502) | ((ushort)read6502(ea_6502+1) << 8));
}

static void putvalue(ushort saveval) {
    if (addrtable[opcode6502] == acc) 
    	a_6502 = (uint8)(saveval & 0x00FF);
    else 
    	write6502(ea_6502, (saveval & 0x00FF));
}


/*instruction handler functions*/
static void adc() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 + value_6502 + (ushort)(status_6502 & FLAG_CARRY);
   
    carrycalc(result_6502);
    zerocalc(result_6502);
    overflowcalc(result_6502, a_6502, value_6502);
    signcalc(result_6502);
    
#ifndef NES_CPU
    if (status_6502 & FLAG_DECIMAL) {
        clearcarry();
        if ((result_6502 & 0x0F) > 0x09) {
            result_6502 += 0x06;
        }
        if ((result_6502 & 0xF0) > 0x90) {
            result_6502 += 0x60;
            setcarry();
        }
        clockticks6502++;
    }
#endif
    saveaccum(result_6502);
}


/*
    uint16_t result = a + b + (uint16_t)(carry ? 1 : 0);

    carrycalc(c, result);
    zerocalc(c, result);
    overflowcalc(c, result, a, b);
    signcalc(c, result);

#ifdef DECIMALMODE
    if (c->flags & FLAG_DECIMAL) {
        clearcarry(c);

        if ((result & 0x0F) > 0x09) {
            result += 0x06;
        }
        if ((result & 0xF0) > 0x90) {
            result += 0x60;
            setcarry(c);
        }

        c->clockticks++;
    }
#endif
    return result;

*/

static void and() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 & value_6502;
   
    zerocalc(result_6502);
    signcalc(result_6502);
   
    saveaccum(result_6502);
}

static void asl() {
    value_6502 = getvalue();
    result_6502 = value_6502 << 1;

    carrycalc(result_6502);
    zerocalc(result_6502);
    signcalc(result_6502);
   
    putvalue(result_6502);
}

static void bcc() {
    if ((status_6502 & FLAG_CARRY) == 0) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void bcs() {
    if ((status_6502 & FLAG_CARRY) == FLAG_CARRY) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void beq() {
    if ((status_6502 & FLAG_ZERO) == FLAG_ZERO) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void bit() {
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 & value_6502;
   
    zerocalc(result_6502);
    status_6502 = (status_6502 & 0x3F) | (uint8)(value_6502 & 0xC0);
}

static void bmi() {
    if ((status_6502 & FLAG_SIGN) == FLAG_SIGN) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void bne() {
    if ((status_6502 & FLAG_ZERO) == 0) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void bpl() {
    if ((status_6502 & FLAG_SIGN) == 0) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void brk_6502() {
    pc_6502++;
    push_6502_16(pc_6502); /*push next instruction address onto stack*/
    push_6502_8(status_6502 | FLAG_BREAK); /*push CPU status to stack*/
    setinterrupt(); /*set interrupt flag*/
    pc_6502 = (ushort)read6502(0xFFFE) | ((ushort)read6502(0xFFFF) << 8);
}

static void bvc() {
    if ((status_6502 & FLAG_OVERFLOW) == 0) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void bvs() {
    if ((status_6502 & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
        oldpc_6502 = pc_6502;
        pc_6502 += reladdr_6502;
        if ((oldpc_6502 & 0xFF00) != (pc_6502 & 0xFF00)) clockticks6502 += 2; /*check if jump crossed a page boundary*/
            else clockticks6502++;
    }
}

static void clc() {
    clearcarry();
}

static void cld() {
    cleardecimal();
}

static void cli() {
    clearinterrupt();
}

static void clv() {
    clearoverflow();
}

static void cmp() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 - value_6502;
   
    if (a_6502 >= (uint8)(value_6502 & 0x00FF)) setcarry();
        else clearcarry();
    if (a_6502 == (uint8)(value_6502 & 0x00FF)) setzero();
        else clearzero();
    signcalc(result_6502);
}

static void cpx() {
    value_6502 = getvalue();
    result_6502 = (ushort)x_6502 - value_6502;
   
    if (x_6502 >= (uint8)(value_6502 & 0x00FF)) setcarry();
        else clearcarry();
    if (x_6502 == (uint8)(value_6502 & 0x00FF)) setzero();
        else clearzero();
    signcalc(result_6502);
}

static void cpy() {
    value_6502 = getvalue();
    result_6502 = (ushort)y_6502 - value_6502;
   
    if (y_6502 >= (uint8)(value_6502 & 0x00FF)) setcarry();
        else clearcarry();
    if (y_6502 == (uint8)(value_6502 & 0x00FF)) setzero();
        else clearzero();
    signcalc(result_6502);
}

static void dec() {
    value_6502 = getvalue();
    result_6502 = value_6502 - 1;
   
    zerocalc(result_6502);
    signcalc(result_6502);
   
    putvalue(result_6502);
}

static void dex() {
    x_6502--;
   
    zerocalc(x_6502);
    signcalc(x_6502);
}

static void dey() {
    y_6502--;
   
    zerocalc(y_6502);
    signcalc(y_6502);
}

static void eor() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 ^ value_6502;
   
    zerocalc(result_6502);
    signcalc(result_6502);
   
    saveaccum(result_6502);
}

static void inc() {
    value_6502 = getvalue();
    result_6502 = value_6502 + 1;
   
    zerocalc(result_6502);
    signcalc(result_6502);
   
    putvalue(result_6502);
}

static void inx() {
    x_6502++;
   
    zerocalc(x_6502);
    signcalc(x_6502);
}

static void iny() {
    y_6502++;
   
    zerocalc(y_6502);
    signcalc(y_6502);
}

static void jmp() {
    pc_6502 = ea_6502;
}

static void jsr() {
    push_6502_16(pc_6502 - 1);
    pc_6502 = ea_6502;
}

static void lda() {
    penaltyop = 1;
    value_6502 = getvalue();
    a_6502 = (uint8)(value_6502 & 0x00FF);
   
    zerocalc(a_6502);
    signcalc(a_6502);
}

static void ldx() {
    penaltyop = 1;
    value_6502 = getvalue();
    x_6502 = (uint8)(value_6502 & 0x00FF);
   
    zerocalc(x_6502);
    signcalc(x_6502);
}

static void ldy() {
    penaltyop = 1;
    value_6502 = getvalue();
    y_6502 = (uint8)(value_6502 & 0x00FF);
   
    zerocalc(y_6502);
    signcalc(y_6502);
}

static void lsr() {
    value_6502 = getvalue();
    result_6502 = value_6502 >> 1;
   
    if (value_6502 & 1) setcarry();
        else clearcarry();
    zerocalc(result_6502);
    signcalc(result_6502);
   
    putvalue(result_6502);
}

static void nop() {
    switch (opcode6502) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }
}

static void ora() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = (ushort)a_6502 | value_6502;
   
    zerocalc(result_6502);
    signcalc(result_6502);
   
    saveaccum(result_6502);
}

static void pha() {
    push_6502_8(a_6502);
}

static void php() {
    push_6502_8(status_6502 | FLAG_BREAK);
}

static void pla() {
    a_6502 = pull_6502_8();
   
    zerocalc(a_6502);
    signcalc(a_6502);
}

static void plp() {
    status_6502 = pull_6502_8() | FLAG_CONSTANT;
}

static void rol() {
    value_6502 = getvalue();
    result_6502 = (value_6502 << 1) | (status_6502 & FLAG_CARRY);
    carrycalc(result_6502);
    zerocalc(result_6502);
    signcalc(result_6502);
    putvalue(result_6502);
}

static void ror() {
    value_6502 = getvalue();
    result_6502 = (value_6502 >> 1) | ((status_6502 & FLAG_CARRY) << 7);
    if (value_6502 & 1) setcarry();
        else clearcarry();
    zerocalc(result_6502);
    signcalc(result_6502);
    putvalue(result_6502);
}

static void rti() {
    status_6502 = pull_6502_8();
    value_6502 = pull_6502_16();
    pc_6502 = value_6502;
}

static void rts() {
    value_6502 = pull_6502_16();
    pc_6502 = value_6502 + 1;
}

static void sbc() {
    penaltyop = 1;
    value_6502 = getvalue();
    result_6502 = ((ushort)a_6502) - value_6502 - (ushort)(status_6502 & FLAG_CARRY);

    carrycalc(result_6502);
    zerocalc(result_6502);
    overflowcalc(result_6502, a_6502, value_6502);
    signcalc(result_6502);

#ifndef NES_CPU
    if (status_6502 & FLAG_DECIMAL) {
        clearcarry();
        /*result -= 0x66;*/
        if ((result_6502 & 0x0F) > 0x09) {
            result_6502 -= 0x06;
        }
        if ((result_6502 & 0xF0) > 0x90) {
            result_6502 -= 0x60;
            setcarry();
        }
        clockticks6502++;
    }
#endif
   
    saveaccum(result_6502);
}

static void sec() {
    setcarry();
}

static void sed() {
    setdecimal();
}

static void sei() {
    setinterrupt();
}

static void sta() {
    putvalue(a_6502);
}

static void stx() {
    putvalue(x_6502);
}

static void sty() {
    putvalue(y_6502);
}

static void tax() {
    x_6502 = a_6502;
   
    zerocalc(x_6502);
    signcalc(x_6502);
}

static void tay() {
    y_6502 = a_6502;
   
    zerocalc(y_6502);
    signcalc(y_6502);
}

static void tsx() {
    x_6502 = sp_6502;
   
    zerocalc(x_6502);
    signcalc(x_6502);
}

static void txa() {
    a_6502 = x_6502;
   
    zerocalc(a_6502);
    signcalc(a_6502);
}

static void txs() {
    sp_6502 = x_6502;
}

static void tya() {
    a_6502 = y_6502;
   
    zerocalc(a_6502);
    signcalc(a_6502);
}

/*undocumented instructions~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef UNDOCUMENTED
    static void lax() {
        lda();
        ldx();
    }

    static void sax() {
        sta();
        stx();
        putvalue(a_6502 & x_6502);
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void dcp() {
        dec();
        cmp();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void isb() {
        inc();
        sbc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void slo() {
        asl();
        ora();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rla() {
        rol();
        and();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void sre() {
        lsr();
        eor();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rra() {
        ror();
        adc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }
#else
    #define lax nop
    #define sax nop
    #define dcp nop
    #define isb nop
    #define slo nop
    #define rla nop
    #define sre nop
    #define rra nop
#endif


static void (*addrtable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
/* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
/* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
/* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
/* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
/* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
/* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
/* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
/* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
/* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
/* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
/* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
/* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
/* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
/* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
/* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
};

static void (*optable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
/* 0 */      brk_6502,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
/* 1 */      bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
/* 2 */      jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
/* 3 */      bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
/* 4 */      rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
/* 5 */      bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
/* 6 */      rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
/* 7 */      bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
/* 8 */      nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
/* 9 */      bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
/* A */      ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
/* B */      bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
/* C */      cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
/* D */      bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
/* E */      cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
/* F */      beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
};

static const uint32 ticktable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};


void nmi6502() {
    push_6502_16(pc_6502);
    push_6502_8(status_6502);
    status_6502 |= FLAG_INTERRUPT;
    pc_6502 = (ushort)read6502(0xFFFA) | ((ushort)read6502(0xFFFB) << 8);
}

void irq6502() {
	/*
    push_6502_16(pc_6502);
    push_6502_8(status);
    status |= FLAG_INTERRUPT;
    pc_6502 = (ushort)read6502(0xFFFE) | ((ushort)read6502(0xFFFF) << 8);
    */
   if ((status_6502 & FLAG_INTERRUPT) == 0) {
        push_6502_16(pc_6502);
        push_6502_8(status_6502 & ~FLAG_BREAK);
        status_6502 |= FLAG_INTERRUPT;
        pc_6502 = mem_6502_read16(0xfffe);
    }
}

uint8 callexternal = 0;
void (*loopexternal)();

void exec6502(uint32 tickcount) {
    clockgoal6502 += tickcount;
    while (clockticks6502 < clockgoal6502) {
        opcode6502 = read6502(pc_6502++);
        status_6502 |= FLAG_CONSTANT;
        penaltyop = 0;
        penaltyaddr = 0;
       	(*addrtable[opcode6502])();
        (*optable[opcode6502])();
        clockticks6502 += ticktable[opcode6502];
        if (penaltyop && penaltyaddr) clockticks6502++;
        instructions6502++;
        if (callexternal) (*loopexternal)();
    }

}

void step6502() {
    opcode6502 = read6502(pc_6502++);
    status_6502 |= FLAG_CONSTANT;

    penaltyop = 0;
    penaltyaddr = 0;

    (*addrtable[opcode6502])();
    (*optable[opcode6502])();
    clockticks6502 += ticktable[opcode6502];
    /*The following line goes commented out in Mike Chamber's usage of the 6502 emulator for MOARNES*/
    if (penaltyop && penaltyaddr) clockticks6502++;
    clockgoal6502 = clockticks6502;

    instructions6502++;

    if (callexternal) (*loopexternal)();
}

void hookexternal(void *funcptr) {
    if (funcptr != (void *)NULL) {
        loopexternal = funcptr;
        callexternal = 1;
    } else callexternal = 0;
}
