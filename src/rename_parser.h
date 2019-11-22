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
    struct RenameRecord {
        static const uint32_t DUPLICATE = 1 << 0;
        static const uint32_t KEEP =      1 << 1;
        std::string src;
        std::string dst;
        uint32_t flags = 0;
    };
    typedef std::map<ino_t, s28::collector::BaseRecord *> InodeMap;
    typedef std::vector<RenameRecord> RenameRecords;


    RenameParser(InodeMap &inomap, std::vector<RenameRecord> &renames);

    void parse(const std::string &inputfile);


private:

    PathContext dir_context;
    PathContext file_context;


    const InodeMap &inomap;
    RenameRecords &renames;

    // recursive descent parsing
    ino_t parse_inodes(std::set<ino_t> *inodes);


    bool parse_file_or_dir(RenameParserContext &ctx);
    void parse_dir_content();
    void parse_dir();
    void parse_file(RenameParserContext &ctx);
    void parse_commands();

    parser::Parslet pars;

    std::vector<std::string> dirchain; // the current dirrectory chain (path)
    std::set<ino_t> duplicates; // set of created file inodes
    std::set<std::string> dircreated; // already created directory paths

    void rename_file(const std::string &src, uint32_t flags, RenameParserContext &ctx) {
        RenameRecord rec;
        rec.src = src;
        std::string path;
        if (file_context.build(dirchain, path, ctx)) {
            rec.dst = path;
            rec.flags = flags;
            renames.push_back(rec);
        }
    }

    void create_directory(RenameParserContext &ctx) {
        if (dirchain.empty()) return;
        std::string path;

        if (dir_context.build(dirchain, path, ctx)) {
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
