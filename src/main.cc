#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


#include <set>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>

#include <openssl/sha.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "node.h"
#include "dir.h"

#include "escape.h"
#include "error.h"
#include "hash.h"
#include "file.h"
#include "parser.h"
#include "utils.h"
#include "rename_parser.h"
#include "record.h"

namespace s28 {


namespace aux {
template<typename CB>
class Collector : public Traverse {
public:
    Collector(const Node *node, CB cb) : node(node), cb(cb) {}
    void walk(const Node *node) override {
        cb(node);
    }

private:
    const Node *node;
    CB cb;
};
}

class Progress {
public:
    virtual void tick(size_t n, size_t total) {}
};

template<typename CB>
void walk(const Node *node, CB cb) {
    aux::Collector<CB> collector(node, cb);
    node->traverse(collector);
}



namespace collector {


typedef std::vector<std::unique_ptr<Record>> Records;
typedef std::vector<std::unique_ptr<BaseRecord>> BaseRecords;


void find(const Node *n, Records &records) {
    std::unique_ptr<Record> rec(new Record());
    rec->node = n;
    records.push_back(std::move(rec));
}


template<typename RECORDS>
void stat(RECORDS &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        struct stat stt;
        std::string path = rec->node->get_path();
        if (::stat(path.c_str(), &stt) == -1) {
            RAISE_ERROR("stat failed; file=" << rec->node->get_path());
        }
        rec->inode = stt.st_ino;
    }
}

void hash(Records &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        const Node *n = rec->node;
        const File *f = dynamic_cast<const File *>(n);
        if (!f) continue;
        rec->hash = hash_file_short(f->get_path());
    }
}


void group_duplicates(Records &records, Progress &progress) {
    size_t cnt = 0;
    std::map<std::string, s28::collector::Record *> uniq;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        if (rec->hash.empty()) continue;
        auto it = uniq.find(rec->hash);
        if (it == uniq.end()) {
            uniq[rec->hash] = rec.get();
        } else {
            s28::collector::Record *repre = it->second->repre;
            if (repre) {
                rec->repre = repre;
                rec->next = repre->next;
            } else {
                repre = it->second;
                repre->repre = rec->repre = repre;
            }
            repre->next = rec.get();
        }
    }
}


template<typename REC>
class RecordsBuilderImpl : public Traverse {
public:
    typedef std::vector<std::unique_ptr<REC>> Records;

    RecordsBuilderImpl(Records &records) : records(records) {}
    void walk(const Node *node) override {
        std::unique_ptr<REC> rec(new REC());
        rec->node = node;
        records.push_back(std::move(rec));
    }

    void on_dir_end(const Node *n) override {
        records.back()->brackets ++;
    }

private:
    Records &records;
};


typedef RecordsBuilderImpl<Record> RecordsBuilder;
typedef RecordsBuilderImpl<BaseRecord> BaseRecordsBuilder;

}



} // namespace s28

std::string tabs(int n) {
    std::string rv;

    while(n > 0) {
        rv += "  ";
        n--;
    }
    return rv;
}

struct Args {
    bool dry = false;
    bool verbose = false;
    bool force = false;
    std::string renamefile;
    std::string renamerepo;
    std::string action;
    std::string prefix;
};


int search_rename_repo(const Args &args) {
    s28::Node::Config config;
    s28::Dir d(config, args.renamerepo, nullptr);
    d.build(config);

    s28::collector::Records records;
    s28::collector::RecordsBuilder rb(records);
    d.traverse_children(rb);

    s28::Progress progress;
    s28::collector::stat(records, progress);
    s28::collector::hash(records, progress);
    s28::collector::group_duplicates(records, progress);

    s28::Escaper es;

    int dep = 0;
    for (auto &rec: records) {
        auto * node = rec->node;

        if (node->is<s28::Dir>()) {
            const s28::Dir *dir = dynamic_cast<const s28::Dir *>(node);
            if (dir->get_children().empty()) {
                std::cout << tabs(dep) << es.escape(node->get_name()) << " {}";
            } else {
                std::cout << tabs(dep) << es.escape(node->get_name()) << " {";
                dep += 1;
            }
        } else {
            std::cout << tabs(dep) << es.escape(node->get_name()) << " #" << rec->inode;
            auto d = rec->repre;
            if (d) {
                while(d) {
                    if (d->inode != rec->inode) {
                        std::cout << "|" << d->inode;
                    }
                    d = d->next;
                }
            }
            std::cout << ";";
        }

        std::cout << std::endl;

        for (int i = 0; i < rec->brackets; ++i) {
            dep --;
            std::cout << tabs(dep) << "}" << std::endl;
        }
    }
    return 0;
}

