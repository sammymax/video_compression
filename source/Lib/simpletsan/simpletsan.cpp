#include "simpletsan.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <map>

static __thread uint64_t depth = 0;
static uint64_t spawn_depth = 0;
static uint64_t spawn_count = 0;
static std::atomic<uint64_t>  max_depth(0);

// A preliminary test of running "run_test.sh" shows that incrementing
// count_reads and count_writes always gives the same answer, so I'm
// going to assume that the program under test is single threaded.
uint64_t count_reads = 0;
uint64_t nested_reads = 0;
uint64_t count_writes = 0;
uint64_t nested_writes = 0;

//std::vector<Bag> Sbags;
//std::vector<Bag> Bbags;

struct AddressRecord {
    uint64_t end;
    AddressRecord(uint64_t end) : end(end) {}
};
std::map<uint64_t, AddressRecord> *all_addresses = nullptr;

extern "C" void simple_tsan_note_parallel() {
    depth++;
    {
        uint64_t prev_value = max_depth.load();
        while (prev_value < depth 
               && !max_depth.compare_exchange_strong(prev_value, depth));
    }
    //fprintf(stderr, "%s\n", __FUNCTION__);
}
extern "C" void simple_tsan_note_sync() {
    depth--;
    //fprintf(stderr, "%s\n", __FUNCTION__);
}
extern "C" void simple_tsan_note_spawn() {
    //fprintf(stderr, "%*s%s\n", depth, "", __FUNCTION__);
    spawn_depth++;
    spawn_count++;
}
extern "C" void simple_tsan_note_continue() {
    //fprintf(stderr, "%*s%s\n", depth, "", __FUNCTION__);
    spawn_depth--;
}


static std::atomic<uint64_t> did_init(0);

static void report();

extern "C" void __tsan_init() {
    if (did_init++ == 0) {
        all_addresses = new std::map<uint64_t, AddressRecord>();
        atexit(report);
    }
}

void maxf(uint64_t &location, const uint64_t val) {
    location = std::max(location, val);
}

void merge_with_next(std::map<uint64_t, AddressRecord> *addresses, const std::map<uint64_t, AddressRecord>::iterator &it) {
    auto next_it = it;
    ++next_it;
    while (1) {
        if (next_it == addresses->end()) return; // there is no next record
        if (it->second.end < next_it->first) return; // the next record doesn't overlap.
        if (0) printf(" next: %016lx-%016lx overlaps with %016lx-%016lx\n",
                      it->first, it->second.end,
                      next_it->first, next_it->second.end);
        maxf(it->second.end, next_it->second.end);
        next_it = addresses->erase(next_it);
    }
}

void merge_with_prev(std::map<uint64_t, AddressRecord> *addresses, std::map<uint64_t, AddressRecord>::iterator it) {
    while (1) {
        if (it == addresses->begin()) return;  // There is no previous record.
        //printf("There is a prev record\n");
        auto prev_it = it;
        --prev_it;
        if (prev_it->second.end < it->first) return; // the previous record doesn't overlap.
        if (0) printf(" prev: %016lx-%016lx overlaps with %016lx-%016lx\n",
                      prev_it->first, prev_it->second.end,
                      it->first, it->second.end);
        maxf(prev_it->second.end, it->second.end);
        addresses->erase(it);
        it = prev_it;
    }
}

void add_to_addresses(std::map<uint64_t, AddressRecord> *addresses, void *addr, uint64_t size) 
// Effect: addresses is a set of address ranges, add a new range,
// merging with an existing range or ranges if necessary.
{
    const uint64_t addr_i = reinterpret_cast<uint64_t>(addr);
    const uint64_t end_i  = addr_i + size;
    // First insert the new record
    //printf("inserting %016lx-%016lx\n", addr_i, end_i);
    auto pair = addresses->insert(std::pair<uint64_t, AddressRecord>(addr_i, AddressRecord(end_i)));
    auto &it = pair.first;
    if (!pair.second) {
        // a record with the same starting address already existed.
        it->second.end = std::max(it->second.end, end_i);
        //printf(" same starting address, new record is %016lx-%016lx\n", it->first, it->second.end);
    } else {
        //printf(" inserted new record\n");
    }
    merge_with_next(addresses, it);
    merge_with_prev(addresses, it);
    if (0) {
        printf("records:");
        for ( auto t : *addresses ) {
            printf(" 0x%016lx-0x%016lx", t.first, t.second.end);
        }
        printf("\n");
    }
}

extern "C" void __tsan_read_range(void *addr, unsigned long size) { 
    count_reads++; 
    if (spawn_depth) {
        nested_reads++;
    }
    add_to_addresses(all_addresses, addr, size);
    //assert(Sbags.size() == depth);
    //if (depth == 0) return; // no need to do anything

}
extern "C" void __tsan_write_range(void *addr, unsigned long size) {
    count_writes++;
    if (spawn_depth) {
        nested_writes++;
    }
    add_to_addresses(all_addresses, addr, size);
}

extern "C" void __tsan_func_entry(void *pc __attribute__((unused))) {}
extern "C" void __tsan_func_exit() {}
extern "C" void __tsan_read1(void *addr)  { __tsan_read_range(addr, 1); }
extern "C" void __tsan_read2(void *addr)  { __tsan_read_range(addr, 2); }
extern "C" void __tsan_read4(void *addr)  { __tsan_read_range(addr, 4); }
extern "C" void __tsan_read8(void *addr)  { __tsan_read_range(addr, 8); }
extern "C" void __tsan_read16(void *addr) { __tsan_read_range(addr, 16); }
extern "C" void __tsan_vptr_update(void **vptr_p, void *new_val) {}
extern "C" void __tsan_write1(void *addr) { __tsan_write_range(addr, 1); }
extern "C" void __tsan_write2(void *addr) { __tsan_write_range(addr, 2); }
extern "C" void __tsan_write4(void *addr) { __tsan_write_range(addr, 4); }
extern "C" void __tsan_write8(void *addr) { __tsan_write_range(addr, 8); }
extern "C" void __tsan_write16(void *addr){ __tsan_write_range(addr, 16); }

static void report() {
    fprintf(stderr, "simple tsan report\n");
    fprintf(stderr, " Max cilk depth = %d\n", max_depth.load());
    fprintf(stderr, " spawn count    = %d\n", spawn_count);
    fprintf(stderr, " n_reads  = %12ld  nested = %12ld\n", count_reads, nested_reads);
    fprintf(stderr, " n_writes = %12ld  nested = %12ld\n", count_writes, nested_writes);
    fprintf(stderr, " n_unique address regions = %ld\n", all_addresses->size());
    uint64_t n_addresses = 0;
    for (const auto &pair : *all_addresses) {
        assert(pair.first < pair.second.end);
        n_addresses += pair.second.end - pair.first;
    }
    fprintf(stderr, " n_unique addresses       - %ld\n", n_addresses);
}
