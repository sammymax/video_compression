#include <cstdio>
#include <mutex>

typedef int __tsan_atomic32;
typedef unsigned long long uptr;

std::mutex mutex;
#define LOCK() std::lock_guard<std::mutex> lock(mutex)


static int count = 0;
extern "C" void __tsan_read1(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_read2(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_read4(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_read8(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_read16(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_write1(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_write2(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_write4(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_write8(void*p __attribute__((unused))) {
    count++;
}
extern "C" void __tsan_write16(void*p __attribute__((unused))) {
    count++;
}

extern "C" void __tsan_read_range(void *addr __attribute((unused)), uptr size __attribute((unused))) {
    count++;
}
extern "C" void __tsan_write_range(void *addr __attribute((unused)), uptr size __attribute((unused))) {
    count++;
}

extern "C" void __tsan_func_exit(void) {  
}
extern "C" void __tsan_func_entry(void *addr __attribute__((unused))) {
}


typedef enum {
  __tsan_memory_order_relaxed = 1 << 0,
  __tsan_memory_order_consume = 1 << 1,
  __tsan_memory_order_acquire = 1 << 2,
  __tsan_memory_order_release = 1 << 3,
  __tsan_memory_order_acq_rel = 1 << 4,
  __tsan_memory_order_seq_cst = 1 << 5,
} __tsan_memory_order;

extern "C" __tsan_atomic32 __tsan_atomic32_fetch_add(volatile __tsan_atomic32 *a,
                                          __tsan_atomic32 v, __tsan_memory_order mo) {
    LOCK();
    printf("a=%p *a=%d v=%d mo=%d\n", a, *a, v, mo);
    __tsan_atomic32 old = *a;
    (*a) += v;
    return old;
}

extern "C" void __tsan_vptr_update(void **vptr_p __attribute__((unused)), void *new_val __attribute__((unused))) {
}

extern "C" void __tsan_init(void) {
    // called for each trhead
}
