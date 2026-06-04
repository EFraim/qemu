/*
 * ARM32 to ARM64 Fast Transpiler - Decoder Module
 * 
 * Decodes ARM32 (AArch32) instructions and extracts operand fields.
 * Designed for direct translation to ARM64 code generation.
 */

#ifndef ARM32_DECODER_H
#define ARM32_DECODER_H

#include <stdint.h>
#include <stdbool.h>

/* ARM32 Instruction Structure */
typedef struct {
    uint32_t encoding;      /* Full 32-bit instruction word */
    uint32_t pc;            /* Program counter */
    uint8_t cond;           /* Condition bits [31:28] */
    uint8_t opcode;         /* Primary opcode field */
} ARM32Instr;

/* ARM32 Instruction Type Classification */
typedef enum {
    ARM32_DATA_PROCESSING,  /* ADD, SUB, MOV, AND, ORR, etc. */
    ARM32_MULTIPLY,         /* MUL, MLA, UMULL, etc. */
    ARM32_LOAD_STORE,       /* LDR, STR, LDRB, STRB, etc. */
    ARM32_LOAD_STORE_MULTI, /* LDM, STM */
    ARM32_BRANCH,           /* B, BL, BX, BLX */
    ARM32_COPROCESSOR,      /* CDP, LDC, STC, MCR, MRC */
    ARM32_SWI_BKPT,         /* SWI, BKPT */
    ARM32_UNKNOWN,
} ARM32InstructionType;

/* Data Processing Fields */
typedef struct {
    uint8_t rd, rn, rm, rs; /* Register operands */
    bool imm_flag;          /* Immediate operand present */
    uint32_t immediate;     /* Immediate value */
    uint8_t shift_type;     /* Shift type (LSL, LSR, ASR, ROR) */
    uint8_t shift_amount;   /* Shift amount */
    uint8_t op;             /* Data processing opcode (ADD=4, SUB=2, etc.) */
} ARM32DataProcFields;

/* Load/Store Fields */
typedef struct {
    uint8_t rd;             /* Destination/Source register */
    uint8_t rn;             /* Base register */
    uint8_t rm;             /* Index register (for reg offset) */
    bool imm_flag;          /* Immediate offset */
    int32_t offset;         /* Offset value (signed) */
    uint8_t shift_type;     /* Shift for register offset */
    uint8_t shift_amount;
    bool pre_indexed;       /* Pre-indexed addressing */
    bool writeback;         /* Write-back to base register */
    uint8_t size;           /* 0=word(4), 1=byte(1), 2=halfword(2) */
    bool is_load;           /* True for load, false for store */
    bool unsigned_flag;     /* Unsigned load (LDRB vs LDRSB) */
} ARM32LoadStoreFields;

/* Branch Fields */
typedef struct {
    int32_t offset;         /* Signed offset in instructions */
    uint8_t cond;           /* Condition */
    bool link;              /* Branch with link (BL) */
    bool exchange;          /* Exchange instruction set (BX/BLX) */
    uint8_t target_reg;     /* For register branches (BX rn) */
} ARM32BranchFields;

/* API Functions */

/**
 * Decode basic ARM32 instruction structure
 */
ARM32Instr arm32_decode(uint32_t encoding, uint32_t pc);

/**
 * Identify instruction type from encoding
 */
ARM32InstructionType arm32_identify_instruction(uint32_t encoding);

/**
 * Extract data processing operand fields
 */
ARM32DataProcFields arm32_extract_data_proc_fields(uint32_t encoding);

/**
 * Extract load/store operand fields
 */
ARM32LoadStoreFields arm32_extract_load_store_fields(uint32_t encoding, uint32_t pc);

/**
 * Extract branch operand fields
 */
ARM32BranchFields arm32_extract_branch_fields(uint32_t encoding, uint32_t pc);

/**
 * Get instruction mnemonic string (for debugging)
 */
const char* arm32_instr_mnemonic(uint32_t encoding);

/**
 * Check if instruction is conditional
 */
bool arm32_is_conditional(uint32_t encoding);

/**
 * Get condition name
 */
const char* arm32_cond_name(uint8_t cond);

#endif /* ARM32_DECODER_H */
