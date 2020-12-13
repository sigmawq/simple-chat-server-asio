//
// Created by swql on 12/5/20.
//

#ifndef UNTITLED_SCS_UTILITY_H
#define UNTITLED_SCS_UTILITY_H

#include <cstdint>
#include <string>
#include <ctime>
#include <stdexcept>
#include "message.h"
#include "bouncing_actions.h"

namespace scs{
    [[nodiscard]] int16_t get_port(const char* port_str){
        try{
            unsigned long long port_raw = std::stoll(port_str);
            if (port_raw > UINT16_MAX) throw std::runtime_error("Port above 65635 is not allowed");
            if (port_raw < 1024) throw std::runtime_error("Port below 1024 is not allowed");
            return static_cast<int16_t>(port_raw);
        }
        catch (std::invalid_argument& e) {
            throw std::runtime_error("Invalid port provided");
        }
    }

    [[nodiscard]] unsigned long long get_current_epoch_time(){
        return time(NULL);
    }

    void copy_until(const char* source, char* destination, int max_chars, char delim = 0){
        for (int i = 0; i < max_chars; i++){
            if (source[i] == delim || i > max_chars - 1){
                return;
            }
            destination[i] = source[i];
        }
    }
}

#endif //UNTITLED_SCS_UTILITY_H
