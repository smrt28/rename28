#include <set>
#include <map>

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "rename_parser.h"
#include "utils.h"
#include "node.h"

namespace s28 {


RenameParser::RenameParser(InodeMap &inomap, std::vector<LogEvent> &log, std::vector<RenameRecord> &renames) :
    dir_path_builder(new DirPathBuilder),
    file_path_builder(new FilePathBuilder),
    inomap(inomap),
    log(log),
    renames(renames)
{

}



ino_t RenameParser::read_inodes(std::set<ino_t> *inodes) {
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

void RenameParser::update_context(std::vector<std::function<void()>> &revstack)
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

   if (command.str() == "flatten") {
       DirPathBuilder *dorig = dir_path_builder.release();
       FilePathBuilder *forigin = file_path_builder.release();
       dir_path_builder.reset(new DirFlattener(dirchain.size(), dorig));
       file_path_builder.reset(new FileFlattener(dirchain.size(), forigin));

       revstack.push_back([this, dorig, forigin]() {
               dir_path_builder.reset(dorig);
               file_path_builder.reset(forigin);
       });
       return;
   }

   if (command.str() == "numbers") {
       FilePathBuilder *forig = file_path_builder.release();
       file_path_builder.reset(new FileNumerator(forig));
       revstack.push_back([this, forig]() {
           file_path_builder.reset(forig);
       });
       return;
   }
}

bool RenameParser::read_file_or_dir(Context &ctx) {
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
            push_create_directory();
            read_dir();
            dirchain.pop_back();
            return true;
        case '#':
            dirchain.push_back(filename);
            ctx.fileorder++;
            read_file();
            dirchain.pop_back();
            return true;

    }
    RAISE_ERROR("expected file or dir");
}

void RenameParser::read_dir_content() {
    parser::ltrim(pars);

    std::vector<std::function<void()>> revstack;

    // read commands to the commands stack
    while (!pars.empty() && *pars == '$') {
        update_context(revstack);
    }

    Context ctx;
    while(read_file_or_dir(ctx)) {
        parser::ltrim(pars);
    }

    for (auto &f: revstack) f();
}



void RenameParser::read_dir() {
    pars.expect_char('{');
    read_dir_content();
    pars.expect_char('}');
}


void RenameParser::read_file() {
    pars.expect_char('#');
    std::set<ino_t> inodes;

    read_inodes(&inodes);

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
            push_rename_file(it->second->node->get_path(), flags);
            found = true;
            break;
        } else {
            /*
            LogEvent l;
            l.inode = ino;
            l.path = path;
            l.code = LogEvent::Code::MISSING_INODE;
            log.push_back(l);
            */
        }
    }
    if (!found) {
        /*
        LogEvent l;
        l.path = path;
        l.code = LogEvent::Code::UNKNOWN_SOURCE;
        log.push_back(l);
        */
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
    read_dir_content();
}


std::string RenameParser::LogEvent::str() {
    std::ostringstream oss;

    std::string msg;
    switch (code) {
        case NONE:
            msg = "N/A";
            break;
        case MISSING_INODE:
            oss << "warn: missing inode; path=" << path << "; inode=" << inode;
            break;
        case UNKNOWN_SOURCE:
            oss << "err: unknown source; path=" << path;
            break;
        default:
            oss << "err: not defined";
    }


    return oss.str();
}


} // namespace s28
