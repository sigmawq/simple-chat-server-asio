//
// Created by swql on 12/12/20.
/*
 * User action - is a command invoked by user. Each action is defined by a name and a corresponding
 * function with the signature: void(*)(ip::tcp::socket*, std::vector<std::string>& args)
 * =======================================================
 * A set of common user actions used by both server and client.
 * User action is always forwarded to a general action
 */
//

#ifndef SIMPLE_CHAT_SERVER_USER_ACTIONS_H
#define SIMPLE_CHAT_SERVER_USER_ACTIONS_H
#include <boost/asio.hpp>
#include "defs.h"

using namespace boost;

namespace scs{

    [[nodiscard]] inline std::vector<std::string> get_arg_list(std::initializer_list<std::string>& il){
        return std::vector<std::string> {il};
    }

    // Returns -1 in case if none were found
    size_t get_next_non_space_symbol_pos(std::string& str, size_t from){
        for (size_t cpos = from; cpos < str.size(); cpos++){
            if (str[cpos] != ' ') return cpos;
        }
        return -1;
    }

    // Returns a pair of:
    // 1. string to word in "str" from "from"
    // 2. size_t position where it ends in "str"
    std::pair<std::string, size_t> get_next_word(std::string& str, size_t from){
        size_t pos = str.find(' ', from);
        if (pos == str.npos) pos = str.size() - 1;
        return
            std::make_pair(std::string(str.c_str() + from, pos), pos);
    }

    std::vector<std::string> parse_command_and_arguments(std::string& buffer){
        // Parse command itself
        std::vector<std::string> command_and_args;
        size_t current_pos = 1; // Start from next char after '/'
        if ((current_pos = get_next_non_space_symbol_pos(buffer, current_pos)) != -1){
            auto word_pair = get_next_word(buffer, current_pos);
            command_and_args.push_back(word_pair.first);
            current_pos = word_pair.second;
        }
        else {
            throw std::runtime_error("Invalid command");
        }
        return command_and_args;
    }

    // Returns a tuple of 2 elements: <is_a_command (bool), command_and_args_or_just_message (vector<string>)>
    // If is_a_command is true and if resulting vector is of size() zero that means command is not of valid format
    std::pair<bool, std::vector<std::string>>
                                                    receive_and_parse_input(std::string& buffer){
        // Process command and arguments
        if (buffer[0] == '/'){
            try{
                return std::make_pair(true, parse_command_and_arguments(buffer));
            }
            catch (std::runtime_error& err){
                return std::make_pair(true, std::vector<std::string> {});
            }
        }

        // Process message
        else {
            return std::make_pair(false, std::vector<std::string> {buffer});
        }
    }

    // Use server socket value for client side
    // and nullptr value for server
    bool start_reading_from_user(ip::tcp::socket* target_socket,
                                 std::map<const std::string,
                                 user_action_fptr>* user_action_map,
                                 io_context* ios
            ){
        bool runs = true;
        while (true){
            std::string new_input;
            std::getline(std::cin, new_input);

            // This check is a special case
            if (new_input == "/exit") {
                ios->stop();
                break;
            }
            auto parsed = scs::receive_and_parse_input(new_input);

            // Handle action
            if (parsed.first){
                // This is quite inefficient but efficiency isn't really important in this part
                try{
                    if (parsed.second.size() == 0) {
                        throw std::runtime_error("Not a valid command input");
                    }
                    auto aptr = user_action_map->at(parsed.second[0]);
                    std::vector<std::string> args;
                    std::copy(parsed.second.begin() + 1, parsed.second.end(), args.begin());
                    aptr(target_socket, args);
                }
                catch (std::out_of_range& e){
                    std::cout << "Command " << parsed.second[0] << " not found" << std::endl;
                }
                catch (std::exception& e){
                    std::cout << e.what() << std::endl;
                }
            }

            // Send chat message to socket (Only for client)
            else {
                if (socket != nullptr){
                    send_chat_message_from_cstr(*target_socket, new_input.c_str(), new_input.size());
                }
            }

        }
        return true;
    }
}

#endif //SIMPLE_CHAT_SERVER_USER_ACTIONS_H
