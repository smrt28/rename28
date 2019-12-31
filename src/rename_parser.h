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
#include "error.h"

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
    // Path modificators
    PathContext dir_context;
    PathContext file_context;

    GlobalRenameContext global_context;
    const InodeMap &inomap;
    RenameRecords &renames;

    // recursive descent parsing
    ino_t parse_inodes(std::set<ino_t> *inodes);

    // recursive descent parser functions
    bool parse_file_or_dir(RenameParserContext &ctx);
    void parse_dir_content();
    void parse_dir();
    void parse_file(RenameParserContext &ctx);

    struct CommandsContext {
        std::string duppattern;
        ApplyPattern *pattern = nullptr;
    };

    void parse_commands(CommandsContext &ctx);
    //

    // parsing context
    parser::Parslet pars;

    std::vector<std::string> dirchain; // the current dirrectory chain (path)
    std::set<ino_t> duplicates; // set of created file inodes

    void rename_file(const std::string &src, uint32_t flags, RenameParserContext &ctx) {
        RenameRecord rec;
        rec.src = src;
        DirChain v;
        int dups = 0;
        static const int LIMIT = 1000;
        for (int i = 0; i < LIMIT; i++) {
            if (file_context.build(dirchain, v, ctx, dups)) {
                dups = global_context.nodes.insert(v);
                if (global_context.nodes.count(v) > 1) {
                    std::cerr << "dup: " << boost::algorithm::join(v, "/") << std::endl;
                    continue;
                }
                rec.dst = boost::algorithm::join(v, "/");
                rec.flags = flags;
                renames.push_back(rec);
            }
            return;
        }
        RAISE_ERROR("too many duplocates1");
    }

    void create_directory(RenameParserContext &ctx) {
        if (dirchain.empty()) return;
        std::string path;

        DirChain v;
        if (dir_context.build(dirchain, v, ctx, 0)) {
            if (global_context.nodes.count(v)) return;
            RenameRecord rec;
            std::string path = boost::algorithm::join(v, "/");
            rec.dst = path;
            renames.push_back(rec);
            global_context.nodes.insert(v);
        }
    }

    bool keepdups = false;
};

} // namespace s28

#endif /* RENAME_PARSER_H */
