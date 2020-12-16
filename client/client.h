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

using namespace boost::asio;

namespace scs{
    std::string username;
    io_context ios;
    ip::tcp::socket socket {ios};
    ip::tcp::endpoint endpoint;

    // Means server sent a chat message, output it
    void bouncing_action_on_chat_message(ip::tcp::socket* socket, message_base* mb){
        auto casted = reinterpret_cast<message_chat*>(mb);
        std::cout << '[' << casted->username << "]: " << casted->message_body << std::endl;
    }

    void general_action_send_username(ip::tcp::socket* socket, std::string& username){
        message_user message_usr;
        message_usr.set_message(username.c_str(), username.size());
        send_message_raw(*socket, (message_base*)&message_usr, 0, READBUFF_SIZE);
    }

    void bouncing_action_on_info(ip::tcp::socket* socket, message_base* mb){
        switch (reinterpret_cast<message_info*>(mb)->sub_code){
            // TODO: Implement different errors as subcodes
        }
    }

    void user_action_change_username(
            ip::tcp::socket* socket,
            std::vector<std::string>& args){
        if (args[0].size() > USERNAME_SIZE) throw std::runtime_error("Username provided is not valid");
        general_action_send_username(socket, args[0]);
    }

    void internal_action_change_username(ip::tcp::socket* socket, std::string username){
        message_user new_username;
        new_username.set_message(username.c_str(), username.size());
        send_message_raw(*socket, (message_base*)&new_username, 0, READBUFF_SIZE);
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
            std::cout << "An internal action code: " << mb->code << " failed to have been processed" << std::endl;
        }
    }

    void handle_socket_eof(ip::tcp::socket* socket){
        std::cout << "Server closed connection" << std::endl;
        ios.stop();
    }

    void disconnect(){
        socket.close();
    }

    bool connect(std::string& address, std::string& port){
        try{
            int16_t port_int = get_port(port.c_str());
            endpoint = ip::tcp::endpoint(
                    boost::asio::ip::address::from_string(address),
                    port_int);
            socket.connect(endpoint);
            std::cout << "Connected to " << endpoint.address().to_string() << std::endl;
            return true;
        }
        catch (std::exception& e){
            std::cout << e.what() << std::endl;
            return false;
        }
        catch (boost::system::system_error& e){
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    void stop(){
        ios.stop();
    }

    void launch(std::string& address, std::string& port, std::string& username){
        if (!connect(address, port)){
            std::cout << "Connection failed";
            return;
        }

        // This should be a separate function, but the whole message system needs some rework
        message_base mb;
        auto casted = base_to_derived<message_user>(mb);
        casted->set_message(username.c_str(), username.size());
        send_username(socket, *casted);

        auto end = std::async(start_reading_from_user, &socket, &user_action_map, &ios);
        message_base mb2;
        mb.erase();
        scs::start_listen_to_socket_continuously(
                &socket, &mb2, handle_incoming_bouncing_action, handle_socket_eof, 0, READBUFF_SIZE);

        ios.run();
        // ========================
        // End is here
        // ========================
        end.get();
        socket.close();
    }
}

#endif //UNTITLED_CLIENT_H
