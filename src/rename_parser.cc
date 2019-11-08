#include <set>
#include <map>

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "rename_parser.h"
#include "escape.h"
#include "utils.h"
#include "node.h"

namespace s28 {
namespace {
std::string read_escaped_string(parser::Parslet &p) {
    parser::ltrim(p);
    parser::Parslet orig = p;

    if (*p == '\'') {
        p.skip();
        while (*p != '\'') {
            if (p.next() == '\\') {
                if (p.next() == 'x') {
                    p.skip();
                }
                p.skip();
                continue;
            }
        }
        p.skip();
    } else {
        while (!isspace(*p)) {
            if (p.next() == '\\') {
                if (p.next() == 'x') {
                    p.skip();
                }
                p.skip();
                continue;
            }
        }
    }
    Escaper es;
    return es.unescape(parser::Parslet(orig.begin(), p.begin()).str());
}
} // namespace


ino_t RenameParser::read_inodes(parser::Parslet &p, std::set<ino_t> *inodes) {
    ino_t firstino = 0;
    for(;;) {
        while(isdigit(*p)) {
            ino_t ino = boost::lexical_cast<ino_t>(parser::number(p));
            if (!firstino) {
                firstino = ino;
            }
            if (inodes) inodes->insert(ino);
        }
        if (*p != '|') break;
        p.skip();
    }
    return firstino;
}


void RenameParser::update_context(parser::Parslet &p, Context &ctx) {
   p.expect_char('$');
   const char *it = p.begin();
   parser::Parslet command;
   for (;;) {
       p++;
       if (*p == '\n') {
           command = parser::Parslet(it, p.begin());
           break;
       }
   }

   parser::trim(command);

   if (command.str() == "numbers") {
       ctx.numbername.reset(new int(0));
   }

   if (command.str() == "flatten") {
       std::cerr << "flat" << std::endl;
      ctx.flatten = true;
   }
}

bool RenameParser::read_file_or_dir(parser::Parslet &p, const std::string &prefix, Context &ctx) {
    if (p.empty() || *p == '}') return false;
    if (*p == '$') {
        update_context(p, ctx);
        return true;
    }
    std::string filename;
    filename = read_escaped_string(p);

    utils::sanitize_filename(filename);
    std::string path;
    parser::ltrim(p);
    if (ctx.numbername && *p == '#') {
        filename = std::to_string(*ctx.numbername);
        (*ctx.numbername)++;
    }
    if (prefix.empty()) {
        path = filename;
    } else {
        path = prefix + "/" + filename;
    }
    switch(*p) {
        case '{':
            if (ctx.flatten) {
                read_dir(p, prefix, ctx);
            } else {
                read_dir(p, path, ctx);
            }
            return true;
        case '#':

            read_file(p, path, ctx);
            return true;

    }
    RAISE_ERROR("expected file or dir");
}

void RenameParser::read_dir_content(parser::Parslet &p, const std::string &prefix, Context &ctx) {
    parser::ltrim(p);
    while(read_file_or_dir(p, prefix, ctx)) {
        parser::ltrim(p);
    }
}


void RenameParser::read_dir(parser::Parslet &p, const std::string &prefix, Context &ctx) {
    Context newctx;
    ctx.inherit(newctx);
    p.expect_char('{');
    if (newctx.mkdir) renames.push_back(std::make_pair("", prefix));
    read_dir_content(p, prefix, newctx);
    p.expect_char('}');
}


void RenameParser::read_file(parser::Parslet &p, const std::string &path, Context &ctx) {
    p.expect_char('#');
    std::set<ino_t> inodes;

    read_inodes(p, &inodes);

    bool found = false;
    for (ino_t ino : inodes) {
        auto it = inomap.find(ino);
        if (it != inomap.end()) {
            renames.push_back(std::make_pair(it->second->node->get_path(), path));
            found = true;
            break;
        } else {
            LogEvent l;
            l.inode = ino;
            l.path = path;
            l.code = LogEvent::Code::MISSING_INODE;
            log.push_back(l);
        }
    }
    if (!found) {
        LogEvent l;
        l.path = path;
        l.code = LogEvent::Code::UNKNOWN_SOURCE;
        log.push_back(l);
    }
    saves += inomap.size() - 1;
    while (!p.empty() && *p != '\n' && *p != ';') p.skip();
    p.skip();
}


void RenameParser::parse(const std::string &inputfile) {
    saves = 0;
    std::ifstream is (inputfile, std::ifstream::binary);

    if (!is) {
        RAISE_ERROR("can't open reaname-file: " << inputfile);
    }

    std::string str((std::istreambuf_iterator<char>(is)),
            std::istreambuf_iterator<char>());

    parser::Parslet p(str);
    Context ctx;
    read_dir_content(p, "", ctx);
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
