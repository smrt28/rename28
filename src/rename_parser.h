#ifndef RENAME_PARSER_H
#define RENAME_PARSER_H

#include <string>
#include <map>
#include <vector>

#include "parser.h"

namespace s28 {

class RenameParser {
public:
    typedef std::pair<std::string, std::string> RenameRecord;
    typedef std::map<ino_t, s28::collector::BaseRecord *> InodeMap;
    std::vector<RenameRecord> renames;

    RenameParser(InodeMap &inomap) :
        inomap(inomap)
    {}

    void parse(const std::string &inputfile);

private:
    int saves = 0;
    InodeMap &inomap;

    ino_t read_inode(parser::Parslet &p);
    void read_inodes(parser::Parslet &p, ino_t &firstino, std::set<ino_t> &inodes);
    void read_dir_content(parser::Parslet &p, const std::string &prefix);

};

} // namespace s28

#endif /* RENAME_PARSER_H */
