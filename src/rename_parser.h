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
#include "path_context.h"

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


    RenameParser(InodeMap &inomap, std::vector<LogEvent> &log, std::vector<RenameRecord> &renames);

    void parse(const std::string &inputfile);


private:

    PathContext dir_context;
    PathContext file_context;


    const InodeMap &inomap;
    LogEvents &log;
    RenameRecords &renames;

    // recursive descent parsing
    ino_t read_inodes(std::set<ino_t> *inodes);

    struct Context {
        size_t dirorder = 0;
        size_t fileorder = 0;
    };

    bool read_file_or_dir(Context &ctx);
    void read_commands();
    void read_dir_content();
    void read_dir();
    void read_file();
    void update_context();

    parser::Parslet pars;

    std::vector<std::string> dirchain; // the current dirrectory chain (path)
    std::set<ino_t> duplicates; // set of created file inodes
    std::set<std::string> dircreated; // already created directory paths

    void rename_file(const std::string &src, uint32_t flags) {
        RenameRecord rec;
        rec.src = src;
        std::string path;
        if (file_context.build(dirchain, path)) {
            rec.dst = path;
            rec.flags = flags;
            renames.push_back(rec);
        }
    }

    void create_directory() {
        if (dirchain.empty()) return;
        std::string path;

        if (dir_context.build(dirchain, path)) {
            if (dircreated.count(path)) return;
            dircreated.insert(path);
            RenameRecord rec;
            rec.dst = path;
            renames.push_back(rec);
        }
    }

    bool keepdups = false;
};

} // namespace s28

#endif /* RENAME_PARSER_H */
