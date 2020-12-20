//
// Created by swql on 12/5/20.
// Username starting from '$' is reserved by server
//

#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <list>
#include "../logger/logger.h"
#include "../common/scs-utility.h"
#include "../common/message.h"
#include "../common/user_actions.h"
#include "../common/defs.h"


using namespace boost::asio;

namespace scs{
    constexpr int BACKLOG_SIZE = 30;

    io_context ios;
    ip::tcp::acceptor acceptor {ios};
    logger log { "./logs-server" };
    const std::string server_username_signature = "$";

    // A list of active connections. Last connection record is always reserved for new connection
    std::shared_ptr<std::list<connection_record>> active_connections;

    void setup(std::string& port){
        int16_t port_int = get_port(port.c_str());
        ip::tcp::endpoint ep(ip::tcp::v4(), port_int);
        acceptor.open(ep.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(ep);
        acceptor.listen(BACKLOG_SIZE);
    }

    void user_action_get_server_info(ip::tcp::socket* socket, std::vector<std::string>& args){
        std::cout << "Server information:" << std::endl;
        std::cout << " Active connections: " << active_connections->size() - 1 << std::endl;
    }

    void user_action_get_active_connections(ip::tcp::socket* socket, std::vector<std::string>& args){
        std::cout << "Active connections list: " << std::endl;

        std::list<connection_record>::const_iterator it;
        size_t counter = 0;
        for (it = active_connections->begin(); it != --active_connections->end(); it++){
            std::string if_allowed_to_write = it->is_allowed_to_write ? "allowed to write" : "not allowed to write";
            std::cout << counter << ". " << it->username << '(' << it->endpoint.address().to_string()
            << ')' << " - " << if_allowed_to_write << std::endl;
            ++counter;
        }
    }

    // Add empty connection record at the end, used for pending connection
    void general_action_add_blank_cr(){
        active_connections->emplace_back(ios);
    }

    bool general_action_validate_username(std::string& username){
        if (username[0] == '$') { return false; }
        for (auto& connection : *active_connections){
            if (connection.username == username){
                return false;
            }
        }
        return true;
    }

    // If you want to send a message from server, username should start from '$'
    void general_action_send_chat_message_to_all(std::string& message, const std::string& username,
                                                 std::vector<ip::tcp::socket*>& exclude){

        // Iterate until the previous before the last element - last element is reserved for new connection
        for (auto it = active_connections->begin(); it != --active_connections->end(); it++){
            // Checke if current connection record is in exclude list
            bool if_to_send = (std::find_if(exclude.begin(), exclude.end(), [&](ip::tcp::socket* sock){
                if (sock == &it->socket) return true;
                return false;
            })) == exclude.end();
            if (if_to_send) send_chat_message(it->socket, message, username, log);
        }
    }

    void user_action_send_msg_to_all(ip::tcp::socket* socket, std::vector<std::string>& args){
        arg_assert(args, 1);
        std::vector<ip::tcp::socket*> empty_exclude;
        general_action_send_chat_message_to_all(args[0], server_username_signature, empty_exclude);
    }

    void bouncing_action_on_chat_message(ip::tcp::socket* socket, message_base* mb){
        try{
            auto sender = std::find_if(active_connections->begin(), active_connections->end()--, [&](connection_record& cr){
                if (&cr.socket == socket){
                    return true;
                }
                return false;
            });

            if (sender == active_connections->end()) throw std::runtime_error("Message sender not found");
            if (!(sender->is_allowed_to_write)) {
                std::cout << "Socket isn't allowed to write. Message not sent" << std::endl;
                send_info_code(*socket, ERROR_WRITE_NOT_ALLOWED, log);
                return;
            }

            auto converted_from_base = reinterpret_cast<message_chat*>(mb);
            std::string username(sender->username);
            std::string message(converted_from_base->message_body);
            output_message(username, message, log);

            std::vector<ip::tcp::socket*> exclude { &(sender->socket) };
            general_action_send_chat_message_to_all(message, username, exclude);
        }
        catch (std::exception& e){
            std::cout << e.what() << std::endl;
        }
    }

    void bouncing_action_on_change_username(ip::tcp::socket* socket, message_base* mb){
        auto& cr =
        *std::find_if(active_connections->begin(), active_connections->end(), [&socket](connection_record& cr){
            if (&(cr.socket) == socket){
                return true;
            }
            return false;
        });
        std::string new_username(reinterpret_cast<message_user*>(mb)->username);
        std::cout << "User " << socket->remote_endpoint().address().to_string()
        << " tries to change it's username from " << cr.username
        << " to " << new_username << std::endl;
        if (general_action_validate_username(new_username)){
            cr.username = new_username;
            cr.is_allowed_to_write = true;
            std::cout << ".. and it is free" << std::endl;
        }
        else {
            std::cout << ".. but it is already in use" << std::endl;
            send_info_code(*socket, ERROR_USERNAME_FAILED, log);
        }
    };

    std::map<scs::HEADER_CODE, bouncing_action_fptr> bouncing_action_map{
            {CHAT_MESSAGE, &bouncing_action_on_chat_message},
            {USERNAME,     &bouncing_action_on_change_username}
    };

    std::map<const std::string, user_action_fptr> user_action_map{
            {"sinfo", &user_action_get_server_info},
            {"show-active", &user_action_get_active_connections},
            {"msg-all", &user_action_send_msg_to_all}
    };

    void handle_bouncing_actions(ip::tcp::socket* socket, scs::message_base* mb){
        std::cout << "Socket sent the following code " << mb->code << std::endl;
        try{
            bouncing_action_map.at(mb->code)(socket, mb);
        }
        catch (std::out_of_range& e){
            std::cout << "Unhandled bouncing action (" << mb->code << ')' << std::endl;
        }
    }

    void handle_socket_eof(ip::tcp::socket* eof_sock){
        std::list<connection_record>::iterator it;
        it = std::find_if(active_connections->begin(), --active_connections->end(), [eof_sock]
                                                                                    (connection_record& cr){
            if (&(cr.socket) == eof_sock){
                return true;
            }
            return false;
        });
        std::cout << eof_sock->remote_endpoint().address().to_string() << "/"
                  << it->username
                  << " closed connection" << std::endl;
        active_connections->erase(it);
        // TODO: Make it better
    }

    void start_async_accept();

    void handle_connection(const boost::system::error_code& error){
        if (!error){
            // This part is bad, I know
            std::cout << "New connection from: " <<
            active_connections->back().socket.remote_endpoint().address().to_string() << std::endl;

            start_listen_to_socket_continuously(
                    &(active_connections->back().socket),
                    (message_base*)(active_connections->back().buffer.data()),
                    &handle_bouncing_actions, &handle_socket_eof,
                    0, READBUFF_SIZE, log);
        }
        else{
            std::cout << "Error: " << error.message() << std::endl;
        }

        general_action_add_blank_cr();
        start_async_accept();
    }

    void start_async_accept(){
        std::cout << "Listening for connections..." << std::endl;
        // Bind member function (simply passing it to async_accept won't work!)
        acceptor.async_accept(active_connections->back().socket, active_connections->back().endpoint,
                              &handle_connection);
    }

    void launch(std::string& port){
        setup(port);
        active_connections = std::make_shared<std::list<connection_record>>();
        general_action_add_blank_cr();
            start_async_accept();
        auto end = std::async(start_reading_from_user, nullptr, &user_action_map, &ios, &log, true);
        ios.run();
        // End is here
        end.get();
        acceptor.close();
    }
};

#endif //UNTITLED_SERVER_H
