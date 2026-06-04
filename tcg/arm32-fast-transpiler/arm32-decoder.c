/*
 * ARM32 to ARM64 Fast Transpiler - Instruction Decoder Implementation
 * 
 * Implements ARM32 instruction decoding and field extraction.
 * Reference: ARM Architecture Reference Manual ARMv7-A and ARMv7-R (DDI 0406C)
 */

#include "arm32-decoder.h"
#include <string.h>

/* Condition code names for debugging */
static const char *cond_names[] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "al", "??",
};

/* Data processing opcode names */
static const char *dp_op_names[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn",
};

/* ============================================================================
 * Basic Decoding Functions
 * ============================================================================ */

ARM32Instr arm32_decode(uint32_t encoding, uint32_t pc)
{
    ARM32Instr instr = {0};
    instr.encoding = encoding;
    instr.pc = pc;
    instr.cond = (encoding >> 28) & 0xF;  /* Bits [31:28] */
    instr.opcode = (encoding >> 25) & 0x7; /* Bits [27:25] */
    return instr;
}

ARM32InstructionType arm32_identify_instruction(uint32_t encoding)
{
    uint32_t opcode = (encoding >> 25) & 0x7;
    uint32_t bits27_26 = (encoding >> 26) & 0x3;
    uint32_t bits7_4 = (encoding >> 4) & 0xF;
    
    /* Branch instructions: bits [27:25] = 101 */
    if ((encoding >> 25) == 0x5) {
        return ARM32_BRANCH;
    }
    
    /* Data processing and variants: bits [27:26] = 00 */
    if (bits27_26 == 0) {
        /* Check for multiply: bits [27:4] = specific pattern */
        if ((bits7_4 & 0x9) == 0x9) {
            return ARM32_MULTIPLY;
        }
        return ARM32_DATA_PROCESSING;
    }
    
    /* Load/Store Single Data Transfer: bits [27:26] = 01 */
    if (bits27_26 == 1) {
        return ARM32_LOAD_STORE;
    }
    
    /* Load/Store Multiple: bits [27:25] = 100 */
    if (opcode == 0x4) {
        return ARM32_LOAD_STORE_MULTI;
    }
    
    /* Coprocessor Data Transfer: bits [27:25] = 110 */
    if (opcode == 0x6) {
        return ARM32_COPROCESSOR;
    }
    
    /* SWI/Software Interrupt: bits [27:24] = 1111 */
    if ((encoding >> 24) == 0xF) {
        return ARM32_SWI_BKPT;
    }
    
    return ARM32_UNKNOWN;
}

/* ============================================================================
 * Data Processing Field Extraction
 * ============================================================================ */

ARM32DataProcFields arm32_extract_data_proc_fields(uint32_t encoding)
{
    ARM32DataProcFields fields = {0};
    
    fields.op = (encoding >> 21) & 0xF;  /* Bits [24:21] - operation */
    fields.rd = (encoding >> 12) & 0xF;  /* Bits [15:12] - destination */
    fields.rn = (encoding >> 16) & 0xF;  /* Bits [19:16] - operand 1 */
    fields.rm = encoding & 0xF;           /* Bits [3:0] - operand 2 (register) */
    
    fields.imm_flag = (encoding >> 25) & 1; /* Bit [25] - immediate flag */
    
    if (fields.imm_flag) {
        /* Immediate operand with rotate */
        uint8_t imm_val = encoding & 0xFF;         /* Bits [7:0] */
        uint8_t rotate = ((encoding >> 8) & 0xF) << 1; /* Bits [11:8] * 2 */
        
        /* Decode immediate: 8-bit value rotated right by rotate amount */
        if (rotate) {
            fields.immediate = (imm_val >> rotate) | (imm_val << (32 - rotate));
        } else {
            fields.immediate = imm_val;
        }
    } else {
        /* Register operand with optional shift */
        fields.shift_type = (encoding >> 5) & 0x3;   /* Bits [6:5] */
        
        bool shift_imm_flag = (encoding >> 4) & 1;   /* Bit [4] */
        if (shift_imm_flag) {
            fields.shift_amount = (encoding >> 7) & 0x1F; /* Bits [11:7] */
        } else {
            fields.rs = (encoding >> 8) & 0xF;       /* Bits [11:8] - shift register */
        }
    }
    
    return fields;
}

/* ============================================================================
 * Load/Store Field Extraction
 * ============================================================================ */

