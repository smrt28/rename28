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
{
       file_context.push(1, new ApplyPattern("%F%.%D"));
       return;
}



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

void RenameParser::parse_commands(CommandsContext &ctx)
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
       dir_context.push(0, new DirFlattener(dirchain.size()));
       file_context.push(0, new FileFlattener(dirchain.size()));
       return;
   }

   if (cmd == "duppattern") {
       std::string duppattern = parser::trim(command).str();
       if (ctx.pattern) {
           ctx.pattern->duppattern = duppattern;
       } else {
           ctx.duppattern = duppattern;
       }

       return;
   }

   if (cmd == "pattern") {
       std::string pattern = parser::trim(command).str();
       std::unique_ptr<ApplyPattern> p(new ApplyPattern(pattern));
       if (!ctx.duppattern.empty()) {
           p->duppattern = ctx.duppattern;
           ctx.pattern = p.get();
       }

       file_context.push(1, p.release());
       return;
   }

   if (cmd == "ascii") {
       file_context.push(2, new CharFilter());
       return;
   }

   if (cmd == "keepdups") {
       keepdups = true;
       return;
   }

   RAISE_ERROR("unknown command: " << cmd);
}

bool RenameParser::parse_file_or_dir(RenameParserContext &ctx) {
    if (pars.empty() || *pars == '}') return false;

    /*
    if (*pars == '/' && pars[1] != '/') {
        while (!pars.empty() && *pars != '\n') {}
        return true;
    }
    */

    if (*pars == '$') {
        RAISE_ERROR("command must be at the directory beggining");
    }


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
    CommandsContext cctx;

    auto dsize = dir_context.state();
    auto fsize = file_context.state();
    bool keepdups_bak = keepdups;

    // read commands to the commands stack
    while (!pars.empty() && *pars == '$') {
        parse_commands(cctx);
    }

    RenameParserContext ctx(global_context);
    while(parse_file_or_dir(ctx)) {
        parser::ltrim(pars);
    }

    dir_context.restore(dsize);
    file_context.restore(fsize);
    keepdups = keepdups_bak;
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
        if (it == inomap.end()) {
            RAISE_ERROR("inode #" << ino << " not found");
        }

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
