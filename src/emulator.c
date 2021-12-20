#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ConditionCodes {
    uint8_t z : 1;
    uint8_t s : 1;
    uint8_t p : 1;
    uint8_t cy : 1;
    uint8_t ac : 1;
} ConditionCodes;

typedef struct State8080 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t* memory;
    struct ConditionCodes codes;
    uint8_t int_enable;
} State8080;

#pragma region Helpers

/**
 * @brief Combine two 1-byte immediates into a 2-byte immediate. Useful for register pairs.
 * 
 * @param a 1-byte immediate
 * @param b 1-byte immediate
 * @return uint16_t Resulting 2-byte immediate
 */
uint16_t combine_immediates(uint8_t a, uint8_t b) {
    return ((uint16_t)a << 8) | (uint16_t)b;
}

/**
 * @brief Prints the codes/flags for an 8080 state
 * 
 * @param state 
 */
void print_codes(State8080* state) {
    printf("Codes {z: %u, s: %u, p: %u, cy: %u, ac: %u}\n", 
        state->codes.z,
        state->codes.s,
        state->codes.p,
        state->codes.cy,
        state->codes.ac);
}

/**
 * @brief Prints a state
 * 
 * @param state 
 */
void print_state(State8080* state) {
    printf("State {a: 0x%02x, bc: 0x%04x, de: 0x%04x, hl: 0x%04x, pc: 0x%04x, sp: 0x%04x}\n\t\t", 
        state->a,
        combine_immediates(state->b, state->c),
        combine_immediates(state->d, state->e),
        combine_immediates(state->h, state->l),
        state->pc, 
        state->sp);

    print_codes(state);
}

/**
 * @brief Shuts down the emualator
 * 
 * @param state 
 */
void shutdown(State8080* state) {
    printf("\nProgram Finished.\nFinal State -> ");
    print_state(state);
    exit(0);
}

#pragma endregion

#pragma region Arithmetic Codes/Flags Calculations

void calculate_codes_z(State8080* state, uint8_t result) {
    state->codes.z = (result & 0xff) == 0;
}

void calculate_codes_s(State8080* state, uint8_t result) {
    state->codes.s = (result & 0x80) != 0;
}

void calculate_codes_p(State8080* state, uint8_t result) {
    const uint8_t RESULT_BITS = 8;

    int parity = 0;
    int i;
    for (i = 0; i < RESULT_BITS; i++) {
        parity += result & 1;
        result = result >> 1;
    }

    state->codes.p = parity % 2 == 0;
}

void calculate_codes_cy(State8080* state, uint8_t result) {
    state->codes.cy = result > 0xff;
}

void calculate_codes_ac(State8080* state, uint8_t result) {
    state->codes.ac = result > 0x09;
}

void calculate_codes_all(State8080* state, uint8_t result) {
    calculate_codes_z(state, result);
    calculate_codes_s(state, result);
    calculate_codes_p(state, result);
    calculate_codes_cy(state, result);
    calculate_codes_ac(state, result);
}

void calculate_codes_all_except_cy(State8080* state, uint8_t result) {
    calculate_codes_z(state, result);
    calculate_codes_s(state, result);
    calculate_codes_p(state, result);
    calculate_codes_ac(state, result);
}

#pragma endregion

#pragma region Errors

void unimplemented_op_error(State8080* state) {
    printf("\nError: Unimplemented operation at 0x%04x (opcode: 0x%02x)\n", 
        state->pc, state->memory[state->pc]);
    exit(1);
}

#pragma endregion

#pragma region Arithmetic Operations

void add(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)state->a + (uint16_t)value;
    calculate_codes_all(state, result);
    state->a = result & 0xff;
}

void adc(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)state->a + (uint16_t)value + (uint16_t)state->codes.cy;
    calculate_codes_all(state, result);
    state->a = result & 0xff;
}

void dad(State8080* state, uint16_t value) {
    uint32_t hl = combine_immediates(state->h, state->l);
    uint32_t result = hl + value;

    state->codes.cy = result > 0xffff;
    state->h = (result & 0xff00) >> 8;
    state->l = result & 0xff;
}

void sub(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)state->a - (uint16_t)value;
    calculate_codes_all(state, result);
    state->a = result & 0xff;
}

void sbb(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)state->a - (uint16_t)value - (uint16_t)state->codes.cy;
    calculate_codes_all(state, result);
    state->a = result & 0xff;
}

