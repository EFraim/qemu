/*
 * ARM32 to ARM64 Fast Transpiler - ARM64 Code Emitter Implementation
 * 
 * Generates ARM64 (AArch64) instructions and manages the code buffer.
 */

#include "arm64-emitter.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Emitter Management
 * ============================================================================ */

ARM64Emitter* arm64_emitter_create(size_t capacity)
{
    ARM64Emitter *emitter = malloc(sizeof(ARM64Emitter));
    emitter->buffer = malloc(capacity);
    emitter->capacity = capacity;
    emitter->pos = 0;
    emitter->is_64bit = 1;  /* Default to 64-bit operations */
    return emitter;
}

void arm64_emitter_destroy(ARM64Emitter *emitter)
{
    if (emitter) {
        free(emitter->buffer);
        free(emitter);
    }
}

size_t arm64_emitter_get_size(ARM64Emitter *emitter)
{
    return emitter->pos;
}

uint8_t* arm64_emitter_get_code(ARM64Emitter *emitter)
{
    return emitter->buffer;
}

void arm64_emitter_reset(ARM64Emitter *emitter)
{
    emitter->pos = 0;
}

/* ============================================================================
 * Low-level Instruction Emission
 * ============================================================================ */

void arm64_emit(ARM64Emitter *emitter, uint32_t instr)
{
    if (emitter->pos + 4 <= emitter->capacity) {
        *(uint32_t *)(emitter->buffer + emitter->pos) = instr;
        emitter->pos += 4;
    }
}

/* ============================================================================
 * Data Processing - ADD/SUB
 * ============================================================================ */

