#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <openssl/sha.h>
#include <string>
#include <errno.h>

#include "error.h"

namespace s28 {
namespace {
class FileDescriptorGuard {
public:
    FileDescriptorGuard(int fd) : fd(fd) {}
    ~FileDescriptorGuard() {
        if (fd >= 0) ::close(fd);
    }
    int fd;
};

} // namespace

std::string hash_file(const std::string &path) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("err:");
        RAISE_ERROR("open for reading; file=" << path);
    }
    FileDescriptorGuard guard(fd);

    char buf[4096];
    ssize_t len;

    do {
        len = read(fd, buf, sizeof(buf));
        if (len < 0)
            RAISE_ERROR("error while reading; file=" << path);

        SHA256_Update(&sha256, buf, len);

    } while(len == sizeof(buf));
    SHA256_Final(hash, &sha256);


    return std::string((char *)hash, sizeof(hash));
}


int base32_encode(const uint8_t *data, int length, uint8_t *result,
        int bufSize) {
    if (length < 0 || length > (1 << 28)) {
        return -1;
    }
    int count = 0;
    if (length > 0) {
        int buffer = data[0];
        int next = 1;
        int bitsLeft = 8;
        while (count < bufSize && (bitsLeft > 0 || next < length)) {
            if (bitsLeft < 5) {
                if (next < length) {
                    buffer <<= 8;
                    buffer |= data[next++] & 0xFF;
                    bitsLeft += 8;
                } else {
                    int pad = 5 - bitsLeft;
                    buffer <<= pad;
                    bitsLeft += pad;
                }
            }
            int index = 0x1F & (buffer >> (bitsLeft - 5));
            bitsLeft -= 5;
            result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
        }
    }
    if (count < bufSize) {
        result[count] = '\000';
    }
    return count;
}


std::string hash_file_short(const std::string &path) {
    std::string h = hash_file(path);
    char buf[256];
    base32_encode((const uint8_t *)h.c_str(), 32, (uint8_t *)buf, sizeof(buf));
    return std::string(buf, 15);
}

}

