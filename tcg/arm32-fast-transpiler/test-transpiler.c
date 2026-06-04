/*
 * ARM32 to ARM64 Fast Transpiler - Test Harness
 * 
 * Standalone test program for validation without full QEMU integration
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "arm32-decoder.h"
#include "arm64-emitter.h"
#include "arm32-transpiler.h"

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

void print_arm32_instruction(uint32_t encoding, uint32_t pc)
{
    ARM32Instr decoded = arm32_decode(encoding, pc);
    const char *mnemonic = arm32_instr_mnemonic(encoding);
    const char *cond = arm32_cond_name(decoded.cond);
    
    printf("0x%08x [0x%08x]: %s%s\n", pc, encoding, mnemonic, cond);
}

void print_arm64_code(uint8_t *code, size_t size)
{
    printf("Generated ARM64 code (%zu bytes):\n", size);
    for (size_t i = 0; i < size; i += 4) {
        uint32_t instr = *(uint32_t *)(code + i);
        printf("  [%zu] 0x%08x\n", i, instr);
    }
}

/* ============================================================================
 * Test Cases
 * ============================================================================ */

void test_decoder_data_processing()
{
    printf("=== Test: ARM32 Data Processing Decoder ===\n");
    
    /* ADD r0, r1, r2 (ARM32) */
    uint32_t add_encoding = 0xe0800001;  /* cond=always, op=ADD, rd=r0, rn=r1, rm=r2 */
    ARM32DataProcFields fields = arm32_extract_data_proc_fields(add_encoding);
    
    printf("ADD r0, r1, r2: rd=%u, rn=%u, rm=%u, op=%u\n", 
           fields.rd, fields.rn, fields.rm, fields.op);
    
    /* MOV r3, #0x42 (ARM32 with immediate) */
    uint32_t mov_imm = 0xe3a03042;  /* cond=always, op=MOV, rd=r3, imm=0x42 */
    fields = arm32_extract_data_proc_fields(mov_imm);
    
    printf("MOV r3, #0x42: rd=%u, immediate=0x%x\n", fields.rd, fields.immediate);
    printf("\n");
}

void test_decoder_load_store()
{
    printf("=== Test: ARM32 Load/Store Decoder ===\n");
    
    /* LDR r0, [r1, #4] (ARM32) */
    uint32_t ldr_encoding = 0xe5910004;
    ARM32LoadStoreFields fields = arm32_extract_load_store_fields(ldr_encoding, 0);
    
    printf("LDR r0, [r1, #4]: rd=%u, rn=%u, is_load=%d, offset=%d\n",
           fields.rd, fields.rn, fields.is_load, fields.offset);
    
    /* STR r2, [r3, #8] (ARM32) */
    uint32_t str_encoding = 0xe5832008;
    fields = arm32_extract_load_store_fields(str_encoding, 0);
    
    printf("STR r2, [r3, #8]: rd=%u, rn=%u, is_load=%d, offset=%d\n",
           fields.rd, fields.rn, fields.is_load, fields.offset);
    printf("\n");
}

void test_decoder_branch()
{
    printf("=== Test: ARM32 Branch Decoder ===\n");
    
    /* B #0x1000 (ARM32) */
    uint32_t b_encoding = 0xea000400;  /* Branch offset encoded */
    ARM32BranchFields fields = arm32_extract_branch_fields(b_encoding, 0);
    
    printf("B target: link=%d, exchange=%d, offset=%d\n",
           fields.link, fields.exchange, fields.offset);
    printf("\n");
}

void test_emitter_basic()
{
    printf("=== Test: ARM64 Code Emitter ===\n");
    
    ARM64Emitter *emitter = arm64_emitter_create(4096);
    
    /* Emit some basic ARM64 instructions */
    arm64_mov_immediate(emitter, 0, 0x42, 0);  /* MOV w0, #0x42 */
    arm64_mov_immediate(emitter, 1, 0x10, 0);  /* MOV w1, #0x10 */
    arm64_add_register(emitter, 2, 0, 1, 0);   /* ADD w2, w0, w1 */
    arm64_branch(emitter, 0);                   /* B . (infinite loop) */
    
    uint8_t *code = arm64_emitter_get_code(emitter);
    size_t size = arm64_emitter_get_size(emitter);
    
    print_arm64_code(code, size);
    
    arm64_emitter_destroy(emitter);
    printf("\n");
}

void test_transpiler_simple()
{
    printf("=== Test: ARM32->ARM64 Transpiler (Simple) ===\n");
    
    ARM32Transpiler *tr = arm32_transpiler_create(4096);
    
    /* Create a simple ARM32 code buffer */
    uint8_t guest_memory[256];
    uint32_t *guest_instructions = (uint32_t *)guest_memory;
    
    /* ARM32 instructions:
     *   0: ADD r0, r1, r2       (0xe0800001)
     *   4: MOV r3, #0x10        (0xe3a03010)
     *   8: B . (branch to self) (0xeafffffe)
     */
    guest_instructions[0] = 0xe0800001;  /* ADD r0, r1, r2 */
    guest_instructions[1] = 0xe3a03010;  /* MOV r3, #0x10 */
    guest_instructions[2] = 0xeafffffe;  /* B . */
    
    printf("Input ARM32 instructions:\n");
    print_arm32_instruction(guest_instructions[0], 0);
    print_arm32_instruction(guest_instructions[1], 4);
    print_arm32_instruction(guest_instructions[2], 8);
    printf("\n");
    
    /* Translate the block */
    TranslatedBlock *block = arm32_transpiler_get_or_translate(tr, 0, guest_memory);
    
    if (block) {
        printf("Generated ARM64 code block:\n");
        printf("  Start PC: 0x%x\n", block->arm32_start_pc);
        printf("  End PC: 0x%x\n", block->arm32_end_pc);
        printf("  ARM64 code size: %zu bytes\n", block->arm64_code_size);
        
        print_arm64_code(block->arm64_code, block->arm64_code_size);
    }
    
    arm32_transpiler_print_stats(tr);
    arm32_transpiler_destroy(tr);
    printf("\n");
}

void test_transpiler_caching()
{
    printf("=== Test: Transpiler Caching ===\n");
    
    ARM32Transpiler *tr = arm32_transpiler_create(4096);
    arm32_transpiler_set_profiling(tr, true);
    
    uint8_t guest_memory[256];
    uint32_t *guest_instructions = (uint32_t *)guest_memory;
    
    guest_instructions[0] = 0xe0800001;  /* ADD r0, r1, r2 */
    guest_instructions[1] = 0xeafffffe;  /* B . */
    
    /* Translate the same block multiple times */
    printf("Translating block at PC 0 three times...\n");
    
    for (int i = 0; i < 3; i++) {
        TranslatedBlock *block = arm32_transpiler_get_or_translate(tr, 0, guest_memory);
        printf("  Iteration %d: block=%p\n", i + 1, (void *)block);
    }
    
    arm32_transpiler_print_stats(tr);
    arm32_transpiler_destroy(tr);
    printf("\n");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(int argc, char *argv[])
{
    printf("ARM32->ARM64 Fast Transpiler - PoC Test Suite\n");
    printf("===========================================\n\n");
    
    test_decoder_data_processing();
    test_decoder_load_store();
    test_decoder_branch();
    test_emitter_basic();
    test_transpiler_simple();
    test_transpiler_caching();
    
    printf("=== All Tests Complete ===\n");
    return 0;
}