void arm64_add_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit)
{
    /*
     * ADD (register) encoding:
     * [31] sf (0=32-bit, 1=64-bit)
     * [30:24] = 0011011
     * [23:22] = shift (00=LSL)
     * [21] = 0
     * [20:16] = rm
     * [15:10] = imm6 (shift amount)
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x0B << 24) | (rm << 16) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_add_immediate(ARM64Emitter *emitter, uint8_t rd, uint8_t rn,
                         uint32_t imm, int shift, int is_64bit)
{
    /*
     * ADDI (immediate) encoding:
     * [31] sf
     * [30:24] = 0010001
     * [23:22] = shift (0=no shift, 1=shift by 12)
     * [21:10] = imm12
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t sh = (shift >> 12) ? 1 : 0;  /* 0 or 12 -> 0 or 1 */
    uint32_t instr = (sf << 31) | (0x11 << 24) | (sh << 22) | (imm << 10) |
                     (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_sub_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit)
{
    /*
     * SUB (register) encoding:
     * Same as ADD but opcode is 0x4B instead of 0x0B
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x4B << 24) | (rm << 16) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_sub_immediate(ARM64Emitter *emitter, uint8_t rd, uint8_t rn,
                         uint32_t imm, int shift, int is_64bit)
{
    /*
     * SUBI (immediate) encoding:
     * Same as ADDI but opcode is 0x51 instead of 0x11
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t sh = (shift >> 12) ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x51 << 24) | (sh << 22) | (imm << 10) |
                     (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

/* ============================================================================
 * Data Processing - MOV/ORR/AND/EOR
 * ============================================================================ */

void arm64_mov_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int is_64bit)
{
    /*
     * MOV (register) is encoded as ORR rd, xzr, rn
     * ORR encoding:
     * [31] sf
     * [30:24] = 0101010
     * [23:22] = shift
     * [21] = 1
     * [20:16] = rm
     * [15:10] = imm6
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x2A << 24) | (1 << 21) | (rn << 16) | (31 << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_mov_immediate(ARM64Emitter *emitter, uint8_t rd, uint64_t imm, int is_64bit)
{
    /*
     * MOV immediate: use MOVZ for first 16 bits, MOVK for additional 16-bit chunks
     * MOVZ encoding:
     * [31] sf
     * [30:29] = 10
     * [28:23] = 100101
     * [22:21] = shift (0=0, 1=16, 2=32, 3=48)
     * [20:5] = imm16
     * [4:0] = rd
     */
    
    if (is_64bit) {
        /* Use MOVZ for first 16 bits */
        uint32_t imm16 = imm & 0xFFFF;
        uint32_t instr = (1 << 31) | (0x25 << 23) | (imm16 << 5) | rd;
        arm64_emit(emitter, instr);
        
        /* Use MOVK for remaining bits */
        for (int shift = 16; shift < 64; shift += 16) {
            imm16 = (imm >> shift) & 0xFFFF;
            if (imm16 || shift == 16) {  /* At least emit second half */
                uint32_t sh = shift / 16;
                instr = (1 << 31) | (0x27 << 23) | (sh << 21) | (imm16 << 5) | rd;
                arm64_emit(emitter, instr);
            }
        }
    } else {
        /* 32-bit: just MOVZ + optional MOVK */
        uint32_t imm16 = imm & 0xFFFF;
        uint32_t instr = (0 << 31) | (0x25 << 23) | (imm16 << 5) | rd;
        arm64_emit(emitter, instr);
        
        imm16 = (imm >> 16) & 0xFFFF;
        if (imm16) {
            instr = (0 << 31) | (0x27 << 23) | (1 << 21) | (imm16 << 5) | rd;
            arm64_emit(emitter, instr);
        }
    }
}

void arm64_and_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit)
{
    /*
     * AND (register) encoding:
     * [31] sf
     * [30:24] = 0001010
     * [23:22] = shift
     * [21] = 0
     * [20:16] = rm
     * [15:10] = imm6
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x0A << 24) | (rm << 16) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_orr_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit)
{
    /*
     * ORR (register) encoding:
     * [31] sf
     * [30:24] = 0101010
     * [23:22] = shift
     * [21] = 0
     * [20:16] = rm
     * [15:10] = imm6
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x2A << 24) | (rm << 16) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_eor_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit)
{
    /*
     * EOR (XOR, register) encoding:
     * [31] sf
     * [30:24] = 0100010
     * [23:22] = shift
     * [21] = 0
     * [20:16] = rm
     * [15:10] = imm6
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x22 << 24) | (rm << 16) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_shift_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t shift_amount,
                          ARM64ShiftType shift_type, int is_64bit)
{
    /*
     * Shift by immediate (UBFM/SBFM variants):
     * For simplicity, use LSL variant
     * [31] sf
     * [30:29] = 10
     * [28:23] = 100011
     * [22] = N (immr/imms match)
     * [21:16] = immr
     * [15:10] = imms
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    
    /* Use UBFM for logical shifts, SBFM for arithmetic */
    uint32_t opcode = (shift_type == ARM64_SHIFT_ASR) ? 0x23 : 0x22;
    
    uint32_t immr = (32 - shift_amount) & 0x3F;
    uint32_t imms = 31 - shift_amount;
    
    uint32_t instr = (sf << 31) | (opcode << 23) | (immr << 16) | (imms << 10) |
                     (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

/* ============================================================================
 * Load/Store Instructions
 * ============================================================================ */

void arm64_ldr(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int32_t offset,
               uint8_t size, int is_64bit)
{
    /*
     * LDR (unsigned immediate) encoding:
     * [31:30] = size (00=byte, 01=halfword, 10=word, 11=dword)
     * [29:27] = 101
     * [26] = 0 (unsigned)
     * [25:24] = 01 (immediate)
     * [23:10] = offset/scale
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t size_bits = (size == 8) ? 3 : (size == 4) ? 2 : 1;
    uint32_t scaled_offset = offset / (1 << (size_bits / 2));
    
    uint32_t instr = (size_bits << 30) | (0x5 << 27) | (0x1 << 24) |
                     ((scaled_offset & 0x3FFF) << 10) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_str(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int32_t offset,
               uint8_t size, int is_64bit)
{
    /*
     * STR (unsigned immediate) encoding:
     * Similar to LDR but different bits
     */
    uint32_t size_bits = (size == 8) ? 3 : (size == 4) ? 2 : 1;
    uint32_t scaled_offset = offset / (1 << (size_bits / 2));
    
    uint32_t instr = (size_bits << 30) | (0x5 << 27) | (0x0 << 24) |
                     ((scaled_offset & 0x3FFF) << 10) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_ldr_reg_offset(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                          uint8_t shift, uint8_t size, int is_64bit)
{
    /*
     * LDR with register offset:
     * [31:30] = size
     * [29:27] = 101
     * [26] = 0 (unsigned)
     * [25:24] = 00 (register)
     * [23:21] = 000
     * [20:16] = rm
     * [15:13] = shift option (111 for LSL #scale)
     * [12:10] = shift amount
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t size_bits = (size == 8) ? 3 : (size == 4) ? 2 : 1;
    
    uint32_t instr = (size_bits << 30) | (0x5 << 27) | (rm << 16) | (0x7 << 13) |
                     (shift << 10) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

void arm64_str_reg_offset(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                          uint8_t shift, uint8_t size, int is_64bit)
{
    uint32_t size_bits = (size == 8) ? 3 : (size == 4) ? 2 : 1;
    
    uint32_t instr = (size_bits << 30) | (0x5 << 27) | (0x0 << 24) | (rm << 16) |
                     (0x7 << 13) | (shift << 10) | (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

/* ============================================================================
 * Branch Instructions
 * ============================================================================ */

void arm64_branch(ARM64Emitter *emitter, int32_t offset)
{
    /*
     * B (unconditional branch) encoding:
     * [31:26] = 000101
     * [25:0] = offset (signed, in instructions)
     */
    uint32_t instr = (0x5 << 26) | (offset & 0x3FFFFFF);
    arm64_emit(emitter, instr);
}

void arm64_branch_link(ARM64Emitter *emitter, int32_t offset)
{
    /*
     * BL (branch with link) encoding:
     * [31:26] = 100101
     * [25:0] = offset
     */
    uint32_t instr = (0x25 << 26) | (offset & 0x3FFFFFF);
    arm64_emit(emitter, instr);
}

void arm64_branch_register(ARM64Emitter *emitter, uint8_t rn)
{
    /*
     * BR (branch to register) encoding:
     * [31:10] = 1101011000011111000000
     * [9:5] = rn
     * [4:0] = 00000
     */
    uint32_t instr = (0x6B << 24) | (rn << 5);
    arm64_emit(emitter, instr);
}

void arm64_branch_link_register(ARM64Emitter *emitter, uint8_t rn)
{
    /*
     * BLR (branch with link to register) encoding:
     * [31:10] = 1101011000111111000000
     * [9:5] = rn
     * [4:0] = 00000
     */
    uint32_t instr = (0x6B << 24) | (0x1 << 21) | (rn << 5);
    arm64_emit(emitter, instr);
}

void arm64_branch_conditional(ARM64Emitter *emitter, uint8_t cond, int32_t offset)
{
    /*
     * B.cond (conditional branch) encoding:
     * [31:24] = 01010100
     * [23:5] = offset (signed, in instructions)
     * [4:0] = cond
     */
    uint32_t instr = (0x54 << 24) | ((offset & 0x7FFFF) << 5) | (cond & 0xF);
    arm64_emit(emitter, instr);
}

void arm64_csel(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                uint8_t cond, int is_64bit)
{
    /*
     * CSEL (conditional select) encoding:
     * [31] sf
     * [30:21] = 1101010100
     * [20:16] = rm
     * [15:12] = cond
     * [11:10] = 00
     * [9:5] = rn
     * [4:0] = rd
     */
    uint32_t sf = is_64bit ? 1 : 0;
    uint32_t instr = (sf << 31) | (0x354 << 21) | (rm << 16) | (cond << 12) |
                     (rn << 5) | rd;
    arm64_emit(emitter, instr);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

uint8_t arm32_to_arm64_cond(uint8_t arm32_cond)
{
    /* ARM32 and ARM64 condition codes are mostly compatible */
    return arm32_cond & 0xF;
}

uint8_t arm32_to_arm64_reg(uint8_t arm32_reg)
{
    /* Direct 1:1 mapping for common registers */
    return arm32_reg & 0x1F;
}
