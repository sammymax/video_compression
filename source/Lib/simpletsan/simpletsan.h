extern "C" {
    void simple_tsan_note_parallel();
    void simple_tsan_note_sync();
    void simple_tsan_note_spawn();
    void simple_tsan_note_continue();
#if 0
    void __tsan_init();
    void __tsan_func_entry(void *pc);
    void __tsan_func_exit();
    void __tsan_read1(void *addr);
    void __tsan_read2(void *addr);
    void __tsan_read4(void *addr);
    void __tsan_read8(void *addr);
    void __tsan_read16(void *addr);
    void __tsan_read_range(void *addr, unsigned long size);
    void __tsan_write1(void *addr);
    void __tsan_write2(void *addr);
    void __tsan_write4(void *addr);
    void __tsan_write8(void *addr);
    void __tsan_write16(void *addr);
    void __tsan_write_range(void *addr, unsigned long size);
    void __tsan_vptr_update(void **vptr_p, void *new_val);
#endif
};
