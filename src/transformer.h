#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

typedef std::vector<std::string> DirChain;

namespace cmd {

class Command {
    public:
        Command() {}
        virtual void update(DirChain &chain) = 0;
};


class Flatten : public Command {
    public:
    Flatten(size_t dep): dep(dep) {}
    void update(DirChain &chain) override {
        while(chain.size() > dep) chain.pop_back();
    }
    private:
        size_t dep;
};





} // namespace cmd
} // namespace s28

#endif /* TRRANSFORMER_H */
