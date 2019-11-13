#ifndef RENAMELOG_H
#define RENAMELOG_H

class RenameLog {
public:
    struct Event {
        enum Type {
            // particular inode not found, but there is a file with same content
            // which can be used instead
            MISSING_INODE, // warn

            // the source file in repo is missing
            UNKNOWN_SOURCE // error
        };

        Type type;
        ino_t inode = 0;
        std::string path;
        const char *position;
    }

    void warn();

    void missing_inode(ino_t inode, const std::string &path)

    void push_event(const Event &e) {
        events.push_back(e);
    }

    std::vector<Event> events;
};

#endif /* RENAMELOG_H */
