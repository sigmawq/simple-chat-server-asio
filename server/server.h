//
// Created by swql on 12/5/20.
//

#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <vector>
#include "../common/scs-utility.h"
#include "../common/message.h"
#include "../common/user_actions.h"

using namespace boost::asio;

namespace scs{
    constexpr int BACKLOG_SIZE = 30;

    class connection_record{
    public:
        std::string username;
        ip::tcp::endpoint endpoint;
        ip::tcp::socket socket;
        std::array<char, scs::READBUFF_SIZE> buffer;

        connection_record(ip::tcp::endpoint& endpoint, ip::tcp::socket& socket) :
                endpoint{endpoint}, socket{std::move(socket)}, buffer{0}
        {
        }

        inline void clear_buffer(){
            std::fill_n(buffer.data(), buffer.size(), 0);
        }

    };

    io_context ios;
    ip::tcp::acceptor acceptor {ios};
    std::vector<connection_record> active_connections;

    // Temporary data, for new connections
    ip::tcp::endpoint temp_ep;
    ip::tcp::socket temp_socket {ios};

    void async_write(std::u16string& message, ip::tcp::socket& socket, int from = 0){
        const boost::asio::const_buffer buff {message.c_str() + from, message.size() * sizeof (int16_t)};
        int message_length = message.size() * sizeof (int16_t);
        auto callback = [&, message_length](const boost::system::error_code& error,
                                            std::size_t bytes_sent) {
            if (!error){
                std::cout << "Sent bytes: " << bytes_sent << " of " << message_length << std::endl;
                if (bytes_sent < message_length){
                    async_write(message, socket, bytes_sent / sizeof (int16_t));
                }
            }
            else std::cout << "Error while async write!" << std::endl;
        };
        socket.async_send(buff, callback);
    }

    void setup(std::string port){
        int16_t port_int = get_port(port.c_str());
        ip::tcp::endpoint ep(ip::tcp::v4(), port_int);
        acceptor.open(ep.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(ep);
        acceptor.listen(BACKLOG_SIZE);
    }

    void user_action_get_server_info(ip::tcp::socket* socket, std::vector<std::string>& args){
        std::cout << "TODO: Server info" << std::endl;
    }

    bool general_action_validate_username(std::string& username){
        for (auto& connection : active_connections){
            if (connection.username == username){
                return false;
            }
        }
        return true;
    }

    void bouncing_action_on_chat_message(ip::tcp::socket* socket, message_base* mb){
        std::cout << "Socket sent a message " << reinterpret_cast<message_chat*>(mb)->message_body
        << std::endl;
    }

    void bouncing_action_on_change_username(ip::tcp::socket* socket, message_base* mb){
        auto& cr =
        *std::find_if(active_connections.begin(), active_connections.end(), [&socket](connection_record& cr){
            if (&(cr.socket) == socket){
                return true;
            }
            return false;
        });
        std::string new_username(reinterpret_cast<message_user*>(mb)->username);
        std::cout << "User " << cr.endpoint.address().to_string()
        << " tries to change it's username from " << cr.username
        << " to " << new_username << std::endl;
        if (general_action_validate_username(new_username)){
            cr.username = new_username;
            std::cout << ".. and it is free" << std::endl;
        }
        else {
            std::cout << ".. but it is already in use" << std::endl;
        }
    };

    std::map<scs::HEADER_CODE, bouncing_action_fptr> bouncing_action_map{
        {CHAT_MESSAGE, &bouncing_action_on_chat_message},
        {USERNAME, &bouncing_action_on_change_username}
    };

    std::map<const std::string, user_action_fptr> user_action_map{
            {"sinfo", &user_action_get_server_info}
    };

    void handle_bouncing_actions(ip::tcp::socket* socket, scs::message_base* mb){
        std::cout << "Socket sent the following code" << mb->code << std::endl;
        try{
            bouncing_action_map.at(mb->code)(socket, mb);
        }
        catch (std::out_of_range& e){
            std::cout << "Unhandled bouncing action (" << mb->code << ')' << std::endl;
        }
    }

    void start_async_accept();
    void handle_connection(const boost::system::error_code& error){
        if (!error){
            // This part is bad, I know
            std::cout << "New connection from: " << temp_ep.address().to_string() << std::endl;
            active_connections.emplace_back(temp_ep, temp_socket);
            start_listen_to_socket_continuously(
                    active_connections.back().socket,
                    (message_base*)active_connections.back().buffer.data(),
                    &handle_bouncing_actions, 0, READBUFF_SIZE);
        }
        else{
            std::cout << "Error: " << error.message() << std::endl;
        }

        start_async_accept();
    }

    void start_async_accept(){
        std::cout << "Listening for connections..." << std::endl;
        // Bind member function (simply passing it to async_accept won't work!)
        acceptor.async_accept(temp_socket, temp_ep, &handle_connection);
    }

    void launch(std::string& port){
        setup(port);
        start_async_accept();
        auto end = std::async(start_reading_from_user, nullptr, &user_action_map);

        ios.run();
        // End is here
        end.get();
        acceptor.close();
    }
};


#endif //UNTITLED_SERVER_H
