#include <set>
#include <map>

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include "rename_parser.h"
#include "utils.h"
#include "node.h"

namespace s28 {

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


void RenameParser::update_context(parser::Parslet &p, const std::string &prefix) {
   p.expect_char('$');
   const char *it = p.begin();
   parser::Parslet command;
   for (;;) {
       p++;
       if (*p == '\n' || *p == ';') {
           command = parser::Parslet(it, p.begin());
           p.skip();
           parser::ltrim(p);
           break;
       }
   }

   parser::trim(command);

   if (command.str() == "numbers") {
       std::unique_ptr<Transformer> t(new tformer::Numbers(dep));
       transformers.push_back(std::move(t));
   } else if (command.str() == "flatten") {
       std::unique_ptr<Transformer> t(new tformer::Flatten(dep, prefix));
       transformers.push_back(std::move(t));
   } else if (command.str() == "keepdups") {
       keepdups = true;
   } else if (command.str() == "dropdups") {
       keepdups = false;
   } else {
       RAISE_ERROR("invalid command");
   }
}

bool RenameParser::read_file_or_dir(parser::Parslet &p, const std::string &prefix) {
    if (p.empty() || *p == '}') return false;
    while (!p.empty() && *p == '$') {
        update_context(p, prefix);
        return true;
    }

    std::string filename;
    if (*p != '#')
        filename = parser::read_escaped_string(p);

    std::string path = prefix;
    parser::ltrim(p);

    switch(*p) {
        case '{':
            apply_transformers(path, filename, Transformer::DIRNAME);
            break;
        case '#':
            apply_transformers(path, filename, Transformer::FILENAME);
            break;
        default:
            RAISE_ERROR("read_file_or_dir");
    }

    if (path.empty()) {
        path = filename;
    } else {
        if (!filename.empty()) {
            utils::sanitize_filename(filename);
            path = path + "/" + filename;
        }
    }

    switch(*p) {
        case '{':
            push_rename("", path, Transformer::DIRNAME);
            read_dir(p, path);
            return true;
        case '#':
            read_file(p, path);
            return true;

    }
    RAISE_ERROR("expected file or dir");
}

void RenameParser::read_dir_content(parser::Parslet &p, const std::string &prefix) {
    parser::ltrim(p);
    while(read_file_or_dir(p, prefix)) {
        parser::ltrim(p);
    }
}


namespace {
template<typename Op>
void pop(std::vector<std::unique_ptr<Op>> &op, int dep) {
    if (op.empty()) return;
    while(!op.empty() && op.back()->dep == dep) {
        op.pop_back();
    }

}
}

void RenameParser::read_dir(parser::Parslet &p, const std::string &prefix) {
    p.expect_char('{');
    dep++;
    read_dir_content(p, prefix);
    pop(transformers, dep);
    dep--;
    p.expect_char('}');
}


void RenameParser::read_file(parser::Parslet &p, const std::string &path) {
    p.expect_char('#');
    std::set<ino_t> inodes;

    read_inodes(p, &inodes);

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
            push_rename(it->second->node->get_path(), path, Transformer::FILENAME, flags);
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
    while (!p.empty() && *p != '\n' && *p != ';') p.skip();
    p.skip();
}


void RenameParser::parse(const std::string &inputfile) {
    std::ifstream is (inputfile, std::ifstream::binary);

    if (!is) {
        RAISE_ERROR("can't open reaname-file: " << inputfile);
    }

    std::string str((std::istreambuf_iterator<char>(is)),
            std::istreambuf_iterator<char>());

    parser::Parslet p(str);
    dep = 0;
    read_dir_content(p, "");
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
