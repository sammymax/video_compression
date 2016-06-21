#include "simpletsan.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>

// A preliminary test of running "run_test.sh" shows that incrementing
// count_reads and count_writes always gives the same answer, so I'm
// going to assume that the program under test is single threaded.

// To do stuff
//    
//   cd bin
//   rm TAppEncoderStatic;
//   make
//   ./run_test

class SPbags {

    struct UnionFindRecord {
        int depth;
        int rep;
        UnionFindRecord(int rep) : depth(0), rep(rep) {} 
    };

    // The unionfind data structure.  if unionfind[i]==i then i
    // represents itself, otherwise i is represented by the
    // representative of unionfind[i].
    // Also, unionfind.size() is the next free bag id.
    std::vector<UnionFindRecord>  unionfind; 
    
    // For each representative, keep track of whether it's a P bag or an S bag.
    std::vector<bool> is_pbag_map;

    // The stack of S bags and P bags.
    //  -1 represents the empty set.
    std::vector<int> sf; 
    std::vector<int> pf;

    struct AddressRecord {
        int reader, writer;
        AddressRecord(int reader, int writer) : reader(reader), writer(writer) {}
    };
    std::unordered_map<uint64_t, AddressRecord> shadowspace;

    int find(int a) {
        if (a == -1) return -1;
        if (unionfind[a].rep == a) return a;
        int rep = find(unionfind[a].rep);
        unionfind[a].rep = rep;
        return rep;
    }
    int do_union(int a, int b) {
        if (a == -1) return b;
        if (b == -1) return a;
        if (a == b) return a;
        int arep = find(a);
        int brep = find(b);
        assert(arep >= 0 && brep >= 0);
        if (arep == brep) return arep;
        if (unionfind[arep].depth < unionfind[brep].depth) {
            unionfind[arep].rep = brep;
            return brep;
        } else if (unionfind[arep].depth > unionfind[brep].depth) {
            unionfind[brep].rep = arep;
            return arep;
        } else {
            unionfind[arep].rep = brep;
            unionfind[brep].depth++;
            return brep;
        }
    }

    bool is_pbag(int bag) {
        if (bag == -1) return false; // no bag means its serial
        return is_pbag_map[bag];
    }
    AddressRecord* get_address_record(uint64_t addr) {
        auto pair = shadowspace.emplace(std::make_pair(-1,AddressRecord(-1, -1)));
        return &pair.first->second;
    }
    void read_byte(uint64_t addr) {
        assert(sf.size() > 0);
        AddressRecord *r = get_address_record(addr);
        if (is_pbag(find(r->writer))) {
            printf("Race exists on 0x%016lx\n", addr);
        }
        if (!is_pbag(find(r->reader))) {
            r->reader = sf[sf.size()-1];
        }
    }
    void write_byte(uint64_t addr) {
        AddressRecord *r = get_address_record(addr);
        if (is_pbag(find(r->reader)) ||
            is_pbag(find(r->writer))) {
            printf("Race exists on 0x%016x\n", addr);
        }
        r->writer = sf[sf.size()-1];
    }
  public:
    SPbags() { 
        unionfind.push_back(UnionFindRecord(0));
        is_pbag_map.push_back(false);
        sf.push_back(0);
        pf.push_back(-1);
    }
    void spawnproc() {
        int new_set = unionfind.size();
        unionfind.push_back(UnionFindRecord(new_set));
        sf.push_back(new_set);
        pf.push_back(-1);
        is_pbag_map.push_back(false);
    }
    void syncproc() {
        assert(sf.size() > 0);
        uint64_t lastindex = sf.size()-1;
        int bag = do_union(sf[lastindex], pf[lastindex]);
        sf[lastindex] = bag;
        pf[lastindex] = -1;
        is_pbag_map[bag] = false;
    }
    void returnproc() {
        uint64_t lastindex = sf.size()-1;
        assert(lastindex > 1);
        assert(pf[lastindex] == -1); // the pbag should always be empty when returning
        int bag = do_union(pf[lastindex - 1], sf[lastindex]);
        pf.pop_back();
        pf[lastindex - 1] = bag;
        sf.pop_back();
        is_pbag_map[bag] = true;
    }
    void read_range(uint64_t addr, uint64_t size) {
        for (uint64_t i = 0; i < size; i++) {
            read_byte(addr+i);
        }
    }
    void write_range(uint64_t addr, uint64_t size) {
        for (uint64_t i = 0; i < size; i++) {
            write_byte(addr+i);
        }
    }
};

static SPbags *bags = nullptr;
static bool in_tsan = false;
class InTsan {
  public:
    InTsan () {
        assert(!in_tsan);
        in_tsan = true;
    }
    ~InTsan () {
        assert(in_tsan);
        in_tsan = false;
    }
};

static void report();

static std::atomic<uint64_t> did_init(0);
extern "C" void __tsan_init() {
    InTsan it;
    printf("initing\n");
    if (did_init++ == 0) {
        printf("doing init\n");
        assert(bags == nullptr);
        bags = new SPbags;
        atexit(report);
    }
}

extern "C" void simple_tsan_note_parallel() {
    if (in_tsan) return;
    InTsan it;
}
extern "C" void simple_tsan_note_sync() {
    if (in_tsan) return;
    InTsan it;
    bags->syncproc();
}
extern "C" void simple_tsan_note_spawn() {
    if (in_tsan) return;
    InTsan it;
    bags->spawnproc();
}
extern "C" void simple_tsan_note_continue() {
    if (in_tsan) return;
    InTsan it;
    bags->returnproc();
}

extern "C" void __tsan_read_range(void *addr, unsigned long size) { 
    if (in_tsan) return;
    InTsan it;
    bags->read_range(reinterpret_cast<uint64_t>(addr), size);
}
extern "C" void __tsan_write_range(void *addr, unsigned long size) {
    if (in_tsan) return;
    InTsan it;
    bags->write_range(reinterpret_cast<uint64_t>(addr), size);
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
}

#if 0



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
#endif
