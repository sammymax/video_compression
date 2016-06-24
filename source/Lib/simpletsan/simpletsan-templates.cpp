/* Templates needed by simpletsan.  We want these instrumented in case the application use them */
/* We compile the tool with -fno-implicit-templates.  If the application doesn't follow suit, that will be OK. */
/* Don't use -fno-implicit-templates here, or you'll get the endless set of templates needed by these templates. */
#include "simpletsan.h"
#include <unordered_map>
#include <vector>

// I cannot figure out what template to instantiate for that "auto" variable, so I'll just cheat and implemen it thus.
AddressRecord* tsan_get_address_record (std::unordered_map<uint64_t, AddressRecord> &shadowspace, uint64_t addr) {
    auto pair = shadowspace.emplace(std::make_pair(addr, AddressRecord(-1, -1)));
    return &pair.first->second;
}

template class std::unordered_map<uint64_t, AddressRecord>;
template class std::unordered_map<uint64_t, uint64_t>;
template class std::vector<int>;
template class std::vector<UnionFindRecord>;
