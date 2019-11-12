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

    struct RenameRecord {
        static const uint32_t DUPLICATE = 1 << 0;
        static const uint32_t KEEP =      1 << 1;
        std::string src;
        std::string dst;
        uint32_t flags = 0;
    };
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

    std::set<ino_t> duplicates;
    std::vector<std::unique_ptr<Transformer>> transformers;

    uint32_t apply_transformers(std::string &path, std::string &filename, Transformer::Type type) {
        uint32_t res = 0;
        for (auto &t: transformers) {
            res |= t->transform(path, filename, type);
        }
        return res;
    }

    std::set<std::string> dircreated;

    void push_rename(const std::string &src, const std::string &dst,
            Transformer::Type type, const uint32_t flags = 0)
    {
        if (type == Transformer::DIRNAME) {
            if (dircreated.count(dst)) {
                return;
            }
        }

        dircreated.insert(dst);
        RenameRecord rec;
        rec.src = src; rec.dst = dst; rec.flags = flags;
        renames.push_back(rec);
    }

    bool keepdups = false;
};

} // namespace s28

#endif /* RENAME_PARSER_H */
