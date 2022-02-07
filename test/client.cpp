// TODO change config.hpp and chat_messages.hpp to correct path

#include "config.hpp"
#include "chat_messages.hpp"
#include <iostream>
#include <memory>
#include <utility>
#include <string.h>
#include <stdlib.h> 
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>

using boost::asio::ip::tcp;


#ifndef INPUT_MESSAGE
#define INPUT_MESSAGE "Enter Message in format <dst_id> <message> or type exit in order to close the connection \n"
#endif

#ifndef ERROR_INPUT_NOT_VALID
#define ERROR_INPUT_NOT_VALID "Input is not valid\n"
#endif

#ifndef EXIT
#define EXIT "exit"
#endif

#ifndef CONNECTION_IS_CLOSED
#define CONNECTION_IS_CLOSED "Connection is closed"
#endif

#ifndef ERROR_COULD_NOT_CONNECT
#define ERROR_COULD_NOT_CONNECT "Could not connect to the server"
#endif


bool is_number(std::string str);


class session
    : public std::enable_shared_from_this<session>
{
    public:
        session(tcp::socket sock)
            : sock_(std::move(sock))
        {
        }

        void start()
        {   
            if (!login_success())
            {
                exit (EXIT_FAILURE);
            }

            do_read();
            user_interact();
        }

    private:
        bool login_success()
        {
            char data[DATA_MAX_LENGTH];
            boost::system::error_code ec;

            sock_.read_some(boost::asio::buffer(data, DATA_MAX_LENGTH), ec);

            if (ec == boost::asio::error::eof)
            {
                std::cout << ERROR_COULD_NOT_CONNECT << std::endl;
                close_socket();

                return false;
            }

            else if (!strcmp(data, ERROR_CONNECT_LOBBY_FULL_MESSAGE))
            {
                std::cout << ERROR_CONNECT_LOBBY_FULL_MESSAGE;
                close_socket();

                return false;       
            }
            
            std::cout << RECEIVED_MESSAGE << data << std::endl;

            return true;
        }
            
        void close_socket()
        {
            sock_.shutdown(tcp::socket::shutdown_both);
            sock_.close();
        }

        void do_read()
        {
            auto self(shared_from_this());

            sock_.async_read_some(boost::asio::buffer(data_, DATA_MAX_LENGTH),
                [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (ec == boost::asio::error::eof)
                {
                    std::cout << CONNECTION_IS_CLOSED << std::endl;
                }

                else if (!ec)
                {
                    std::cout << RECEIVED_MESSAGE << data_ << std::endl;
                    do_read(); 
                }
            });
        }

        void user_interact()
        {
            auto self(shared_from_this());

            char data[DATA_MAX_LENGTH];
            
            std::cout << INPUT_MESSAGE << std::endl;
            std::cin.getline(data, DATA_MAX_LENGTH);

            if (!strcmp(data, EXIT))
            {
                close_socket();
                exit (EXIT_SUCCESS);
            }

            else if(valid_data(data))
            {
                send_message(data, strlen(data));
            }

            else
            {
                std::cerr << ERROR_INPUT_NOT_VALID << std::endl;
                user_interact();
            }
        }

        bool valid_data(const char * data)
        {
            if (!isdigit(*data))
            {
                return false;
            }

            std::vector<std::string> results;

            boost::algorithm::split(results, data, [](char c){return c == ' ';});

            return is_number(results[0]);
        }

        void send_message(char * data, std::size_t length)
        {
            auto self(shared_from_this());
            // char data[DATA_MAX_LENGTH];
            
            boost::asio::async_write(sock_, boost::asio::buffer(data, length),
                [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (ec == boost::asio::error::eof)
                {
                    std::cout << "Connection closed\n";
                }

                else if (ec)
                {
                    throw boost::system::system_error(ec);
                }
                else
                {
                    user_interact();
                }
            });
        }

        tcp::socket sock_;
        char data_[DATA_MAX_LENGTH];
};

bool is_number(std::string str)
{
    for(std::string::iterator it = str.begin(); it != str.end(); it++)
    {
        if (!isdigit(*it))
        {
            return false;
        }
    }

    return true;
}

void client(boost::asio::io_service& io_service, std::string ip, unsigned short port)
{
    using namespace boost::asio;
    using ip::tcp;

    tcp::socket sock(io_service);
    sock.connect(ip::tcp::endpoint(ip::address::from_string(ip), port));

    std::shared_ptr<session> s;

    s = std::make_shared<session>(std::move(sock));

    s->start();

    io_service.run();

}



int main(int argc, char const *argv[])
{
    boost::asio::io_service io_service;

    client(io_service, IP, PORT);

    return 0;
}