void inr(State8080* state, uint8_t* reg) {
    (*reg)++;
    calculate_codes_all_except_cy(state, *reg);
}

void dcr(State8080* state, uint8_t* reg) {
    (*reg)--;
    calculate_codes_all_except_cy(state, *reg);
}

void inx(State8080* state, uint8_t* reg_1, uint8_t* reg_2) {
    (*reg_2)++;
    if (*reg_2 == 0) {
        (*reg_1)++;
    }
}

void dcx(State8080* state, uint8_t* reg_1, uint8_t* reg_2) {
    (*reg_2)--;
    if (*reg_2 == 0xff) {
        (*reg_1)--;
    }
}

void cpi(State8080* state, uint8_t value) {
    uint8_t result = state->a - value;
    calculate_codes_all(state, result);

    state->pc++;
}

#pragma endregion

#pragma region Logical and Bitwise Operations

void rrc(State8080* state) {
    state->codes.cy = state->a & 1;
    state->a = ((state->a & 1) << 7) | (state->a >> 1);    
}

void ana(State8080* state, uint8_t value) {
    state->a = state->a & value;
    calculate_codes_all(state, state->a);
}

void xra(State8080* state, uint8_t value) {
    state->a = state->a ^ value;
    calculate_codes_all(state, state->a);
}

void ora(State8080* state, uint8_t value) {
    state->a = state->a | value;
    calculate_codes_all(state, state->a);
}

#pragma endregion

#pragma region Data Transfer Operations

void mvi(State8080* state, uint8_t* reg, uint8_t value) {
    *reg = value;
    state->pc++;
}

void mov(State8080* state, uint8_t* move_to_reg, uint8_t* move_from_reg) {
    *move_to_reg = *move_from_reg;
}

void lxi(State8080* state, uint8_t* reg_1, uint8_t* reg_2, uint8_t* opcode) {
    *reg_1 = opcode[2];
    *reg_2 = opcode[1];
    state->pc += 2;
}

void ldax(State8080* state, uint8_t reg_1_val, uint8_t reg_2_val) {
    uint16_t reg_pair_addr = combine_immediates(reg_1_val, reg_2_val);
    state->a = state->memory[reg_pair_addr];
}

void stax(State8080* state, uint8_t reg_1_val, uint8_t reg_2_val) {
    uint16_t addr = combine_immediates(reg_1_val, reg_2_val);
    state->memory[addr] = state->a;
}

#pragma endregion

#pragma region Branch Operations

void jmp(State8080* state, unsigned char* opcode) {
    // Combine the next 2 bytes into an address and assign the program counter to it.
    state->pc = combine_immediates(opcode[2], opcode[1]);
    state->pc -= 1;
}

void call(State8080* state, unsigned char* opcode) {
    // The return address is the next instruction. Since CALL is a 3 byte instruction, the next
    // instruction is 2 bytes away (remember state->pc is incremented after each operation, which
    // is why we're adding 2 instead of 3 to get to the next instruction.)
    uint16_t ret_address = state->pc + 2;

    // Push the address of the next instruction onto the stack
    state->memory[state->sp - 1] = (ret_address >> 8) & 0xff;
    state->memory[state->sp - 2] = ret_address & 0xff;
    state->sp -= 2;

    // Jump to the address
    jmp(state, opcode);
}

void ret(State8080* state) {
    // Assign the program counter to the address at the top of the stack, then move the stack
    // pointer back down to "pop" it.
    state->pc = combine_immediates(state->memory[state->sp + 1], state->memory[state->sp]);
    state->sp += 2;
}

#pragma endregion

#pragma region Stack Operations

void push(State8080* state, uint8_t* reg_1, uint8_t* reg_2) {
    state->memory[state->sp - 1] = *reg_1;
    state->memory[state->sp - 2] = *reg_2;
    state->sp -= 2;
}

void pop(State8080* state, uint8_t* reg_1, uint8_t* reg_2) {
    *reg_1 = state->memory[state->sp + 1];
    *reg_2 = state->memory[state->sp];
    state->sp += 2;
}

#pragma endregion

#pragma region Main Operation Emulation Switch Case

