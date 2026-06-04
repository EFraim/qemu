/*
 * ARM32 to ARM64 Fast Transpiler - ARM64 Code Emitter Header
 * 
 * Emits ARM64 (AArch64) instructions and manages code buffer.
 * Reference: ARM Architecture Reference Manual ARM64 (DDI 0487)
 */

#ifndef ARM64_EMITTER_H
#define ARM64_EMITTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ARM64 Register Codes */
typedef enum {
    ARM64_REG_X0 = 0, ARM64_REG_X1, ARM64_REG_X2, ARM64_REG_X3,
    ARM64_REG_X4, ARM64_REG_X5, ARM64_REG_X6, ARM64_REG_X7,
    ARM64_REG_X8, ARM64_REG_X9, ARM64_REG_X10, ARM64_REG_X11,
    ARM64_REG_X12, ARM64_REG_X13, ARM64_REG_X14, ARM64_REG_X15,
    ARM64_REG_X16, ARM64_REG_X17, ARM64_REG_X18, ARM64_REG_X19,
    ARM64_REG_X20, ARM64_REG_X21, ARM64_REG_X22, ARM64_REG_X23,
    ARM64_REG_X24, ARM64_REG_X25, ARM64_REG_X26, ARM64_REG_X27,
    ARM64_REG_X28, ARM64_REG_X29, ARM64_REG_X30, ARM64_REG_XZR = 31,
} ARM64Register;

/* ARM64 Shift Types */
typedef enum {
    ARM64_SHIFT_LSL = 0,
    ARM64_SHIFT_LSR = 1,
    ARM64_SHIFT_ASR = 2,
    ARM64_SHIFT_ROR = 3,
} ARM64ShiftType;

/* Code Emitter Context */
typedef struct {
    uint8_t *buffer;        /* Code buffer */
    size_t capacity;        /* Buffer capacity in bytes */
    size_t pos;             /* Current position in bytes */
    bool is_64bit;          /* Default to 64-bit operations */
} ARM64Emitter;

/* ============================================================================
 * Emitter Management
 * ============================================================================ */

ARM64Emitter* arm64_emitter_create(size_t capacity);
void arm64_emitter_destroy(ARM64Emitter *emitter);
size_t arm64_emitter_get_size(ARM64Emitter *emitter);
uint8_t* arm64_emitter_get_code(ARM64Emitter *emitter);
void arm64_emitter_reset(ARM64Emitter *emitter);

/* ============================================================================
 * Low-level Instruction Emission
 * ============================================================================ */

void arm64_emit(ARM64Emitter *emitter, uint32_t instr);

/* ============================================================================
 * Instruction Generators - Data Processing
 * ============================================================================ */

void arm64_add_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit);
void arm64_add_immediate(ARM64Emitter *emitter, uint8_t rd, uint8_t rn,
                         uint32_t imm, int shift, int is_64bit);
void arm64_sub_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit);
void arm64_sub_immediate(ARM64Emitter *emitter, uint8_t rd, uint8_t rn,
                         uint32_t imm, int shift, int is_64bit);
void arm64_mov_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int is_64bit);
void arm64_mov_immediate(ARM64Emitter *emitter, uint8_t rd, uint64_t imm, int is_64bit);
void arm64_and_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit);
void arm64_orr_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit);
void arm64_eor_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                        int is_64bit);
void arm64_shift_register(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t shift_amount,
                          ARM64ShiftType shift_type, int is_64bit);

/* ============================================================================
 * Load/Store Instructions
 * ============================================================================ */

void arm64_ldr(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int32_t offset,
               uint8_t size, int is_64bit);
void arm64_str(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, int32_t offset,
               uint8_t size, int is_64bit);
void arm64_ldr_reg_offset(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                          uint8_t shift, uint8_t size, int is_64bit);
void arm64_str_reg_offset(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                          uint8_t shift, uint8_t size, int is_64bit);

/* ============================================================================
 * Branch Instructions
 * ============================================================================ */

void arm64_branch(ARM64Emitter *emitter, int32_t offset);
void arm64_branch_link(ARM64Emitter *emitter, int32_t offset);
void arm64_branch_register(ARM64Emitter *emitter, uint8_t rn);
void arm64_branch_link_register(ARM64Emitter *emitter, uint8_t rn);

/* ============================================================================
 * Conditional Execution
 * ============================================================================ */

void arm64_branch_conditional(ARM64Emitter *emitter, uint8_t cond, int32_t offset);
void arm64_csel(ARM64Emitter *emitter, uint8_t rd, uint8_t rn, uint8_t rm,
                uint8_t cond, int is_64bit);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

uint8_t arm32_to_arm64_cond(uint8_t arm32_cond);
uint8_t arm32_to_arm64_reg(uint8_t arm32_reg);

#endif /* ARM64_EMITTER_H */
