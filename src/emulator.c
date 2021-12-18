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

/*---------------- Helpers ----------------*/
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

/*-------- Arithmetic Codes/Flags Calculations --------*/
void calculate_codes_z(State8080* state, uint8_t result) {
    state->codes.z = (result & 0xff) == 0;
}

void calculate_codes_s(State8080* state, uint8_t result) {
    state->codes.s = (result & 0x80) != 0;
}

void calculate_codes_p(State8080* state, uint8_t result) {
    state->codes.p = __builtin_parity(result);
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

/*------------------------ Errors ------------------------*/
void unimplemented_op_error(State8080* state) {
    printf("Error: Unimplemented operation at 0x%04x (opcode: 0x%02x)\n", 
        state->pc, state->memory[state->pc]);
    exit(1);
}

/*--------------- Arithmetic Instructions ---------------*/
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
    uint16_t hl_addr = ((uint16_t)(state->h) << 8) | (uint16_t)(state->l);
    uint32_t result = (uint32_t)state->memory[hl_addr] + (uint32_t)value;
    state->memory[hl_addr] = result & 0xffff;
    state->codes.cy = result > 0xffff;
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

/*--------------------- Logical Instructions ---------------------*/
void rrc(State8080* state) {
    state->codes.cy = state->a & 1;
    state->a = ((state->a & 1) << 7) | (state->a >> 1);    
}

/*----------------- Data Transfer Instructions -----------------*/
void mvi(State8080* state, uint8_t* reg, uint8_t value) {
    *reg = value;
    state->pc++;
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

/*----------------- Branch Instructions -----------------*/
void jmp(State8080* state, unsigned char* opcode) {
    // Combine the next 2 bytes into an address and assign the program counter to it.
    state->pc = combine_immediates(opcode[2], opcode[1]);
    state->pc += 2;
}

void call(State8080* state, unsigned char* opcode) {
    // The return address is the next instruction. Since CALL is a 3 byte instruction, the next
    // instruction is 2 bytes away (remember state->pc is incremented after each operation, which
    // is why we're adding 2 instead of 3 to get to the next instruction.)
    uint16_t ret_address = state->pc + 2;

    // Push the address of the next instruction onto the stack
    state->memory[state->sp - 1] = (ret_address >> 8) & 0xff;
    state->memory[state->sp - 2] = ret_address & 0xff;
    state->sp = state->sp - 2;

    // Jump to the address
    jmp(state, opcode);
}

void ret(State8080* state) {
    // Assign the program counter to the address at the top of the stack, then move the stack
    // pointer back down to "pop" it.
    printf("SP: 0x%04x, SP+1: 0x%04x\n", state->sp, state->sp + 1);
    state->pc = combine_immediates(state->memory[state->sp], state->memory[state->sp + 1]);
    state->sp += 2;
}

/*-------------------------------------------------------*/

void emulate_op(State8080* state) {
    uint8_t* opcode = &state->memory[state->pc];
    uint16_t addr_offset;

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
            addr_offset = combine_immediates(state->b, state->c);
            dad(state, state->memory[addr_offset]);
            break;
        case 0x0a: ldax(state, state->b, state->c); break;  // LDAX B
        case 0x0b: unimplemented_op_error(state); break;
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
            addr_offset = combine_immediates(state->d, state->e);
            dad(state, state->memory[addr_offset]);
            break;
        case 0x1a: ldax(state, state->d, state->e); break;  // LDAX D
        case 0x1b: unimplemented_op_error(state); break;
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
            addr_offset = combine_immediates(state->h, state->l);
            dad(state, state->memory[addr_offset]);
            break;
        case 0x2a: unimplemented_op_error(state); break;
        case 0x2b: unimplemented_op_error(state); break;
        case 0x2c: inr(state, &state->l); break;            // INR L
        case 0x2d: dcr(state, &state->l); break;            // DCR L
        case 0x2e: mvi(state, &state->l, opcode[1]); break; // MVI L,1-byte-imeddiate
        case 0x2f: unimplemented_op_error(state); break;

        case 0x30: unimplemented_op_error(state); break;
        case 0x31:                                          // LXI SP,2-byte-immediate
            lxi(state, (uint8_t*)&state->sp, (uint8_t*)(&state->sp + 1), opcode); 
            break;
        case 0x32:                                          // STA 2-byte-immediate
            stax(state, opcode[2], opcode[1]);
            state->pc += 2;
            break;
        case 0x33: state->memory[state->pc]++; break;       // INX SP
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
            dad(state, state->memory[state->sp]);
            break;
        case 0x3a:                                          // LDAX D
             ldax(state, opcode[2], opcode[1]);
             state->pc += 2;
             break;
        case 0x3b: unimplemented_op_error(state); break;
        case 0x3c: inr(state, &state->a); break;            // INR A
        case 0x3d: dcr(state, &state->a); break;            // DCR A
        case 0x3e: mvi(state, &state->a, opcode[1]); break; // MVI A,1-byte-imeddiate
        case 0x3f: unimplemented_op_error(state); break;

        case 0x40: unimplemented_op_error(state); break;
        case 0x41: unimplemented_op_error(state); break;
        case 0x42: unimplemented_op_error(state); break;
        case 0x43: unimplemented_op_error(state); break;
        case 0x44: unimplemented_op_error(state); break;
        case 0x45: unimplemented_op_error(state); break;
        case 0x46: unimplemented_op_error(state); break;
        case 0x47: unimplemented_op_error(state); break;
        case 0x48: unimplemented_op_error(state); break;
        case 0x49: unimplemented_op_error(state); break;
        case 0x4a: unimplemented_op_error(state); break;
        case 0x4b: unimplemented_op_error(state); break;
        case 0x4c: unimplemented_op_error(state); break;
        case 0x4d: unimplemented_op_error(state); break;
        case 0x4e: unimplemented_op_error(state); break;
        case 0x4f: unimplemented_op_error(state); break;

        case 0x50: unimplemented_op_error(state); break;
        case 0x51: unimplemented_op_error(state); break;
        case 0x52: unimplemented_op_error(state); break;
        case 0x53: unimplemented_op_error(state); break;
        case 0x54: unimplemented_op_error(state); break;
        case 0x55: unimplemented_op_error(state); break;
        case 0x56: unimplemented_op_error(state); break;
        case 0x57: unimplemented_op_error(state); break;
        case 0x58: unimplemented_op_error(state); break;
        case 0x59: unimplemented_op_error(state); break;
        case 0x5a: unimplemented_op_error(state); break;
        case 0x5b: unimplemented_op_error(state); break;
        case 0x5c: unimplemented_op_error(state); break;
        case 0x5d: unimplemented_op_error(state); break;
        case 0x5e: unimplemented_op_error(state); break;
        case 0x5f: unimplemented_op_error(state); break;

        case 0x60: unimplemented_op_error(state); break;
        case 0x61: unimplemented_op_error(state); break;
        case 0x62: unimplemented_op_error(state); break;
        case 0x63: unimplemented_op_error(state); break;
        case 0x64: unimplemented_op_error(state); break;
        case 0x65: unimplemented_op_error(state); break;
        case 0x66: unimplemented_op_error(state); break;
        case 0x67: unimplemented_op_error(state); break;
        case 0x68: unimplemented_op_error(state); break;
        case 0x69: unimplemented_op_error(state); break;
        case 0x6a: unimplemented_op_error(state); break;
        case 0x6b: unimplemented_op_error(state); break;
        case 0x6c: unimplemented_op_error(state); break;
        case 0x6d: unimplemented_op_error(state); break;
        case 0x6e: unimplemented_op_error(state); break;
        case 0x6f: unimplemented_op_error(state); break;

        case 0x70: unimplemented_op_error(state); break;
        case 0x71: unimplemented_op_error(state); break;
        case 0x72: unimplemented_op_error(state); break;
        case 0x73: unimplemented_op_error(state); break;
        case 0x74: unimplemented_op_error(state); break;
        case 0x75: unimplemented_op_error(state); break;
        case 0x76: unimplemented_op_error(state); break;
        case 0x77: unimplemented_op_error(state); break;
        case 0x78: unimplemented_op_error(state); break;
        case 0x79: unimplemented_op_error(state); break;
        case 0x7a: unimplemented_op_error(state); break;
        case 0x7b: unimplemented_op_error(state); break;
        case 0x7c: unimplemented_op_error(state); break;
        case 0x7d: unimplemented_op_error(state); break;
        case 0x7e: unimplemented_op_error(state); break;
        case 0x7f: unimplemented_op_error(state); break;

        case 0x80: add(state, state->b); break;     // ADD B
        case 0x81: add(state, state->c); break;     // ADD C
        case 0x82: add(state, state->d); break;     // ADD D
        case 0x83: add(state, state->e); break;     // ADD E
        case 0x84: add(state, state->h); break;     // ADD H
        case 0x85: add(state, state->l); break;     // ADD L
        case 0x86:                                  // ADD M
            addr_offset = ((uint16_t)(state->h) << 8) | (uint16_t)(state->l);
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
            addr_offset = ((uint16_t)(state->h) << 8) | (uint16_t)(state->l);
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
            addr_offset = ((uint16_t)(state->h) << 8) | (uint16_t)(state->l);
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
            addr_offset = ((uint16_t)(state->h) << 8) | (uint16_t)(state->l);
            sbb(state, state->memory[addr_offset]);
            break;
        case 0x9f: sbb(state, state->a); break;     // SBB A

        case 0xa0: unimplemented_op_error(state); break;
        case 0xa1: unimplemented_op_error(state); break;
        case 0xa2: unimplemented_op_error(state); break;
        case 0xa3: unimplemented_op_error(state); break;
        case 0xa4: unimplemented_op_error(state); break;
        case 0xa5: unimplemented_op_error(state); break;
        case 0xa6: unimplemented_op_error(state); break;
        case 0xa7: unimplemented_op_error(state); break;
        case 0xa8: unimplemented_op_error(state); break;
        case 0xa9: unimplemented_op_error(state); break;
        case 0xaa: unimplemented_op_error(state); break;
        case 0xab: unimplemented_op_error(state); break;
        case 0xac: unimplemented_op_error(state); break;
        case 0xad: unimplemented_op_error(state); break;
        case 0xae: unimplemented_op_error(state); break;
        case 0xaf: unimplemented_op_error(state); break;

        case 0xb0: unimplemented_op_error(state); break;
        case 0xb1: unimplemented_op_error(state); break;
        case 0xb2: unimplemented_op_error(state); break;
        case 0xb3: unimplemented_op_error(state); break;
        case 0xb4: unimplemented_op_error(state); break;
        case 0xb5: unimplemented_op_error(state); break;
        case 0xb6: unimplemented_op_error(state); break;
        case 0xb7: unimplemented_op_error(state); break;
        case 0xb8: unimplemented_op_error(state); break;
        case 0xb9: unimplemented_op_error(state); break;
        case 0xba: unimplemented_op_error(state); break;
        case 0xbb: unimplemented_op_error(state); break;
        case 0xbc: unimplemented_op_error(state); break;
        case 0xbd: unimplemented_op_error(state); break;
        case 0xbe: unimplemented_op_error(state); break;
        case 0xbf: unimplemented_op_error(state); break;

        case 0xc0: unimplemented_op_error(state); break;
        case 0xc1: unimplemented_op_error(state); break;
        case 0xc2:                                              // JNZ address
            if (state->codes.z != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xc3: jmp(state, opcode); break;                   // JMP address
        case 0xc4: unimplemented_op_error(state); break;
        case 0xc5: unimplemented_op_error(state); break;
        case 0xc6: add(state, opcode[1]); state->pc++; break;   // ADI 1-byte-immediate
        case 0xc7: unimplemented_op_error(state); break;
        case 0xc8: unimplemented_op_error(state); break;
        case 0xc9: ret(state); break;
        case 0xca:                                              // JZ address
            if (state->codes.z == 0) {
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
        case 0xd1: unimplemented_op_error(state); break;
        case 0xd2:                                              // JNC address
            if (state->codes.cy == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xd3: unimplemented_op_error(state); break;
        case 0xd4: unimplemented_op_error(state); break;
        case 0xd5: unimplemented_op_error(state); break;
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
        case 0xe1: unimplemented_op_error(state); break;
        case 0xe2:                                  // JPO address
            if (state->codes.p == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xe3: unimplemented_op_error(state); break;
        case 0xe4: unimplemented_op_error(state); break;
        case 0xe5: unimplemented_op_error(state); break;
        case 0xe6: unimplemented_op_error(state); break;
        case 0xe7: unimplemented_op_error(state); break;
        case 0xe8: unimplemented_op_error(state); break;
        case 0xe9: unimplemented_op_error(state); break;
        case 0xea:                                  // JPE address
            if (state->codes.p != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xeb: unimplemented_op_error(state); break;
        case 0xec: unimplemented_op_error(state); break;
        case 0xed: unimplemented_op_error(state); break;
        case 0xee: unimplemented_op_error(state); break;
        case 0xef: unimplemented_op_error(state); break;

        case 0xf0: unimplemented_op_error(state); break;
        case 0xf1: unimplemented_op_error(state); break;
        case 0xf2:                                  // JP address
            if (state->codes.s == 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xf3: unimplemented_op_error(state); break;
        case 0xf4: unimplemented_op_error(state); break;
        case 0xf5: unimplemented_op_error(state); break;
        case 0xf6: unimplemented_op_error(state); break;
        case 0xf7: unimplemented_op_error(state); break;
        case 0xf8: unimplemented_op_error(state); break;
        case 0xf9: unimplemented_op_error(state); break;
        case 0xfa:                                  // JM address
            if (state->codes.s != 0) {
                jmp(state, opcode);
            }
            else {
                state->pc += 2;
            }
            break;
        case 0xfb: unimplemented_op_error(state); break;
        case 0xfc: unimplemented_op_error(state); break;
        case 0xfd: unimplemented_op_error(state); break;
        case 0xfe: unimplemented_op_error(state); break;
        case 0xff: unimplemented_op_error(state); break;

        default: unimplemented_op_error(state); break;
    }

    state->pc += 1;
}

void print_state(State8080* state) {
    printf("{a: %u, b: %u, c: %u, d: %u, e: %u, h: %u, l: %u, sp: 0x%04x, pc: 0x%04x}\n", 
        state->a, state->b, state->c, state->d, state->e, state->h, state->l, state->sp, state->pc);
}

int main(int argc, char** argv) {
    // Open the file and verify it's valid
    FILE *file = fopen(argv[1], "rb");

    if (file == NULL) {
        printf("Error: Could not open %s\n", argv[1]);
        exit(1);
    }

    // Get the file size and read it into a memory buffer
    fseek(file, 0L, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    unsigned char* buffer = malloc(file_size);
    fread(buffer, file_size, 1, file);
    fclose(file);

    // Initialize state with ROM as the memory buffer
    ConditionCodes codes = {0, 0, 0, 0, 0};
    State8080 state = {0, 0, 0, 0, 0, 0, 0, 0, 0, buffer, codes, 0};
    
    printf("Init -- ");
    print_state(&state);

    // Read through the buffer and emulate each operation.
    unsigned int opcounter = 0;
    while(state.pc < file_size) {
        uint8_t cur_op = state.memory[state.pc];
        printf("%04u -- 0x%02x -> ", opcounter, cur_op);
        emulate_op(&state);
        printf("State: ");
        print_state(&state);

        opcounter++;
    }

    return 0;
}