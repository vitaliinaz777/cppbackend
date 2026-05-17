#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

int main(int argc, const char** argv) {
    static const int port = 3333;
    static const size_t max_buffer_size = 1024;

    if (argc != 2) {
        std::cout << "Usage: "sv << argv[0] << " <server IP>"sv << std::endl;
        return 1;
    }

    try {
        // Создаём io_context, который будет использоваться для всех операций ввода-вывода.
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет. 
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint. 
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        
        // Создаём endpoint для удалённого сервера, используя IP-адрес из аргумента командной строки и порт 3333.
        auto endpoint = udp::endpoint(net::ip::make_address(argv[1], ec), port);
        
        // Отправляем данные на сервер.
        socket.send_to(net::buffer("Hello from UDP-client"sv), endpoint); 

        // Получаем данные и endpoint.
        std::array<char, max_buffer_size> recv_buf; // Буфер для приёма данных
        udp::endpoint sender_endpoint; // Endpoint для хранения адреса отправителя

        // Получаем данные и endpoint отправителя. Размер полученных данных будет возвращён функцией receive_from.
        size_t size = socket.receive_from(net::buffer(recv_buf), sender_endpoint); 

        std::cout << "Server responded "sv << std::string_view(recv_buf.data(), size) << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}