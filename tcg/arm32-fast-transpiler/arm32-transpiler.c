/*
 * ARM32 to ARM64 Fast Transpiler - Main Implementation
 * 
 * Core translation logic: converts ARM32 instruction streams to ARM64 code.
 */

#include "arm32-transpiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Transpiler Lifecycle
 * ============================================================================ */

ARM32Transpiler* arm32_transpiler_create(size_t code_buffer_size)
{
    ARM32Transpiler *tr = malloc(sizeof(ARM32Transpiler));
    if (!tr) return NULL;
    
    tr->emitter = arm64_emitter_create(code_buffer_size);
    if (!tr->emitter) {
        free(tr);
        return NULL;
    }
    
    tr->cache_capacity = 10000;
    tr->cache = malloc(sizeof(TranslationCacheEntry) * tr->cache_capacity);
    if (!tr->cache) {
        arm64_emitter_destroy(tr->emitter);
        free(tr);
        return NULL;
    }
    
    tr->cache_size = 0;
    tr->total_blocks_translated = 0;
    tr->total_instructions_translated = 0;
    tr->cache_hits = 0;
    tr->cache_misses = 0;
    tr->max_block_size = 100;  /* Max 100 instructions per block */
    tr->enable_caching = true;
    tr->enable_profiling = false;
    
    return tr;
}

void arm32_transpiler_destroy(ARM32Transpiler *tr)
{
    if (tr) {
        if (tr->emitter) {
            arm64_emitter_destroy(tr->emitter);
        }
        if (tr->cache) {
            free(tr->cache);
        }
        free(tr);
    }
}

void arm32_transpiler_reset(ARM32Transpiler *tr)
{
    if (tr) {
        arm64_emitter_reset(tr->emitter);
        tr->cache_size = 0;
        tr->total_blocks_translated = 0;
        tr->total_instructions_translated = 0;
    }
}

/* ============================================================================
 * Translation Cache
 * ============================================================================ */

TranslatedBlock* arm32_transpiler_lookup_cache(ARM32Transpiler *tr, uint32_t pc)
{
    for (size_t i = 0; i < tr->cache_size; i++) {
        if (tr->cache[i].pc == pc) {
            return tr->cache[i].block;
        }
    }
    return NULL;
}

void arm32_transpiler_insert_cache(ARM32Transpiler *tr, uint32_t pc, TranslatedBlock *block)
{
    if (tr->cache_size < tr->cache_capacity) {
        tr->cache[tr->cache_size].pc = pc;
        tr->cache[tr->cache_size].block = block;
        tr->cache_size++;
    }
}

void arm32_transpiler_flush_cache(ARM32Transpiler *tr)
{
    tr->cache_size = 0;
}

/* ============================================================================
 * Instruction-level Translation
 * ============================================================================ */

void arm32_translate_data_processing(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc)
{
    ARM32DataProcFields fields = arm32_extract_data_proc_fields(encoding);
    ARM64Emitter *emitter = tr->emitter;
    
    uint8_t rd = arm32_to_arm64_reg(fields.rd);
    uint8_t rn = arm32_to_arm64_reg(fields.rn);
    uint8_t rm = arm32_to_arm64_reg(fields.rm);
    
    /* Map ARM32 data processing opcodes to ARM64 instructions */
    switch (fields.op) {
        case 0x4: /* ADD */
            if (fields.imm_flag) {
                arm64_add_immediate(emitter, rd, rn, fields.immediate, 0, 0);
            } else {
                arm64_add_register(emitter, rd, rn, rm, 0);
            }
            break;
            
        case 0x2: /* SUB */
            if (fields.imm_flag) {
                arm64_sub_immediate(emitter, rd, rn, fields.immediate, 0, 0);
            } else {
                arm64_sub_register(emitter, rd, rn, rm, 0);
            }
            break;
            
        case 0xD: /* MOV */
            if (fields.imm_flag) {
                arm64_mov_immediate(emitter, rd, fields.immediate, 0);
            } else {
                arm64_mov_register(emitter, rd, rm, 0);
            }
            break;
            
        case 0x0: /* AND */
            arm64_and_register(emitter, rd, rn, rm, 0);
            break;
            
        case 0xC: /* ORR */
            arm64_orr_register(emitter, rd, rn, rm, 0);
            break;
            
        case 0x1: /* EOR */
            arm64_eor_register(emitter, rd, rn, rm, 0);
            break;
            
        default:
            /* Unimplemented data processing opcode - emit NOP or trap */
            break;
    }
}

void arm32_translate_load_store(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc)
{
    ARM32LoadStoreFields fields = arm32_extract_load_store_fields(encoding, pc);
    ARM64Emitter *emitter = tr->emitter;
    
    uint8_t rd = arm32_to_arm64_reg(fields.rd);
    uint8_t rn = arm32_to_arm64_reg(fields.rn);
    uint8_t rm = arm32_to_arm64_reg(fields.rm);
    
    uint8_t size = fields.size ? 1 : 4;  /* 1=byte, 4=word */
    
    if (fields.is_load) {
        if (fields.imm_flag) {
            arm64_ldr(emitter, rd, rn, fields.offset, size, 0);
        } else {
            arm64_ldr_reg_offset(emitter, rd, rn, rm, fields.shift_amount, size, 0);
        }
    } else {
        if (fields.imm_flag) {
            arm64_str(emitter, rd, rn, fields.offset, size, 0);
        } else {
            arm64_str_reg_offset(emitter, rd, rn, rm, fields.shift_amount, size, 0);
        }
    }
}

