#ifndef RENAME_PARSER_H
#define RENAME_PARSER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include <string>
#include <map>
#include <vector>
#include <set>

#include "parser.h"
#include "record.h"

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

    // recursive descent parsing
    ino_t read_inodes(parser::Parslet &p, std::set<ino_t> *inodes);
    bool read_file_or_dir(parser::Parslet &p, const std::string &prefix);
    void read_dir_content(parser::Parslet &p, const std::string &prefix);
    void read_dir(parser::Parslet &p, const std::string &prefix);
    void read_file(parser::Parslet &p, const std::string &path);
};

} // namespace s28

#endif /* RENAME_PARSER_H */
