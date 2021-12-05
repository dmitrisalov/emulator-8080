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
    uint8_t *memory;
    struct ConditionCodes codes;
    uint8_t int_enable;
} State8080;

void unimplemented_op_error(State8080* state) {
    printf("Error: Unimplemented operation at 0x%02x\n", state->pc);
    exit(1);
}

uint8_t parity(uint16_t value) {
    return __builtin_parity(value);
}

uint8_t is_zero(uint16_t value) {
    return (value & 0xff) == 0;
}

uint8_t sign(uint16_t value) {
    return (value & 0x80) != 0;
}

uint8_t carry(uint16_t value) {
    return value > 0xff;
}

/*--------------- Arithmetic Instructions ---------------*/
void add(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)(state->a) + (uint16_t)value;
    state->codes.z = is_zero(result);
    state->codes.s = sign(result);
    state->codes.p = parity(result);
    state->codes.cy = carry(result);
    state->a = result & 0xff;
}

void adc(State8080* state, uint8_t value) {
    uint16_t result = (uint16_t)(state->a) + (uint16_t)value + (uint16_t)state->codes.cy;
    state->codes.z = is_zero(result);
    state->codes.s = sign(result);
    state->codes.p = parity(result);
    state->codes.cy = carry(result);
    state->a = result & 0xff;
}

/*----------------- Branch Instructions -----------------*/
void jmp(State8080* state, unsigned char* opcode) {
    // Combine the next 2 bytes into an address and assign the program counter to it.
    state->pc = ((uint16_t)(opcode[2]) << 8) | (uint16_t)opcode[1];
}

void call(State8080* state, unsigned char* opcode) {
    // The return address is the next instruction. Since CALL is a 3 byte instruction, the next
    // instruction is 2 bytes away (remember state->pc is incremented after each operation, which
    // is why we're adding 2 instead of 3 to get to the next instruction.)
    uint16_t ret_address = state->pc + 2;

    // Push the address of the next instruction onto the stack
    state->memory[state->sp - 1] = (ret_address >> 8) & 0xff;
    state->memory[state->sp - 2] = ret_address * 0xff;
    state->sp = state->sp - 2;

    // Jump to the address
    jmp(state, opcode);
}

void ret(State8080* state) {
    // Assign the program counter to the address at the top of the stack, then move the stack
    // pointer back down to "pop" it.
    state->pc = (uint16_t)(state->memory[state->sp]) | 
        (uint16_t)((state->memory[state->sp + 1]) << 8);
    state->sp += 2;
}

/*-------------------------------------------------------*/

