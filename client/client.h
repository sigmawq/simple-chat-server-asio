//
// Created by swql on 12/5/20.
//

#ifndef UNTITLED_CLIENT_H
#define UNTITLED_CLIENT_H

#include <string>
#include <cstdint>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "../common/scs-utility.h"
#include "../common/message.h"
#include "../common/user_actions.h"
#include "../common/bouncing_actions.h"
#include "../logger/logger.h"

using namespace boost::asio;

namespace scs{
    std::string username;
    io_context ios;
    ip::tcp::socket socket {ios};
    ip::tcp::endpoint endpoint;
    logger log { "./logs-client" };

    // Means server sent a chat message, output it
    void bouncing_action_on_chat_message(ip::tcp::socket* socket, message_base* mb){
        std::string username = reinterpret_cast<message_chat*>(mb)->username;
        std::string message = reinterpret_cast<message_chat*>(mb)->message_body;
        output_message(username, message, log);
    }

    void general_action_handle_error(HEADER_SUBCODE error_code){
        std::string desc;
        switch(error_code){
            case ERROR_WRITE_NOT_ALLOWED: desc = "Write not allowed"; break;
            case ERROR_USERNAME_FAILED: desc = "Username change failed"; break;
            default: desc = "Description not provided";
        }
        log.push("Server error (", error_code, "): ", desc);
    }

    void general_action_send_username(std::string& username){
        send_username(socket, username, log);
    }

    void bouncing_action_on_info(ip::tcp::socket* socket, message_base* mb){
        auto casted = reinterpret_cast<message_info*>(mb);
        if (is_error(casted->sub_code)){
            general_action_handle_error(casted->sub_code);
            return;
        }
        else{
            switch (casted->sub_code){
                //TODO
            }
        }
    }

    void user_action_change_username(
            ip::tcp::socket* socket,
            std::vector<std::string>& args){
        arg_assert(args, 1);
        if (args[0].size() > USERNAME_SIZE) throw std::runtime_error("Username provided is not valid");
        general_action_send_username(args[0]);
    }

    void internal_action_change_username(ip::tcp::socket* socket, std::string& username){
        send_username(*socket, username, log);
    }

    std::map<const std::string, user_action_fptr> user_action_map{
            {"username", &user_action_change_username}
    };

    std::map<const scs::HEADER_CODE, bouncing_action_fptr> bouncing_action_map{
            {CHAT_MESSAGE, bouncing_action_on_chat_message},
            {INFO, bouncing_action_on_info}
    };

    void handle_incoming_bouncing_action(ip::tcp::socket* socket, message_base* mb){
        try{
            bouncing_action_map.at(mb->code)(socket, mb);
        }
        catch (std::out_of_range& e){
            log.push("An internal action code: ", mb->code, " failed to be processed");
        }
    }

    void handle_socket_eof(ip::tcp::socket* socket){
        log.push("Server closed connection");
        ios.stop();
    }

    void disconnect(){
        socket.close();
    }

    void connect(std::string& address, std::string& port){
        int16_t port_int = get_port(port.c_str());
        endpoint = ip::tcp::endpoint(
                boost::asio::ip::address::from_string(address),
                port_int);
        socket.connect(endpoint);
        log.push("Connected to ", endpoint.address().to_string(), ':', port);
    }

    void stop(){
        ios.stop();
    }

    void launch(std::string& address, std::string& port, std::string& username){
        scs::username = username;
        log.push(
                "username/address/port: ", username, ' ', address, ' ', port);
        log.push("Starting scs-client..");
        try {
            connect(address, port);
            general_action_send_username(username);
        }
        catch(std::exception const& e){
            log.push("Connection failed: ", e.what());
        }
        catch(...){
            log.push("Connection failed: ", "unexpected exception");
        }

        auto end = std::async(start_reading_from_user, &socket, &user_action_map, &ios, &log, false);

        message_base mb2;
        scs::start_listen_to_socket_continuously(
                &socket, &mb2, handle_incoming_bouncing_action, handle_socket_eof, 0, READBUFF_SIZE, log);

        ios.run();
        // ========================
        // End is here
        // ========================
        socket.close();
    }
}

#endif //UNTITLED_CLIENT_H
