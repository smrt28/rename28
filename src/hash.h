#ifndef HASH_H
#define HASH_H

namespace s28 {
    std::string hash_file(const std::string &path);
    std::string hash_file_short(const std::string &path);
    int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize);

}

#endif
