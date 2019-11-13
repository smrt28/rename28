#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

typedef std::vector<std::string> DirChain;

namespace cmd {

class Command {
    public:
        virtual void update_chain(DirChain &dirchain) {}
};


class Flatten {


};





} // namespace cmd
} // namespace s28

#endif /* TRRANSFORMER_H */
