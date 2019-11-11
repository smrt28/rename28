#ifndef RENAME_PARSER_H
#define RENAME_PARSER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include <string>
#include <map>
#include <vector>
#include <set>
#include <memory>

#include "transformer.h"
#include "parser.h"
#include "record.h"

namespace s28 {

class RenameParser {
public:
    class LogEvent {
    public:
        enum Code {
            NONE,
            MISSING_INODE,
            UNKNOWN_SOURCE
        };

        bool is_error() const {
            switch(code) {

                case UNKNOWN_SOURCE:
                    return true;
                default: return false;
            }
        }

        Code code = NONE;
        ino_t inode = 0;
        std::string path;
        std::string str();
    };

    typedef std::pair<std::string, std::string> RenameRecord;
    typedef std::map<ino_t, s28::collector::BaseRecord *> InodeMap;
    typedef std::vector<LogEvent> LogEvents;
    typedef std::vector<RenameRecord> RenameRecords;


    RenameParser(InodeMap &inomap, std::vector<LogEvent> &log, std::vector<RenameRecord> &renames) :
        inomap(inomap),
        log(log),
        renames(renames)
    {}

    void parse(const std::string &inputfile);

private:
    const InodeMap &inomap;
    LogEvents &log;
    RenameRecords &renames;
    int dep = 0;


    // recursive descent parsing
    ino_t read_inodes(parser::Parslet &p, std::set<ino_t> *inodes);
    bool read_file_or_dir(parser::Parslet &p, const std::string &prefix);
    void read_dir_content(parser::Parslet &p, const std::string &prefix);
    void read_dir(parser::Parslet &p, const std::string &prefix);
    void read_file(parser::Parslet &p, const std::string &path);
    void update_context(parser::Parslet &p, const std::string &prefix);

    std::vector<std::unique_ptr<Transformer>> transformers;

    std::string apply_transformers(const std::string &filename, Transformer::Type type) {
        std::string rv = filename;
        for (auto &t: transformers) {
            rv = t->transform(rv, type);
        }
        return rv;
    }
};

} // namespace s28

#endif /* RENAME_PARSER_H */
