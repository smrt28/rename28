#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

typedef std::vector<std::string> DirChain;

namespace cmd {
namespace filename {

class Base {
public:
    virtual std::string get_file_name(const std::string &) = 0;
};

class Identity : public Base {
public:
    std::string get_file_name(const std::string &s) {
        return s;
    }
};

} // namespace filename
} // namespace cmd
} // namespace s28

#endif /* TRRANSFORMER_H */
