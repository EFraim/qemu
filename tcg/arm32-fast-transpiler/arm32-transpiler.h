/*
 * ARM32 to ARM64 Fast Transpiler - Main Translator Module
 * 
 * Core translation logic: converts ARM32 instruction streams to ARM64 code.
 */

#ifndef ARM32_TRANSPILER_H
#define ARM32_TRANSPILER_H

#include <stdint.h>
#include <stddef.h>
#include "arm32-decoder.h"
#include "arm64-emitter.h"

/* Translation block metadata */
typedef struct {
    uint32_t arm32_start_pc;  /* Starting PC of ARM32 block */
    uint32_t arm32_end_pc;    /* Ending PC of ARM32 block */
    uint8_t *arm64_code;      /* Pointer to generated ARM64 code */
    size_t arm64_code_size;   /* Size of generated ARM64 code in bytes */
    uint32_t exec_count;      /* Execution count (for profiling) */
} TranslatedBlock;

/* Translation cache entry */
typedef struct {
    uint32_t pc;
    TranslatedBlock *block;
} TranslationCacheEntry;

/* Main transpiler context */
typedef struct {
    ARM64Emitter *emitter;        /* ARM64 code generator */
    TranslationCacheEntry *cache; /* Translation block cache */
    size_t cache_size;
    size_t cache_capacity;
    
    /* Statistics */
    uint32_t total_blocks_translated;
    uint32_t total_instructions_translated;
    uint32_t cache_hits;
    uint32_t cache_misses;
    
    /* Configuration */
    size_t max_block_size;        /* Maximum instructions per block */
    bool enable_caching;
    bool enable_profiling;
} ARM32Transpiler;

/* ============================================================================
 * Transpiler Lifecycle
 * ============================================================================ */

/**
 * Create a new ARM32->ARM64 transpiler instance
 * @param code_buffer_size: Size of emitted code buffer (bytes)
 * @return: Initialized transpiler context
 */
ARM32Transpiler* arm32_transpiler_create(size_t code_buffer_size);

/**
 * Destroy transpiler and free all resources
 */
void arm32_transpiler_destroy(ARM32Transpiler *tr);

/**
 * Reset transpiler state (clear cache, reset code buffer)
 */
void arm32_transpiler_reset(ARM32Transpiler *tr);

/* ============================================================================
 * Translation API
 * ============================================================================ */

/**
 * Translate an ARM32 basic block to ARM64
 * @param tr: Transpiler context
 * @param start_pc: Starting program counter
 * @param guest_memory: Pointer to guest memory (ARM32 code location)
 * @return: Translated block, or NULL on error
 */
TranslatedBlock* arm32_transpiler_translate_block(ARM32Transpiler *tr, uint32_t start_pc,
                                                  uint8_t *guest_memory);

/**
 * Get or translate a block (uses cache if available)
 */
TranslatedBlock* arm32_transpiler_get_or_translate(ARM32Transpiler *tr, uint32_t pc,
                                                   uint8_t *guest_memory);

/**
 * Clear the translation cache
 */
void arm32_transpiler_flush_cache(ARM32Transpiler *tr);

/* ============================================================================
 * Individual Instruction Translators
 * ============================================================================ */

/**
 * Translate ARM32 data processing instruction to ARM64
 */
void arm32_translate_data_processing(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc);

/**
 * Translate ARM32 load/store instruction to ARM64
 */
void arm32_translate_load_store(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc);

/**
 * Translate ARM32 branch instruction to ARM64
 */
void arm32_translate_branch(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc);

/**
 * Translate ARM32 multiply instruction to ARM64
 */
void arm32_translate_multiply(ARM32Transpiler *tr, uint32_t encoding, uint32_t pc);

/* ============================================================================
 * State Management (Flags, Registers, etc.)
 * ============================================================================ */

/**
 * Handle condition code evaluation at runtime
 * For conditional instructions, generates code to check flags
 */
void arm32_emit_condition_check(ARM32Transpiler *tr, uint8_t cond, uint8_t target_reg);

/**
 * Update PSTATE flags after instruction
 * (if instruction sets flags, generate code to update ARM64 NZCV)
 */
void arm32_emit_flags_update(ARM32Transpiler *tr, bool sets_flags);

/* ============================================================================
 * Statistics & Debugging
 * ============================================================================ */

/**
 * Print transpiler statistics
 */
void arm32_transpiler_print_stats(ARM32Transpiler *tr);

/**
 * Enable/disable profiling
 */
void arm32_transpiler_set_profiling(ARM32Transpiler *tr, bool enable);

#endif /* ARM32_TRANSPILER_H */
