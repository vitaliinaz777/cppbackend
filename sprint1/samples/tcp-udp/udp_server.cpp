#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
// TCP больше не нужен, импортируем имя UDP
using net::ip::udp;

using namespace std::literals;

int main() {
    static const int port = 3333;
    static const size_t max_buffer_size = 1024; // Максимальный размер буфера для приёма данных

    try {
        // Создаём io_context, который будет использоваться для всех операций ввода-вывода.
        boost::asio::io_context io_context;

        // Создаём UDP-сокет и привязываем его к порту 3333 на всех интерфейсах.
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            // UDP-датаграммы могут быть размером до 65507 байт, но мы ограничим размер буфера для простоты.
            std::array<char, max_buffer_size> recv_buf;

            // Создаём endpoint для хранения адреса клиента, от которого придёт датаграмма.
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента. 
            // Размер полученных данных будет возвращён функцией receive_from.
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            // Выводим полученные данные на консоль. Используем std::string_view для удобства. 
            // Размер данных может быть меньше размера буфера, поэтому используем возвращённый размер.  
            std::cout << "Client said "sv << std::string_view(recv_buf.data(), size) << std::endl;

            // Отправляем ответ на полученный endpoint, игнорируя ошибку.
            // На этот раз не отправляем перевод строки: размер датаграммы будет получен автоматически.
            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("Hello from UDP-server"sv), remote_endpoint, 0, ignored_error);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}