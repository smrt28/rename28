#ifndef RENAME_PARSER_H
#define RENAME_PARSER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/algorithm/string/join.hpp>

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


    void dumpchain() {
        for (auto &s: dirchain) {
            std::cerr << s << " ";
        }
        std::cerr << std::endl;

    }

private:
    const InodeMap &inomap;
    LogEvents &log;
    RenameRecords &renames;
    int dep = 0;


    // recursive descent parsing
    ino_t read_inodes(std::set<ino_t> *inodes);
    bool read_file_or_dir();
    void read_dir_content();
    void read_dir();
    void read_file();
    void update_context();

    parser::Parslet pars;

    std::vector<std::string> dirchain;

    std::set<ino_t> duplicates;


    std::set<std::string> dircreated;

    void push_rename_file(const std::string &src, uint32_t flags) {
        RenameRecord rec;
        rec.src = src;
        rec.dst = boost::algorithm::join(dirchain, "/");
        rec.flags = flags;
        renames.push_back(rec);
    }

    void push_create_directory() {
        if (dirchain.empty()) return;
        std::string path;
        path = boost::algorithm::join(dirchain, "/");
        if (dircreated.count(path)) return;
        dircreated.insert(path);
        RenameRecord rec;
        rec.dst = path;
        renames.push_back(rec);
    }

    bool keepdups = false;
};

} // namespace s28

#endif /* RENAME_PARSER_H */