ARM32LoadStoreFields arm32_extract_load_store_fields(uint32_t encoding, uint32_t pc)
{
    ARM32LoadStoreFields fields = {0};
    
    fields.rd = (encoding >> 12) & 0xF;    /* Bits [15:12] */
    fields.rn = (encoding >> 16) & 0xF;    /* Bits [19:16] - base register */
    
    fields.is_load = (encoding >> 20) & 1; /* Bit [20] - L flag */
    
    /* Byte/Word flag: Bit [22] */
    uint8_t b_flag = (encoding >> 22) & 1;
    
    if (b_flag) {
        fields.size = 1; /* Byte */
    } else {
        fields.size = 0; /* Word */
    }
    
    fields.pre_indexed = (encoding >> 24) & 1;  /* Bit [24] - P flag */
    fields.writeback = (encoding >> 21) & 1;    /* Bit [21] - W flag */
    
    /* Addressing mode: immediate vs register */
    fields.imm_flag = !((encoding >> 25) & 1); /* Bit [25] - I flag inverted */
    
    if (fields.imm_flag) {
        /* Immediate offset */
        fields.offset = encoding & 0xFFF; /* Bits [11:0] */
        
        /* Apply sign based on U flag (Bit [23]) */
        if (!((encoding >> 23) & 1)) {
            fields.offset = -fields.offset;
        }
    } else {
        /* Register offset with optional shift */
        fields.rm = encoding & 0xF;              /* Bits [3:0] */
        fields.shift_type = (encoding >> 5) & 0x3; /* Bits [6:5] */
        fields.shift_amount = (encoding >> 7) & 0x1F; /* Bits [11:7] */
        
        /* Apply sign based on U flag */
        if (!((encoding >> 23) & 1)) {
            fields.shift_amount = -fields.shift_amount;
        }
    }
    
    return fields;
}

/* ============================================================================
 * Branch Field Extraction
 * ============================================================================ */

ARM32BranchFields arm32_extract_branch_fields(uint32_t encoding, uint32_t pc)
{
    ARM32BranchFields fields = {0};
    
    fields.cond = (encoding >> 28) & 0xF;       /* Bits [31:28] */
    fields.link = (encoding >> 24) & 1;         /* Bit [24] - L flag */
    
    /* Branch offset is sign-extended 24-bit value, shifted left by 2 */
    int32_t offset_24 = encoding & 0xFFFFFF;
    
    /* Sign extend from 24 bits */
    if (offset_24 & 0x800000) {
        offset_24 |= 0xFF000000;
    }
    
    /* Offset is in bytes, already counted from after instruction fetch */
    fields.offset = offset_24;
    
    /* Check for BX/BLX (exchange instruction set) */
    fields.exchange = ((encoding >> 4) & 0xF) == 0x1; /* Bits [7:4] = 0001 */
    if (fields.exchange) {
        fields.target_reg = encoding & 0xF; /* Bits [3:0] - target register */
    }
    
    return fields;
}

/* ============================================================================
 * Debugging/String Functions
 * ============================================================================ */

const char* arm32_cond_name(uint8_t cond)
{
    if (cond < 16) {
        return cond_names[cond];
    }
    return "??";
}

const char* arm32_instr_mnemonic(uint32_t encoding)
{
    ARM32InstructionType type = arm32_identify_instruction(encoding);
    
    switch (type) {
        case ARM32_DATA_PROCESSING: {
            uint8_t op = (encoding >> 21) & 0xF;
            return dp_op_names[op];
        }
        case ARM32_LOAD_STORE: {
            bool is_load = (encoding >> 20) & 1;
            bool is_byte = (encoding >> 22) & 1;
            
            if (is_load) {
                return is_byte ? "ldrb" : "ldr";
            } else {
                return is_byte ? "strb" : "str";
            }
        }
        case ARM32_BRANCH: {
            bool link = (encoding >> 24) & 1;
            bool exchange = ((encoding >> 4) & 0xF) == 0x1;
            
            if (exchange) {
                return link ? "blx" : "bx";
            } else {
                return link ? "bl" : "b";
            }
        }
        case ARM32_MULTIPLY:
            return "mul";
        default:
            return "???";
    }
}

bool arm32_is_conditional(uint32_t encoding)
{
    uint8_t cond = (encoding >> 28) & 0xF;
    return cond != 0xE && cond != 0xF; /* Not AL (always) or NV (never) */
}