void emulate_op(State8080* state) {
    unsigned char* opcode = &(state->memory[state->pc]);
    uint16_t addr_offset;

    switch(*opcode) {
        case 0x00: break;       // NOP
        case 0x01:              // LXI B,2-byte-immediate
            state->b = opcode[2];
            state->c = opcode[1];
            state->pc += 2;
            break;
        case 0x02:              // STAX B
            unimplemented_op_error(state); break;
        case 0x03: unimplemented_op_error(state); break;
        case 0x04: unimplemented_op_error(state); break;
        case 0x05: unimplemented_op_error(state); break;
        case 0x06: unimplemented_op_error(state); break;
        case 0x07: unimplemented_op_error(state); break;
        case 0x08: unimplemented_op_error(state); break;
        case 0x09: unimplemented_op_error(state); break;
        case 0x0a: unimplemented_op_error(state); break;
        case 0x0b: unimplemented_op_error(state); break;
        case 0x0c: unimplemented_op_error(state); break;
        case 0x0d: unimplemented_op_error(state); break;
        case 0x0e: unimplemented_op_error(state); break;
        case 0x0f: unimplemented_op_error(state); break;

        case 0x10: unimplemented_op_error(state); break;
        case 0x11: unimplemented_op_error(state); break;
        case 0x12: unimplemented_op_error(state); break;
        case 0x13: unimplemented_op_error(state); break;
        case 0x14: unimplemented_op_error(state); break;
        case 0x15: unimplemented_op_error(state); break;
        case 0x16: unimplemented_op_error(state); break;
        case 0x17: unimplemented_op_error(state); break;
        case 0x18: unimplemented_op_error(state); break;
        case 0x19: unimplemented_op_error(state); break;
        case 0x1a: unimplemented_op_error(state); break;
        case 0x1b: unimplemented_op_error(state); break;
        case 0x1c: unimplemented_op_error(state); break;
        case 0x1d: unimplemented_op_error(state); break;
        case 0x1e: unimplemented_op_error(state); break;
        case 0x1f: unimplemented_op_error(state); break;

        case 0x20: unimplemented_op_error(state); break;
        case 0x21: unimplemented_op_error(state); break;
        case 0x22: unimplemented_op_error(state); break;
        case 0x23: unimplemented_op_error(state); break;
        case 0x24: unimplemented_op_error(state); break;
        case 0x25: unimplemented_op_error(state); break;
        case 0x26: unimplemented_op_error(state); break;
        case 0x27: unimplemented_op_error(state); break;
        case 0x28: unimplemented_op_error(state); break;
        case 0x29: unimplemented_op_error(state); break;
        case 0x2a: unimplemented_op_error(state); break;
        case 0x2b: unimplemented_op_error(state); break;
        case 0x2c: unimplemented_op_error(state); break;
        case 0x2d: unimplemented_op_error(state); break;
        case 0x2e: unimplemented_op_error(state); break;
        case 0x2f: unimplemented_op_error(state); break;

        case 0x30: unimplemented_op_error(state); break;
        case 0x31: unimplemented_op_error(state); break;
        case 0x32: unimplemented_op_error(state); break;
        case 0x33: unimplemented_op_error(state); break;
        case 0x34: unimplemented_op_error(state); break;
        case 0x35: unimplemented_op_error(state); break;
        case 0x36: unimplemented_op_error(state); break;
        case 0x37: unimplemented_op_error(state); break;
        case 0x38: unimplemented_op_error(state); break;
        case 0x39: unimplemented_op_error(state); break;
        case 0x3a: unimplemented_op_error(state); break;
        case 0x3b: unimplemented_op_error(state); break;
        case 0x3c: unimplemented_op_error(state); break;
        case 0x3d: unimplemented_op_error(state); break;
        case 0x3e: unimplemented_op_error(state); break;
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

        case 0x90: unimplemented_op_error(state); break;
        case 0x91: unimplemented_op_error(state); break;
        case 0x92: unimplemented_op_error(state); break;
        case 0x93: unimplemented_op_error(state); break;
        case 0x94: unimplemented_op_error(state); break;
        case 0x95: unimplemented_op_error(state); break;
        case 0x96: unimplemented_op_error(state); break;
        case 0x97: unimplemented_op_error(state); break;
        case 0x98: unimplemented_op_error(state); break;
        case 0x99: unimplemented_op_error(state); break;
        case 0x9a: unimplemented_op_error(state); break;
        case 0x9b: unimplemented_op_error(state); break;
        case 0x9c: unimplemented_op_error(state); break;
        case 0x9d: unimplemented_op_error(state); break;
        case 0x9e: unimplemented_op_error(state); break;
        case 0x9f: unimplemented_op_error(state); break;

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
        case 0xd6: unimplemented_op_error(state); break;
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
        case 0xe5: unimplemented_op_error(state); break;
        case 0xe6: unimplemented_op_error(state); break;
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
        case 0xeb: unimplemented_op_error(state); break;
        case 0xec: unimplemented_op_error(state); break;
        case 0xed: unimplemented_op_error(state); break;
        case 0xee: unimplemented_op_error(state); break;
        case 0xef: unimplemented_op_error(state); break;

        case 0xf0: unimplemented_op_error(state); break;
        case 0xf1: unimplemented_op_error(state); break;
        case 0xf2:                                              // JP address
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
        case 0xfa:                                              // JM address
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
    printf("State: {a: %u, b: %u, c: %u, d: %u, e: %u, h: %u, l: %u, sp: 0x%02x, pc: 0x%02x\n", 
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

    // Read through the buffer and emulate every operation.
    ConditionCodes codes = {0, 0, 0, 0, 0};
    State8080 state = {0, 0, 0, 0, 0, 0, 0, 0, 0, buffer, codes, 0};
    while(state.pc < file_size) {
        print_state(&state);
        emulate_op(&state);
    }

    return 0;
}