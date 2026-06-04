# ARM32 to ARM64 Fast Transpiler - Build Configuration

## Overview

This module provides a direct ARM32 (AArch32) to ARM64 (AArch64) instruction transpiler for QEMU. 
It bypasses TCG's intermediate representation layer to achieve higher performance through direct 
1:1 instruction mapping.

## Building the PoC

### Standalone Test Harness

To build and test the transpiler without full QEMU integration:

```bash
# From the QEMU root directory
cd tcg/arm32-fast-transpiler

# Compile the test harness
gcc -o test-transpiler \
    arm32-decoder.c \
    arm64-emitter.c \
    arm32-transpiler.c \
    test-transpiler.c \
    -Wall -Wextra -O2

# Run tests
./test-transpiler
```

### With QEMU Integration

To integrate into QEMU's build system, the module can be conditionally compiled:

```bash
./configure --enable-arm32-fast-transpiler

make
```

## Architecture

### Core Components

1. **arm32-decoder.h/c**: ARM32 instruction decoding and field extraction
   - Decodes ARM32 opcodes and extracts operand fields
   - Identifies instruction types (data processing, load/store, branch, etc.)
   - Supports 32-bit ARM instruction encoding (ARMv7)

2. **arm64-emitter.h/c**: ARM64 code generation
   - Emits native ARM64 instructions into a code buffer
   - Provides high-level functions for common operations (ADD, LDR, STR, B, etc.)
   - Manages instruction encoding directly

3. **arm32-transpiler.h/c**: Main translation logic
   - Orchestrates decoder and emitter
   - Manages translation block cache
   - Handles instruction-level translation
   - Provides profiling and statistics

### Translation Flow

```
ARM32 instruction stream
        ↓
[arm32_decoder] → Extract operand fields
        ↓
[arm32_transpiler] → Select translation strategy
        ↓
[arm64_emitter] → Generate ARM64 code
        ↓
ARM64 executable code buffer
```

## Instruction Coverage

### Currently Supported

- **Data Processing**: ADD, SUB, MOV, AND, ORR, EOR
- **Load/Store**: LDR, STR (register and immediate offset)
- **Branch**: B, BL, BX, BLX
- **Shifts**: LSL, LSR, ASR (basic support)

### Future Support

- Multiply instructions (MUL, MLA, etc.)
- Load/Store Multiple (LDM, STM)
- Coprocessor operations
- Conditional execution optimization
- Thumb instruction encoding
- NEON/Advanced SIMD operations

## Performance Characteristics

### Expected Benefits

- **Minimal translation overhead**: Direct instruction mapping avoids IR interpretation
- **Efficient caching**: Translated blocks cached for reuse
- **Low memory footprint**: Generated code is compact (typically 1:1 ARM32→ARM64 ratio)

### Benchmarking

Run profiling to measure performance:

```c
ARM32Transpiler *tr = arm32_transpiler_create(1024*1024);
arm32_transpiler_set_profiling(tr, true);

// ... run translations ...

arm32_transpiler_print_stats(tr);
```

## Integration with QEMU

### User-Space Integration (qemu-arm)

Hook into the CPU execution loop in `qemu/accel/tcg/cpu-exec.c`:

```c
#ifdef USE_ARM32_FAST_TRANSPILER
TranslationBlock* tb = arm32_transpiler_get_or_translate(
    thread_local_transpiler,
    cpu->env.regs[15],  /* PC */
    cpu->env.guest_mem
);
#endif
```

### System-Mode Integration (qemu-system-arm)

For full system emulation, integration requires:
1. MMU address translation support
2. Device memory region handling
3. Interrupt coordination
4. Privileged instruction handling

Currently focused on user-space (qemu-arm) for PoC simplicity.

## Testing

Run the included test suite:

```bash
./test-transpiler
```

Expected output includes:
- Decoder validation (instruction type identification)
- Emitter validation (ARM64 code generation)
- Transpiler validation (end-to-end translation)
- Cache efficiency metrics

## Known Limitations

1. **ARM32 Conditional Execution**: Not yet fully optimized; may require runtime checks
2. **Flag Updates**: CPSR→PSTATE mapping needs more work
3. **Register Encoding**: Assumes AArch32 registers map 1:1 to AArch64 (w0-w31)
4. **Instruction Set Coverage**: ~30% of ARMv7 instructions currently supported
5. **Privilege Levels**: User-space only; no EL0/EL1 distinction yet

## References

- ARM Architecture Reference Manual ARMv7-A and ARMv7-R (DDI 0406C)
- ARM Architecture Reference Manual ARM64 (DDI 0487)
- QEMU Internals Documentation: https://wiki.qemu.org/Documentation/Internals
- TCG Documentation: https://wiki.qemu.org/Documentation/TCG

## Future Work

1. Expand instruction coverage to >80% of ARMv7
2. Optimize conditional instruction handling
3. Implement Thumb instruction support
4. Add NEON/SIMD translation
5. Optimize memory access sequences
6. Implement dynamic code patching for frequently-used sequences
7. Full system-mode support with MMU
8. Integration with QEMU's performance profiling

## Contributing

For bug reports, feature requests, or improvements:
1. Test with `test-transpiler`
2. Add test cases for new instructions
3. Validate against real ARM32 binaries
4. Benchmark before/after changes