void emulate_op(State8080* state) {
    uint8_t* opcode = &state->memory[state->pc];
    printf("0x%02x -> ", *opcode);

    uint16_t addr_offset;
    uint16_t value;

    switch(*opcode) {
        case 0x00: break;                                   // NOP
        case 0x01:                                          // LXI B,2-byte-immediate
            lxi(state, &state->b, &state->c, opcode); 
            break;
        case 0x02: unimplemented_op_error(state); break;
        case 0x03: inx(state, &state->b, &state->c); break; // INX B
        case 0x04: inr(state, &state->b); break;            // INR B
        case 0x05: dcr(state, &state->b); break;            // DCR B
        case 0x06: mvi(state, &state->b, opcode[1]); break; // MVI B,1-byte-imeddiate
        case 0x07: unimplemented_op_error(state); break;
        case 0x08: unimplemented_op_error(state); break;
        case 0x09:                                          // DAD B
            value = combine_immediates(state->b, state->c);
            dad(state, value);
            break;
        case 0x0a: ldax(state, state->b, state->c); break;  // LDAX B
        case 0x0b: dcx(state, &state->b, &state->c); break; // DCX B
        case 0x0c: inr(state, &state->c); break;            // INR C
        case 0x0d: dcr(state, &state->c); break;            // DCR C
        case 0x0e: mvi(state, &state->c, opcode[1]); break; // MVI C,1-byte-imeddiate
        case 0x0f: rrc(state); break;                       // RRC

        case 0x10: unimplemented_op_error(state); break;
        case 0x11:                                          // LXI D,2-byte-immediate
            lxi(state, &state->d, &state->e, opcode); 
            break;
        case 0x12: unimplemented_op_error(state); break;
        case 0x13: inx(state, &state->d, &state->e); break; // INX D
        case 0x14: inr(state, &state->d); break;            // INR D
        case 0x15: dcr(state, &state->d); break;            // DCR D
        case 0x16: mvi(state, &state->d, opcode[1]); break; // MVI D,1-byte-imeddiate
        case 0x17: unimplemented_op_error(state); break;
        case 0x18: unimplemented_op_error(state); break;
        case 0x19:                                          // DAD D
            value = combine_immediates(state->d, state->e);
            dad(state, value);
            break;
        case 0x1a: ldax(state, state->d, state->e); break;  // LDAX D
        case 0x1b: dcx(state, &state->d, &state->e); break; // DCX D
        case 0x1c: inr(state, &state->e); break;            // INR E
        case 0x1d: dcr(state, &state->e); break;            // DCR E
        case 0x1e: mvi(state, &state->e, opcode[1]); break; // MVI E,1-byte-imeddiate
        case 0x1f: unimplemented_op_error(state); break;

        case 0x20: unimplemented_op_error(state); break;
        case 0x21:                                          // LXI H,2-byte-immediate
            lxi(state, &state->h, &state->l, opcode); 
            break;
        case 0x22: unimplemented_op_error(state); break;
        case 0x23: inx(state, &state->h, &state->l); break;   // INX H
        case 0x24: inr(state, &state->h); break;            // INR H
        case 0x25: dcr(state, &state->h); break;            // DCR H
        case 0x26: mvi(state, &state->h, opcode[1]); break; // MVI H,1-byte-imeddiate
        case 0x27: unimplemented_op_error(state); break;
        case 0x28: unimplemented_op_error(state); break;
        case 0x29:                                          // DAD H
            value = combine_immediates(state->h, state->l);
            dad(state, value);
            break;
        case 0x2a: unimplemented_op_error(state); break;
        case 0x2b: dcx(state, &state->h, &state->l); break; // DCX H
        case 0x2c: inr(state, &state->l); break;            // INR L
        case 0x2d: dcr(state, &state->l); break;            // DCR L
        case 0x2e: mvi(state, &state->l, opcode[1]); break; // MVI L,1-byte-imeddiate
        case 0x2f: unimplemented_op_error(state); break;

        case 0x30: unimplemented_op_error(state); break;
        case 0x31:                                          // LXI SP,2-byte-immediate
            state->sp = combine_immediates(opcode[2], opcode[1]);
            state->pc += 2;
            break;
        case 0x32:                                          // STA 2-byte-immediate
            stax(state, opcode[2], opcode[1]);
            state->pc += 2;
            break;
        case 0x33: state->sp++; break;       // INX SP
        case 0x34:                                          // INR M
            addr_offset = combine_immediates(state->h, state->l);
            inr(state, &state->memory[addr_offset]);
            break;
        case 0x35:                                          // DCR M
            addr_offset = combine_immediates(state->h, state->l);
            dcr(state, &state->memory[addr_offset]);
            break;
        case 0x36:                                          // MVI M,1-byte-imeddiate
            addr_offset = combine_immediates(state->h, state->l);
            mvi(state, &state->memory[addr_offset], opcode[1]);
            break;
        case 0x37: unimplemented_op_error(state); break;
        case 0x38: unimplemented_op_error(state); break;
        case 0x39:                                          // DAD SP
            dad(state, state->sp);
            break;
        case 0x3a:                                          // LDAX 2-byte-immediate
             ldax(state, opcode[2], opcode[1]);
             state->pc += 2;
             break;
        case 0x3b: state->sp--; break;                      // DCX B
        case 0x3c: inr(state, &state->a); break;            // INR A
        case 0x3d: dcr(state, &state->a); break;            // DCR A
        case 0x3e: mvi(state, &state->a, opcode[1]); break; // MVI A,1-byte-imeddiate
        case 0x3f: unimplemented_op_error(state); break;

        case 0x40: mov(state, &state->b, &state->b); break; // MOV B,B
        case 0x41: mov(state, &state->b, &state->c); break; // MOV B,C
        case 0x42: mov(state, &state->b, &state->d); break; // MOV B,D
        case 0x43: mov(state, &state->b, &state->e); break; // MOV B,E
        case 0x44: mov(state, &state->b, &state->h); break; // MOV B,H
        case 0x45: mov(state, &state->b, &state->l); break; // MOV B,L
        case 0x46:                                          // MOV B,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->b, &state->memory[addr_offset]);
            break;
        case 0x47: mov(state, &state->b, &state->a); break; // MOV B,A
        case 0x48: mov(state, &state->c, &state->b); break; // MOV C,B
        case 0x49: mov(state, &state->c, &state->c); break; // MOV C,C
        case 0x4a: mov(state, &state->c, &state->d); break; // MOV C,D
        case 0x4b: mov(state, &state->c, &state->e); break; // MOV C,E
        case 0x4c: mov(state, &state->c, &state->h); break; // MOV C,H
        case 0x4d: mov(state, &state->c, &state->l); break; // MOV C,L
        case 0x4e:                                          // MOV C,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->c, &state->memory[addr_offset]);
            break;
        case 0x4f: mov(state, &state->c, &state->a); break; // MOV C,A

        case 0x50: mov(state, &state->d, &state->b); break; // MOV D,B
        case 0x51: mov(state, &state->d, &state->c); break; // MOV D,C
        case 0x52: mov(state, &state->d, &state->d); break; // MOV D,D
        case 0x53: mov(state, &state->d, &state->e); break; // MOV D,E
        case 0x54: mov(state, &state->d, &state->h); break; // MOV D,H
        case 0x55: mov(state, &state->d, &state->l); break; // MOV D,L
        case 0x56:                                          // MOV D,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->d, &state->memory[addr_offset]);
            break;
        case 0x57: mov(state, &state->d, &state->a); break; // MOV D,A
        case 0x58: mov(state, &state->e, &state->b); break; // MOV E,B
        case 0x59: mov(state, &state->e, &state->c); break; // MOV E,C
        case 0x5a: mov(state, &state->e, &state->d); break; // MOV E,D
        case 0x5b: mov(state, &state->e, &state->e); break; // MOV E,E
        case 0x5c: mov(state, &state->e, &state->h); break; // MOV E,H
        case 0x5d: mov(state, &state->e, &state->l); break; // MOV E,L
        case 0x5e:                                          // MOV E,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->e, &state->memory[addr_offset]);
            break;
        case 0x5f: mov(state, &state->e, &state->a); break; // MOV E,A

        case 0x60: mov(state, &state->h, &state->b); break; // MOV H,B
        case 0x61: mov(state, &state->h, &state->c); break; // MOV H,C
        case 0x62: mov(state, &state->h, &state->d); break; // MOV H,D
        case 0x63: mov(state, &state->h, &state->e); break; // MOV H,E
        case 0x64: mov(state, &state->h, &state->h); break; // MOV H,H
        case 0x65: mov(state, &state->h, &state->l); break; // MOV H,L
        case 0x66:                                          // MOV H,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->h, &state->memory[addr_offset]);
            break;
        case 0x67: mov(state, &state->h, &state->a); break; // MOV H,A
        case 0x68: mov(state, &state->l, &state->b); break; // MOV L,B
        case 0x69: mov(state, &state->l, &state->c); break; // MOV L,C
        case 0x6a: mov(state, &state->l, &state->d); break; // MOV L,D
        case 0x6b: mov(state, &state->l, &state->e); break; // MOV L,E
        case 0x6c: mov(state, &state->l, &state->h); break; // MOV L,H
        case 0x6d: mov(state, &state->l, &state->l); break; // MOV L,L
        case 0x6e:                                          // MOV L,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->l, &state->memory[addr_offset]);
            break;
        case 0x6f: mov(state, &state->l, &state->a); break; // MOV L,A

        case 0x70:                                          // MOV M,B
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->b);
            break;
        case 0x71:                                          // MOV M,C
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->c);
            break;
        case 0x72:                                          // MOV M,D
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->d);
            break;
        case 0x73:                                          // MOV M,E
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->e);
            break;
        case 0x74:                                          // MOV M,H
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->h);
            break;
        case 0x75:                                          // MOV M,L
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->l);
            break;
        case 0x76: unimplemented_op_error(state); break;
        case 0x77:                                          // MOV M,A
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->memory[addr_offset], &state->a);
            break;
        case 0x78: mov(state, &state->a, &state->b); break; // MOV A,B
        case 0x79: mov(state, &state->a, &state->c); break; // MOV A,C
        case 0x7a: mov(state, &state->a, &state->d); break; // MOV A,D
        case 0x7b: mov(state, &state->a, &state->e); break; // MOV A,E
        case 0x7c: mov(state, &state->a, &state->h); break; // MOV A,H
        case 0x7d: mov(state, &state->a, &state->l); break; // MOV A,L
        case 0x7e:                                          // MOV A,M
            addr_offset = combine_immediates(state->h, state->l);
            mov(state, &state->a, &state->memory[addr_offset]);
            break;
        case 0x7f: mov(state, &state->a, &state->a); break; // MOV A,A

        case 0x80: add(state, state->b); break;     // ADD B
        case 0x81: add(state, state->c); break;     // ADD C
        case 0x82: add(state, state->d); break;     // ADD D
        case 0x83: add(state, state->e); break;     // ADD E
        case 0x84: add(state, state->h); break;     // ADD H
        case 0x85: add(state, state->l); break;     // ADD L
        case 0x86:                                  // ADD M
            addr_offset = combine_immediates(state->h, state->l);
            add(state, state->memory[addr_offset]);
            break;
        case 0x87: add(state, state->a); break;     // ADD A
        case 0x88: adc(state, state->b); break;     // ADC B
        case 0x89: adc(state, state->c); break;     // ADC C
        case 0x8a: adc(state, state->d); break;     // ADC D
        case 0x8b: adc(state, state->e); break;     // ADC E
        case 0x8c: adc(state, state->h); break;     // ADC H
        case 0x8d: adc(state, state->l); break;     // ADC L
        case 0x8e:                                  // ADC M
            addr_offset = combine_immediates(state->h, state->l);
            adc(state, state->memory[addr_offset]);
            break;
        case 0x8f: adc(state, state->a); break;     // ADC A

        case 0x90: sub(state, state->b); break;     // SUB B
        case 0x91: sub(state, state->c); break;     // SUB C
        case 0x92: sub(state, state->d); break;     // SUB D
        case 0x93: sub(state, state->e); break;     // SUB E
        case 0x94: sub(state, state->h); break;     // SUB H
        case 0x95: sub(state, state->l); break;     // SUB L
        case 0x96:                                  // SUB M
            addr_offset = combine_immediates(state->h, state->l);
            sub(state, state->memory[addr_offset]);
            break;
        case 0x97: sub(state, state->a); break;     // SUB A
        case 0x98: sbb(state, state->b); break;     // SBB B
        case 0x99: sbb(state, state->c); break;     // SBB C
        case 0x9a: sbb(state, state->d); break;     // SBB D
        case 0x9b: sbb(state, state->e); break;     // SBB E
        case 0x9c: sbb(state, state->h); break;     // SBB H
        case 0x9d: sbb(state, state->l); break;     // SBB L
        case 0x9e:                                  // SBB M
            addr_offset = combine_immediates(state->h, state->l);
            sbb(state, state->memory[addr_offset]);
            break;
        case 0x9f: sbb(state, state->a); break;     // SBB A

        case 0xa0: ana(state, state->b);            // ANA B
        case 0xa1: ana(state, state->c);            // ANA C
        case 0xa2: ana(state, state->d);            // ANA D
        case 0xa3: ana(state, state->e);            // ANA E
        case 0xa4: ana(state, state->h);            // ANA H
        case 0xa5: ana(state, state->l);            // ANA L
        case 0xa6:                                  // ANA M
            addr_offset = combine_immediates(state->h, state->l);
            ana(state, state->memory[addr_offset]);
            break;
        case 0xa7: ana(state, state->a);            // ANA A
        case 0xa8: xra(state, state->b);            // XRA B
        case 0xa9: xra(state, state->c);            // XRA C
        case 0xaa: xra(state, state->d);            // XRA D
        case 0xab: xra(state, state->e);            // XRA E
        case 0xac: xra(state, state->h);            // XRA H
        case 0xad: xra(state, state->l);            // XRA L
        case 0xae:                                  // XRA M
            addr_offset = combine_immediates(state->h, state->l);
            xra(state, state->memory[addr_offset]);
            break;
        case 0xaf: xra(state, state->a);            // XRA A

        case 0xb0: ora(state, state->b);            // ORA B
        case 0xb1: ora(state, state->c);            // ORA C
        case 0xb2: ora(state, state->d);            // ORA D
        case 0xb3: ora(state, state->e);            // ORA E
        case 0xb4: ora(state, state->h);            // ORA H
        case 0xb5: ora(state, state->l);            // ORA L
        case 0xb6:                                  // ORA M
            addr_offset = combine_immediates(state->h, state->l);
            ora(state, state->memory[addr_offset]);
            break;
        case 0xb7: ora(state, state->l);            // ORA L
        case 0xb8: unimplemented_op_error(state); break;
        case 0xb9: unimplemented_op_error(state); break;
        case 0xba: unimplemented_op_error(state); break;
        case 0xbb: unimplemented_op_error(state); break;
        case 0xbc: unimplemented_op_error(state); break;
        case 0xbd: unimplemented_op_error(state); break;
        case 0xbe: unimplemented_op_error(state); break;
        case 0xbf: unimplemented_op_error(state); break;

        case 0xc0: unimplemented_op_error(state); break;
        case 0xc1: pop(state, &state->b, &state->c); break;     // POP B
        case 0xc2:                                              // JNZ address
            if (state->codes.z == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xc3: jmp(state, opcode); break;                   // JMP address
        case 0xc4: unimplemented_op_error(state); break;
        case 0xc5: push(state, &state->b, &state->c); break;    // PUSH B
        case 0xc6: add(state, opcode[1]); state->pc++; break;   // ADI 1-byte-immediate
        case 0xc7: unimplemented_op_error(state); break;
        case 0xc8: unimplemented_op_error(state); break;
        case 0xc9: ret(state); break;                           // RET
        case 0xca:                                              // JZ address
            if (state->codes.z != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xcb: unimplemented_op_error(state); break;
        case 0xcc: unimplemented_op_error(state); break;
        case 0xcd: call(state, opcode); break;
        case 0xce: adc(state, opcode[1]); state->pc++; break;   // ACI 1-byte-immediate
        case 0xcf: unimplemented_op_error(state); break;

        case 0xd0: unimplemented_op_error(state); break;
        case 0xd1: pop(state, &state->d, &state->e); break;     // POP D
        case 0xd2:                                              // JNC address
            if (state->codes.cy == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xd3: /*TODO*/ state->pc++; break;                 // OUT 1-byte-immediate
        case 0xd4: unimplemented_op_error(state); break;
        case 0xd5: push(state, &state->d, &state->e); break;    // PUSH D
        case 0xd6: sub(state, opcode[1]); state->pc++; break;   // SUI 1-byte-immediate
        case 0xd7: unimplemented_op_error(state); break;
        case 0xd8: unimplemented_op_error(state); break;
        case 0xd9: unimplemented_op_error(state); break;
        case 0xda:                                              // JC address
            if (state->codes.cy != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xdb: unimplemented_op_error(state); break;
        case 0xdc: unimplemented_op_error(state); break;
        case 0xdd: unimplemented_op_error(state); break;
        case 0xde: unimplemented_op_error(state); break;
        case 0xdf: unimplemented_op_error(state); break;

        case 0xe0: unimplemented_op_error(state); break;
        case 0xe1: pop(state, &state->h, &state->l); break;     // POP H
        case 0xe2:                                              // JPO address
            if (state->codes.p == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xe3: unimplemented_op_error(state); break;
        case 0xe4: unimplemented_op_error(state); break;
        case 0xe5: push(state, &state->h, &state->l); break;    // PUSH H
        case 0xe6: ana(state, opcode[1]); state->pc++; break;   // ANA 1-byte-immediate
        case 0xe7: unimplemented_op_error(state); break;
        case 0xe8: unimplemented_op_error(state); break;
        case 0xe9: unimplemented_op_error(state); break;
        case 0xea:                                              // JPE address
            if (state->codes.p != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xeb:                                              // XCHG
            value = state->d;
            state->d = state->h;
            state->h = (uint8_t)value;
            value = state->e;
            state->e = state->l;
            state->l = (uint8_t)value;
            break;
        case 0xec: unimplemented_op_error(state); break;
        case 0xed: unimplemented_op_error(state); break;
        case 0xee: unimplemented_op_error(state); break;
        case 0xef: unimplemented_op_error(state); break;

        case 0xf0: unimplemented_op_error(state); break;
        case 0xf1:                                          // POP PSW
            value = state->memory[state->sp];
            state->codes.z = (value & 0b1) != 0;
            state->codes.s = (value & 0b10) != 0;
            state->codes.p = (value & 0b100) != 0;
            state->codes.cy = (value & 0b1000) != 0;
            state->codes.ac = (value & 0b10000) != 0;
            state->a = state->memory[state->sp + 1];
            state->sp += 2;
            break;
        case 0xf2:                                          // JP address
            if (state->codes.s == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xf3: unimplemented_op_error(state); break;
        case 0xf4: unimplemented_op_error(state); break;
        case 0xf5:                                          // PUSH PSW
            value = (state->codes.z |
                state->codes.s << 1 |
                state->codes.p << 2 |
                state->codes.cy << 3 |
                state->codes.ac << 4);
            push(state, &state->a, (uint8_t*)&value);
            break;
        case 0xf6: unimplemented_op_error(state); break;
        case 0xf7: unimplemented_op_error(state); break;
        case 0xf8: unimplemented_op_error(state); break;
        case 0xf9: unimplemented_op_error(state); break;
        case 0xfa:                                          // JM address
            if (state->codes.s != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xfb: state->int_enable = 1; break;            // EI
        case 0xfc: unimplemented_op_error(state); break;
        case 0xfd: unimplemented_op_error(state); break;
        case 0xfe: cpi(state, opcode[1]); break;            // CPI 1-byte-immediate
        case 0xff: unimplemented_op_error(state); break;

        default: unimplemented_op_error(state); break;
    }

    state->pc += 1;
}

#pragma endregion

#pragma region Emulator Initialization

/**
 * @brief Initializes an 8080 state with 64kb memory allocated.
 * 
 * @return State8080* 
 */
State8080* init_8080() {
    // Initializing 8080 state and allocating 64kb of memory
    State8080* state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000);
    return state;
}

/**
 * @brief Reads a binary file into a state's memory.
 * 
 * @param state The 8080 state
 * @param filename Path to the file
 * @param offset The memory offset where the beginning of the file will start
 * @return uint16_t The size of the file that was read
 */
uint16_t read_file_into_memory(State8080* state, char* filename, uint16_t offset) {
    // Open the file and verify it's valid
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        printf("\nError: Could not open %s\n", filename);
        exit(1);
    }

    // Get the file size and read it into the memory buffer
    fseek(file, 0L, SEEK_END);
    uint16_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    fread(&state->memory[offset], file_size, 1, file);
    fclose(file);

    // Set the program counter to the beginning of the rom
    state->pc = offset;

    return file_size;
}

#pragma endregion

/**
 * @brief Main method where program starts.
 * 
 * @param argc Number of arguments
 * @param argv Arguments
 * @return int Return code
 */
int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("Please provide a ROM file as an argument.");
        exit(1);
    }

    State8080* state = init_8080();
    uint16_t file_size = read_file_into_memory(state, argv[1], 0x100);
    
    printf("Init -- ");
    print_state(state);

    // Read through the buffer and emulate each operation.
    unsigned int opcounter = 0;
    while(state->pc < file_size) {
        uint8_t cur_op = state->memory[state->pc];
        printf("%04u -- ", opcounter);
        emulate_op(state);
        print_state(state);

        opcounter++;

        if (opcounter > 50000) {
            shutdown(state);
        }
    }

    shutdown(state);

    return 0;
}