void arm32_translate_branch(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc)
{
    ARM32BranchFields fields = arm32_extract_branch_fields(encoding, pc);
    ARM64Emitter *emitter = tr->emitter;
    
    /* Convert ARM32 offset (bytes) to instruction count */
    int32_t offset_instr = fields.offset >> 2;
    
    if (fields.exchange) {
        /* BX/BLX - branch to register (exchange instruction set) */
        uint8_t target_reg = arm32_to_arm64_reg(fields.target_reg);
        
        if (fields.link) {
            arm64_branch_link_register(emitter, target_reg);
        } else {
            arm64_branch_register(emitter, target_reg);
        }
    } else {
        /* B/BL - branch to immediate offset */
        if (fields.link) {
            arm64_branch_link(emitter, offset_instr);
        } else {
            arm64_branch(emitter, offset_instr);
        }
    }
}

void arm32_translate_multiply(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc)
{
    /* Multiply instructions: implement later if needed */
    /* For PoC, emit NOP or skip */
}

/* ============================================================================
 * Block-level Translation
 * ============================================================================ */

TranslatedBlock* arm32_transpiler_translate_block(ARM32Transpiler *tr, uint32_t start_pc,
                                                  uint8_t *guest_memory)
{
    ARM64Emitter *emitter = tr->emitter;
    size_t code_start_pos = arm64_emitter_get_size(emitter);
    
    TranslatedBlock *block = malloc(sizeof(TranslatedBlock));
    if (!block) return NULL;
    
    block->arm32_start_pc = start_pc;
    block->arm64_code = arm64_emitter_get_code(emitter) + code_start_pos;
    block->exec_count = 0;
    
    uint32_t pc = start_pc;
    uint32_t *guest_instructions = (uint32_t *)guest_memory;
    size_t instr_count = 0;
    
    /* Translate instructions until we hit a branch or max block size */
    while (instr_count < tr->max_block_size) {
        /* Load instruction from guest memory */
        uint32_t encoding = guest_instructions[pc / 4];
        
        /* Decode instruction type */
        ARM32InstructionType type = arm32_identify_instruction(encoding);
        
        /* Translate instruction */
        switch (type) {
            case ARM32_DATA_PROCESSING:
                arm32_translate_data_processing(tr, encoding, pc);
                break;
                
            case ARM32_LOAD_STORE:
                arm32_translate_load_store(tr, encoding, pc);
                break;
                
            case ARM32_BRANCH:
                arm32_translate_branch(tr, encoding, pc);
                block->arm32_end_pc = pc + 4;
                goto done_block;  /* End block on branch */
                
            case ARM32_MULTIPLY:
                arm32_translate_multiply(tr, encoding, pc);
                break;
                
            default:
                /* Unknown or unimplemented instruction */
                break;
        }
        
        pc += 4;
        instr_count++;
    }
    
done_block:
    block->arm32_end_pc = pc;
    block->arm64_code_size = arm64_emitter_get_size(emitter) - code_start_pos;
    
    /* Update statistics */
    tr->total_blocks_translated++;
    tr->total_instructions_translated += instr_count;
    
    return block;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

TranslatedBlock* arm32_transpiler_get_or_translate(ARM32Transpiler *tr, uint32_t pc,
                                                   uint8_t *guest_memory)
{
    if (!tr->enable_caching) {
        return arm32_transpiler_translate_block(tr, pc, guest_memory);
    }
    
    /* Check cache first */
    TranslatedBlock *cached = arm32_transpiler_lookup_cache(tr, pc);
    if (cached) {
        if (tr->enable_profiling) {
            tr->cache_hits++;
        }
        return cached;
    }
    
    /* Translate new block */
    TranslatedBlock *block = arm32_transpiler_translate_block(tr, pc, guest_memory);
    if (block) {
        arm32_transpiler_insert_cache(tr, pc, block);
        if (tr->enable_profiling) {
            tr->cache_misses++;
        }
    }
    
    return block;
}

/* ============================================================================
 * Debugging & Statistics
 * ============================================================================ */

void arm32_transpiler_print_stats(ARM32Transpiler *tr)
{
    printf("=== ARM32->ARM64 Transpiler Statistics ===\n");
    printf("Total blocks translated: %u\n", tr->total_blocks_translated);
    printf("Total instructions translated: %u\n", tr->total_instructions_translated);
    printf("Cache size: %zu / %zu\n", tr->cache_size, tr->cache_capacity);
    
    if (tr->enable_profiling) {
        printf("Cache hits: %u\n", tr->cache_hits);
        printf("Cache misses: %u\n", tr->cache_misses);
        
        if (tr->cache_hits + tr->cache_misses > 0) {
            double hit_rate = (double)tr->cache_hits / (tr->cache_hits + tr->cache_misses) * 100.0;
            printf("Cache hit rate: %.2f%%\n", hit_rate);
        }
    }
    
    printf("Emitted code size: %zu bytes\n", arm64_emitter_get_size(tr->emitter));
    printf("Code buffer capacity: %zu bytes\n", tr->emitter->capacity);
}

void arm32_transpiler_set_profiling(ARM32Transpiler *tr, bool enable)
{
    tr->enable_profiling = enable;
}
