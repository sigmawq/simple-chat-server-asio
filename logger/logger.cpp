//
// Created by swql on 12/19/20.
//

#include "logger.h"

void logger::handle_provided_path(std::filesystem::path &path) {
    {
        try{
            if (!std::filesystem::exists(path)) {
                std::filesystem::create_directory(path);
            }
            if (std::filesystem::is_directory(path)){
                create_new_log_file(path);
            }
            else{
                throw std::runtime_error("Provided path is not a directory");
            }
        }
        catch (std::runtime_error& err){
            std::cout << "[Logger error on init]: " << err.what() << std::endl;
        }

    }
}

void logger::create_new_log_file(std::filesystem::path &path) {
    auto datetime = get_timestamp(true);
    path.append("log" + datetime + ".txt");
    this->handle.open(path);
    if (!this->handle.good()) { throw std::runtime_error("Error while creating file"); }
}

logger::logger(std::filesystem::path& path)
{
    initialize(path);
}

logger::logger(std::filesystem::path path) {
    initialize(path);
}

logger::logger() {
    std::filesystem::path path {"./"};
    initialize(path);
}

std::string logger::get_timestamp(bool spacefree) {
    time_t time = std::time(NULL);
    tm* tm_struct = std::localtime(&time);
    std::string datetime(std::asctime(tm_struct));
    datetime.pop_back();
    if (spacefree) {
        // Remove-erase idiom
        std::replace(datetime.begin(), datetime.end(), ' ', '-');
    }
    return datetime;
}

void logger::initialize(std::filesystem::path &path) {
    {
        try{
            handle_provided_path(path);
        }
        catch (std::runtime_error& err){
            std::cout << "Logger failed to create/use a directory path provided by: " << path
                      << std::endl;
        }
    }
}
