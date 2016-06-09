#include "simpletsan.h"

extern "C" void simple_tsan_note_parallel() {}
extern "C" void simple_tsan_note_sync() {}
extern "C" void simple_tsan_note_spawn() {}
extern "C" void simple_tsan_note_continue() {}

extern "C" void __tsan_init() {}
extern "C" void __tsan_func_entry(void *pc __attribute__((unused))) {}
extern "C" void __tsan_func_exit() {}
extern "C" void __tsan_read1(void *addr) {}
extern "C" void __tsan_read2(void *addr) {}
extern "C" void __tsan_read4(void *addr) {}
extern "C" void __tsan_read8(void *addr) {}
extern "C" void __tsan_read16(void *addr) {}
extern "C" void __tsan_read_range(void *addr, unsigned long size) {}
extern "C" void __tsan_vptr_update(void **vptr_p, void *new_val) {}
extern "C" void __tsan_write1(void *addr) {}
extern "C" void __tsan_write2(void *addr) {}
extern "C" void __tsan_write4(void *addr) {}
extern "C" void __tsan_write8(void *addr) {}
extern "C" void __tsan_write16(void *addr) {}
extern "C" void __tsan_write_range(void *addr, unsigned long size) {}
