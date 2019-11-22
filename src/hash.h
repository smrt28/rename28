#ifndef HASH_H
#define HASH_H
#include <string>
#include <vector>

namespace s28 {
    typedef std::pair<uint64_t, uint64_t> hash128_t;
    hash128_t hash_dirchain(const std::vector<std::string> &v);
    std::string hash_file(const std::string &path);
    std::string hash_file_short(const std::string &path);
    int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize);

}

#endif
