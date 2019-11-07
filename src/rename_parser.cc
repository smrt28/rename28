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


bool RenameParser::read_file_or_dir(parser::Parslet &p, const std::string &prefix) {
    if (p.empty() || *p == '}') return false;

    std::string filename = read_escaped_string(p);
    utils::sanitize_filename(filename);
    std::string path;
    if (prefix.empty()) {
        path = filename;
    } else {
        path = prefix + "/" + filename;
    }
    parser::ltrim(p);
    switch(*p) {
        case '{':
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


void RenameParser::read_dir(parser::Parslet &p, const std::string &prefix) {
    p.expect_char('{');
    read_dir_content(p, prefix);
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
            renames.push_back(std::make_pair(path, it->second->node->get_path()));
            std::cout << "[" << path << "] <- [" <<
                it->second->node->get_path() <<
                "] #" << ino << std::endl;
            found = true;
            break;
        } else {
            std::cerr << "warn: " << path << ": missing inode=" << ino << std::endl;
        }
    }
    if (!found) {
        std::cerr << "err: " << path << "; is missing" << std::endl;
    }
    saves += inomap.size() - 1;
    while (*p != '\n') p.skip();
}


void RenameParser::parse(const std::string &inputfile) {
    saves = 0;
    std::ifstream is (inputfile, std::ifstream::binary);
    std::string str((std::istreambuf_iterator<char>(is)),
            std::istreambuf_iterator<char>());

    parser::Parslet p(str);
    read_dir_content(p, "");
}


} // namespace s28
