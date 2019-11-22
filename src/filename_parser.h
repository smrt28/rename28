#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include <string>
#include <vector>

namespace s28 {

class RenameParserContext;

class FileNameParser {
    static const int TOKEN_STRING = 1;
    static const int TOKEN_WILDCARD = 2;
public:
    FileNameParser(const std::string &rawpatern);
    std::string parse(const std::string &fname, const RenameParserContext &ctx);

    struct Wildcard {
        int type;
        size_t c;
        size_t arg;
    };

    struct String {
        int type;
        const char *it;
        const char *eit;
    };

    union Token {
        int type;
        String str;
        Wildcard wld;
    };


    std::vector<Token> patern;
    std::string rawpatern;

    int cnt = 0;
};

} // namespace s28

#endif /* FILENAMEPARSER_H */
