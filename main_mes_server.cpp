#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
// #include "UserDBManager.h"
// #include "MessageDBManager.h"

#include<map>


using boost::asio::ip::tcp;


//таблица игра со словами и сокетами
struct table
{
    std::vector<tcp::socket&> socket_;
    std::vector<std::string> words;
};

std::map<std::string, table> base;
std::map<std::string, std::vector<tcp::socket&>> queue_;

static unsigned int game_id;


//куда таймер с игрой
class Connection
{
public:
    Connection(boost::asio::io_context& io_context, DBConnection& conn_)
            : socket_(io_context), DB_conn(conn_){}

    tcp::socket& socket(){return socket_;}

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(recieved_, max_length),
                                boost::bind(&Connection::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

private:

    bool FindWord(std::string msg, std::string word)
    {
        return msg.find(word) != std::string::npos ? true : false;
    }

    std::string readUntil(char until, const char* arr, int &counter)
    {
        std::string result = "";
        while (arr[counter] != until) {
            result += recieved_[counter++];
        }
        return result;
    }

    void static CopyFromStringToCharArray(const std::string& str, size_t str_size, char* arr)
    {
        for (size_t counter = 0; counter < str_size; ++counter)
        {
            arr[counter] = str[counter];
        }
    }

    std::string PrepareStringTasksResponse(std::vector<Task> arr, UserDBManager& UDBM)
    {
        std::string result = "";
        size_t size = arr.size();
        if (arr.size() > 0)
        {
            for (size_t counter = 0; counter < size - 1; ++counter)
            {
                result += std::to_string(arr[counter].id) + ":" + arr[counter].head + ":" + UDBM.get_user(arr[counter].assigner_id).login
                          + ":" + UDBM.get_user(arr[counter].executor_id).login + ":";
            }
            result += std::to_string(arr[size - 1].id) + ":" + arr[size - 1].head + ":" + UDBM.get_user(arr[size - 1].assigner_id).login
                      + ":" + UDBM.get_user(arr[size - 1].executor_id).login;
        }

        return result;
    }

    void EmptyBuffer(char* arr)
    {
        for (size_t counter = 0; counter < max_length; ++counter)
        {
            arr[counter] = '\r';
        }
    }

    void Process()
    {
       // MessageDBManager MDBM(DB_conn.session);
       // UserDBManager UDBM(DB_conn.session);
        int i = 0;
        std::string request, new_text, task_id, from_id;
        request = readUntil(':', recieved_, i);

        ++i;

        if (request == "settings")
        {
            std::string type_game = readUntil(':', recieved_, i);
            i++;
            std::string level_game = readUntil(':', recieved_, i);
            i++;
            
            tcp::socket& sock = socket();
            std::string id = type_game + level_game;
            if (queue_[id].size() == 5)
            {
                game_id++;
                std::string str = std::to_string(game_id);
                base[str].socket_ = queue_[id];
                //base[str].words = get_words; ТУТ НУЖЕН ВАНЯ ЧТОБЫ Я ПОЛУЧИЛ СЛОВА
                queue_[id].clear();
            
                //разослать всем клиентам инфу о начале игры
            }
            else
                queue_[id].push_back(socket_);
        }
        else if (request == "msg")
        {
            //как узнать отгадывающий или объясняющий????????
            std::string flag = readUntil(':', recieved_, i);
            i++;
            std::string id = readUntil(':', recieved_, i);
            i++;
            //как мне узнать кто есть кто среди всех сокетов
            std::string msg = readUntil(':', recieved_, i);
            i++;
            std::string word = base[id].words.back();
            base[id].words.pop_back();
            
            bool flag_find = FindWord(msg, word);
            //значит отгадывающий
            if (flag == "0")
            {
                if (flag_find == 1)
                {
                    //отправить сообщение с баллами
                }
                else
                {
                    //отправить сообщение всем игрокам
                }
            }
        }
        else if (request == "autorize") 
        {
            std::string login = readUntil(':', recieved_, i);
            i++;
            //ТУТ НУЖЕН ВАНЯ ЧТОБЫ Я АВТОРИЗИРОВАЛ ПОЛЬЗОВАТЕЛСЯ
            //ПОТОМ ОТПРАВИТЬ
        }
        EmptyBuffer(recieved_);//что это за хрень?
    }

    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred)
    {
        Process();
        if (send_required)
        {
            if (!error)
            {
                boost::asio::async_write(socket_,
                                         boost::asio::buffer(sent_, to_send.size()),
                                         boost::bind(&Connection::handle_write, this,
                                                     boost::asio::placeholders::error));
            }
            else
            {
                delete this;
            }
        }
        else
        {
            handle_write(error);
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_read_some(boost::asio::buffer(recieved_, max_length),
                                    boost::bind(&Connection::handle_read, this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 10000 };
    char recieved_[max_length];
    char sent_[max_length];
    bool send_required;
    std::string to_send;
    DBConnection& DB_conn;
};



class Server
{
public:
    Server(short port, DBConnection& conn_)
            : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), DB_conn(conn_)
    {
        start_accept();
    }

    void run()
    {
        io_context_.run();
    }

private:
    void start_accept()
    {
        Connection* new_connection = new Connection(io_context_, DB_conn);
        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&Server::handle_accept,
                                           this,
                                           new_connection,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(Connection* new_connection,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        else
        {
            delete new_connection;
        }

        start_accept();
    }

private:
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    DBConnection& DB_conn;
};



int main(int argc, char* argv[])
{
    DBConnection DB_conn;
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        Server s(std::atoi(argv[1]), DB_conn);

        s.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}