int apply_rename(const Args &args) {
    s28::Node::Config config;
    s28::Dir d(config, args.renamerepo, nullptr);
    d.build(config);

    s28::collector::BaseRecords records;
    s28::collector::BaseRecordsBuilder rb(records);
    d.traverse(rb);


    s28::Progress progress;
    s28::collector::stat(records, progress);

    s28::RenameParser::InodeMap inomap;
    for (auto &r: records) {
        inomap[r->inode] = r.get();
    }

    s28::RenameParser::LogEvents logs;
    s28::RenameParser::RenameRecords renames;

    s28::RenameParser rp(inomap, logs, renames);
    rp.parse(args.renamefile);

    bool ok = true;
    for (auto &log: logs) {
        std::cerr << log.str() << std::endl;
        ok = false;
    }

    if (!ok && !args.force) return 1;

    s28::Escaper es;
    for (auto &rename: renames) {
        if (rename.src.empty()) {
            std::cout << "mkdir -p " << args.prefix  << es.escape(rename.dst) << std::endl;
        } else {
            if ((rename.flags & s28::RenameParser::RenameRecord::DUPLICATE)
                    && !(rename.flags & s28::RenameParser::RenameRecord::KEEP)) {
                std::cout << "# ";
            }
            std::cout << "ln " << es.escape(rename.src) << " " <<
                  args.prefix << es.escape(rename.dst) << std::endl;
        }
    }

    return 0;
}


bool parse_args(Args &args, int argc, char **argv) {
    using namespace boost::program_options;
    options_description desc{"Options"};

    try {
        desc.add_options()
            ("action,a", value<std::string>(&args.action)->required(), "apply | load")
            ("help,h", "Help screen")
            ("dry-run,d", bool_switch(&args.dry), "dry run")
            ("verbose,v", bool_switch(&args.verbose), "verbose")
            ("rename-file,f", value<std::string>(&args.renamefile)->default_value(".rename"), "rename input file")
            ("rename-repo,r", value<std::string>(&args.renamerepo)->default_value(".renameRepo"), "rename repository name")
            ("force", bool_switch(&args.force), "force")
            ("prefix", value<std::string>(&args.prefix), "output file path prefix")
            ;

        positional_options_description posop;
        posop.add("action", 1);

        variables_map vm;
        store(command_line_parser(argc, argv).options(desc)
                      .positional(posop).run(),
                    vm);
        notify(vm);

        if (vm.count("help")) RAISE_ERROR("Usage");
        if (args.action != "apply" && args.action != "load")
            RAISE_ERROR("invalid --apply argument");
    } catch(const std::exception &e) {
        std::cout << "err: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return false;
    }
    return true;
}

#include "filename_parser.h"

int main(int argc, char **argv) {

    s28::FileNameParser fnp("%n_%N.%e");
    std::cout << "[" << fnp.parse("configlog.asdf.fff") << "]" << std::endl;;
    std::cout << "[" << fnp.parse("configlog.asdf.fff") << "]" << std::endl;;
    std::cout << "[" << fnp.parse("configlog.asdf.fff") << "]" << std::endl;;
    std::cout << "[" << fnp.parse("configlog.asdf.fff") << "]" << std::endl;;

    return 0;


    Args args;
    try {
        if (!parse_args(args, argc, argv)) return 1;
        if (args.action == "load") {
            search_rename_repo(args);
        } else if (args.action == "apply") {
            apply_rename(args);
        }
    } catch(const std::exception &e) {
        std::cerr << "err:" << e.what() << std::endl;
        return 1;
    }
    return 0;
}

