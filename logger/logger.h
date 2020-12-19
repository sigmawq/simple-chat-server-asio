//
// Created by swql on 12/19/20.
// Simple logger, that writes provided messages with timestamps to file, automatically created
// by logger in folder path specified by user. Define LOGGER_TO_COUT to also output messages to
// cout.
//

#ifndef LOGGER_LOGGER_H
#define LOGGER_LOGGER_H
#include <string>
#include <filesystem>
#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef LOGGER_TO_COUT
#define LCOUT(str) std::cout << str
#define LCOUT_NL(str) std::cout << str << std::endl
#define PUSH_NL() std::cout << std::endl;
#else
#define LCOUT(str)
#define LCOUT_NL(str)
#define PUSH_NL()
#endif


class logger {
    std::ofstream handle;

    // Name will be log<current_date_and_time>.txt
    static std::string get_timestamp(bool spacefree = false);

    void create_new_log_file(std::filesystem::path& path);

    void handle_provided_path(std::filesystem::path& path);

    void initialize(std::filesystem::path& path);

    // Base case
    template<typename none = void>
    void push_same_line() {
        PUSH_NL();
        handle << '\n';
        handle.flush();
    }

    template<typename Head, typename... Tail>
    void push_same_line(Head head, Tail... messages){
        LCOUT(head);
        handle << head;
        push_same_line(messages...);
    }
public:

    // Log file will created in folder "path"
    logger(std::filesystem::path& path);

    logger(std::filesystem::path path);

    // Log file will be created in current directory
    logger();

    logger(const logger& other) = delete;

    // Push a message to logger
    template<typename Head, typename... Tail >
    void push(Head h, Tail... t){
        LCOUT("[");
        auto ts = get_timestamp();
        LCOUT(ts);
        LCOUT("]: ");

        handle << "[" << ts << "]: ";
        push_same_line(h, t...);
    }

    // Push a message with provided prefix
    template<typename Prefix, typename Head, typename... Tail >
    void pushp(Prefix p, Head h, Tail... t){
        LCOUT("[");
        auto ts = get_timestamp();
        LCOUT(ts);
        LCOUT(", ");
        LCOUT(p);
        LCOUT("]: ");

        handle << "[" << ts << ", " << p << "]: ";
        push_same_line(h, t...);
    }
};


#endif //LOGGER_LOGGER_H
