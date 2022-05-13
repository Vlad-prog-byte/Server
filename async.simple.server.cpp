
#include <cstdlib>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;


//смысл connection это ответить или принять запрос клиента
class Connection
{
public:
    Connection(boost::asio::io_context& io_context);
    void start();

    //возвращает нам сокет соединения
    tcp::socket& socket(); 

    private:
    //колбек на чтение
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    //колбэк на запись
    void handle_write(const boost::system::error_code& error);

private:
    tcp::socket socket_;
    enum {max_length = 1024};
    char data_[max_length];

};

class Connection
{
public:
    Connection(boost::asio::io_context& io_context) : socket_(io_context) {}//подключение сокета к io_context

    tcp::socket& socket() { return socket_; }

    //асинхронное чтение данных считываем запрос от клиента запускаем колбек
    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&Connection::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            //НЕ Е** КАК 
            boost::asio::async_write(socket_,
                boost::asio::buffer(data_, bytes_transferred), boost::bind(&Connection::handle_write, this, boost::asio::placeholders::error));
        }
        //ЕСЛИ ВСЕ ЖЕ ОШИБКА ТО ДАННОЕ СОЕДИНЕНИЕ НЕ НУЖНО => удаляем
        else
        {
            delete this;
        }
    }

    //ТАК И НЕ ПОНЯЛ ЗАЧЕМ ОНО НУЖНО
    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&Connection::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};


//Класс сервер нужен чтобы принимать и закрывать соединения с клиентом который потом вызывает класс connection для связи с клиентом
// class Server
// {
// public:
//     //инициализация порта хоста
//     Server(short port);
//     //хапуск сервера
//     void run();

// private:
//     //принимать входящие сообщения сообщения
//     void start_accept();

//     //колбэк для уст соединений
//     void handle_accept(Connection *new_collection, const boost::system::error_code& error);


// private:
//     boost::asio::io_context io_context_;
//     tcp:: acceptor acceptor_;
// };

//Конструктор сервера 
class Server
{
public:
    Server(short port)
            : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) // не *** зачем это нужно норм объяснения пока не нашел
    {
        //запуск приема запросов
        start_accept();
    }

    void run()
    {
        io_context_.run();
    }

private:
    void start_accept()
    {
        //ПРОСТО СОЗДАЕМ СОЕДИНЕНИЕ СЕРВЕРА С КЛИЕНТОМ ПРИ ВЫЗОВЕ ДАННОЕ СТРОЧКЕ ПРОСТО ПРОИСХОДИТ ПОДСОЕДИНЕНИЕ СОКЕТА К IO_CONTEXT
        Connection* new_connection = new Connection(io_context_);

        //асинхронный прием на запрос
        acceptor_.async_accept(new_connection->socket(),
                    boost::bind(&Server::handle_accept, this, new_connection, boost::asio::placeholders::error));
    }

    //КОЛБЭК
    void handle_accept(Connection* new_connection, const boost::system::error_code& error)
    {
        if (!error)
        {
            //ФИГНЯ ИЗ CONNECTION бред
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
};




int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        Server s(std::atoi(argv[1]));

        s.run();//ЧТО ЭТО ТАКОЕ?????????7
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
