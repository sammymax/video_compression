#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <execinfo.h>
#include <unistd.h>

typedef int __tsan_atomic32;
typedef unsigned long long uptr;

std::mutex mutex;
#define LOCK() std::lock_guard<std::mutex> lock(mutex)


static unsigned long count = 0;

static void do_backtrace() {
    const int N = 100;
    void *array[N];
    size_t size;
    size = backtrace(array, N);
    char **strings = backtrace_symbols(array, size);
    printf("backtrace for count = %ld\n", count);
    for (size_t i = 0; i < size; i++) {
        printf(" %s\n", strings[i]);
    }
    free(strings);
}

static void maybe_print_backtrace() {
    if (count == 143274162 || count == 212254962)
        do_backtrace();
}

// prev_w is the count value for the first write previous to the current parallel loop iteration.
// prev_r is the count value for the first read  previous to the current parallel loop iteration.
// this_w is the count value for the first write in the current parallel loop iteration.
// this_r is for read.
static std::unordered_map<unsigned long long, unsigned long> prev_w, prev_r, this_w, this_r;
static bool in_loop = false;

static const unsigned long long stack_low = 0x7ffffffd8000LL;
static       unsigned long long stack_hi  = 0x7ffffffff000LL;

extern "C" void __tsan_start_parallel_iteration() {
    if (!in_loop) {
        in_loop = true;
        stack_hi = (long long)__builtin_frame_address(0);
        prev_w.clear();
        prev_r.clear();
        this_w.clear();
        this_r.clear();
    } else {
        for (const auto &p : this_w) {
            const auto &k = std::get<0>(p);
            const auto &c  = std::get<1>(p);
            if (prev_w.count(k) == 0) {
                prev_w[k] = c;
            }
        }
        this_w.clear();
        for (const auto &p : this_r) {
            const auto &k = std::get<0>(p);
            const auto &c  = std::get<1>(p);
            if (prev_r.count(k) == 0) {
                prev_r[k] = c;
            }
        }
        this_r.clear();
    }
}
extern "C" void __tsan_after_parallel_loop() {
    in_loop = false;
    printf("count=%ld\n", count);
}
static void note_read1(unsigned long up) {
    if (stack_low <= up && up < stack_hi) return; // don't instrument things in the part of the stack that's allocated inside the cilk_for.
    if (prev_w.count(up) != 0) {
        printf("read/read race: counts=%ld %ld p=0x%lx\n", prev_w[up], count, up);
        abort();
    }
    if (this_r.count(up) == 0) {
        this_r[up] = count;
    }
}
static void note_read(const void *p, const size_t size) {
    if (in_loop) {
        maybe_print_backtrace();
        unsigned long up = reinterpret_cast<unsigned long>(p);
        for (size_t i = 0; i < size; i++) {
            note_read1(up+i);
        }
    }
    count++;
}
static void note_write1(unsigned long up) {
    if (stack_low <= up && up < stack_hi) return; // don't instrument things in the part of the stack that's allocated inside the cilk_for
    if (prev_r.count(up) != 0) {
        printf("read/write race: counts=%ld %ld p=0x%lx\n", prev_r[up], count, up);
        abort();
    }
    if (prev_w.count(up) != 0) {
        printf("write/write race: counts=%ld %ld p=0x%lx\n", prev_w[up], count, up);
        exit(1);
    }

    if (this_w.count(up) == 0) {
        this_w[up] = count;
    }
}

static void note_write(const void *p, const size_t size) {
    if (in_loop) {
        maybe_print_backtrace();
        unsigned long up = reinterpret_cast<unsigned long>(p);
        for (size_t i = 0; i < size; i++) {
            note_write1(up+i);
        }
    }
    count++;
}

extern "C" void __tsan_read1(void*p) {
    note_read(p, 1);
}
extern "C" void __tsan_read2(void*p) {
    note_read(p, 2);
}
extern "C" void __tsan_read4(void*p) {
    note_read(p, 4);
}
extern "C" void __tsan_read8(void*p) {
    note_read(p, 8);
}
extern "C" void __tsan_read16(void*p) {
    note_read(p, 16);
}
extern "C" void __tsan_write1(void*p) {
    note_write(p, 1);
}
extern "C" void __tsan_write2(void*p) {
    note_write(p, 2);
}
extern "C" void __tsan_write4(void*p) {
    note_write(p, 4);
}
extern "C" void __tsan_write8(void*p) {
    note_write(p, 8);
}
extern "C" void __tsan_write16(void*p) {
    note_write(p, 16);
}

extern "C" void __tsan_read_range(void *addr, uptr size) {
    note_read(addr, size);
}
extern "C" void __tsan_write_range(void *addr, uptr size) {
    note_write(addr, size);
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
    //printf("a=%p *a=%d v=%d mo=%d\n", a, *a, v, mo);
    __tsan_atomic32 old = *a;
    (*a) += v;
    return old;
}

extern "C" void __tsan_vptr_update(void **vptr_p __attribute__((unused)), void *new_val __attribute__((unused))) {
}

extern "C" void __tsan_init(void) {
    // called for each trhead
}

extern "C" void __tsan_print_info(void) {
    printf("counter=%ld\n", count);
}
