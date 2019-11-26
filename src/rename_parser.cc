#include <set>
#include <map>

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "rename_parser.h"
#include "utils.h"
#include "node.h"

namespace s28 {


RenameParser::RenameParser(InodeMap &inomap, std::vector<RenameRecord> &renames) :
    inomap(inomap),
    renames(renames)
{}



ino_t RenameParser::parse_inodes(std::set<ino_t> *inodes) {
    ino_t firstino = 0;
    for(;;) {
        while(isdigit(*pars)) {
            ino_t ino = boost::lexical_cast<ino_t>(parser::number(pars));
            if (!firstino) {
                firstino = ino;
            }
            if (inodes) inodes->insert(ino);
        }
        if (*pars != '|') break;
        pars.skip();
    }
    return firstino;
}

void RenameParser::parse_commands()
{
   pars.expect_char('$');
   const char *it = pars.begin();
   parser::Parslet command;
   for (;;) {
       pars++;
       if (*pars == '\n' || *pars == ';') {
           command = parser::Parslet(it, pars.begin());
           pars.skip();
           parser::ltrim(pars);
           break;
       }
   }

   parser::trim(command);

   std::string cmd = parser::word(command);

   if (cmd == "flatten") {
       dir_context.push(new DirFlattener(dirchain.size()));
       file_context.push(new FileFlattener(dirchain.size()));
       return;
   }

   if (cmd == "pattern") {
       std::string pattern = parser::trim(command).str();
       file_context.push(new ApplyPattern(pattern));
       return;
   }

   if (cmd == "ascii") {
       file_context.push(new CharFilter());
       return;
   }

   RAISE_ERROR("unknown command: " << cmd);
}

bool RenameParser::parse_file_or_dir(RenameParserContext &ctx) {
    if (pars.empty() || *pars == '}') return false;
    if (*pars == '$') RAISE_ERROR("command must be at the directory beggining");
    std::string filename;
    if (*pars != '#')
        filename = parser::read_escaped_string(pars);

    parser::ltrim(pars);

    switch(*pars) {
        case '{':
            dirchain.push_back(filename);
            ctx.dirorder++;
            create_directory(ctx);
            parse_dir();
            dirchain.pop_back();
            return true;
        case '#':
            dirchain.push_back(filename);
            ctx.fileorder++;
            parse_file(ctx);
            dirchain.pop_back();
            return true;

    }
    RAISE_ERROR("expected file or dir");
}

void RenameParser::parse_dir_content() {
    parser::ltrim(pars);

    size_t dsize = dir_context.size();
    size_t fsize = file_context.size();

    // read commands to the commands stack
    while (!pars.empty() && *pars == '$') {
        parse_commands();
    }

    RenameParserContext ctx(global_context);
    while(parse_file_or_dir(ctx)) {
        parser::ltrim(pars);
    }

    dir_context.pop_to(dsize);
    file_context.pop_to(fsize);
}



void RenameParser::parse_dir() {
    pars.expect_char('{');
    parse_dir_content();
    pars.expect_char('}');
}


void RenameParser::parse_file(RenameParserContext &ctx) {
    pars.expect_char('#');
    std::set<ino_t> inodes;

    parse_inodes(&inodes);

    bool found = false;
    for (ino_t ino : inodes) {
        auto it = inomap.find(ino);
        if (it != inomap.end()) {
            uint32_t flags = 0;
            if (duplicates.count(ino)) {
                flags = RenameParser::RenameRecord::DUPLICATE;
                if (keepdups) flags |= RenameParser::RenameRecord::KEEP;
            } else {
                duplicates.insert(ino);
            }
            rename_file(it->second->node->get_path(), flags, ctx);
            found = true;
            break;
        } else {
            // inode not found
        }
    }
    if (!found) {
        // cant copy the file which is not in repo
    }
    while (!pars.empty() && *pars != '\n' && *pars != ';') pars.skip();
    pars.skip();
}


void RenameParser::parse(const std::string &inputfile) {
    std::ifstream is (inputfile, std::ifstream::binary);

    if (!is) {
        RAISE_ERROR("can't open reaname-file: " << inputfile);
    }

    std::string str((std::istreambuf_iterator<char>(is)),
            std::istreambuf_iterator<char>());

    pars = parser::Parslet(str);
    parse_dir_content();
}

} // namespace s28
