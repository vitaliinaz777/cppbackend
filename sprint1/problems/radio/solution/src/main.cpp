/*
Задание 1 — Рация на UDP
Возьмите заготовку кода из папки sprint1/problems/radio своего репозитория.
В ней вы найдёте готовую программу, записывающую звук с микрофона и сразу же воспроизводящую его.
Для работы с аудио используется библиотека miniaudio, доступная в репозитории Conan.
Ваша задача — переделать заготовку и сделать так, чтобы звук записывался на одном устройстве, затем передавался по UDP и воспроизводился на сервере.
В заготовке пользователь просто нажимает ENTER. В вашем решении он должен ввести IP-адрес сервера при передаче каждого аудиосообщения.
Клиент и сервер будут одной программой. При запуске пользователь указывает следующие параметры командной строки:
слово client либо server,
рабочий порт, по которому будет устанавливаться соединение.
💡 Процесс оцифровки звука называется дискретизацией. При этой процедуре аналоговый звуковой сигнал измеряется много раз в секунду. Каждое измерение кодируется числом, которое называется «семпл». Если несколько измерений соответствуют одному моменту времени (например, два измерения при записи в формате стерео), то этот набор семплов называется фреймом. Наиболее популярная частота дискретизации — 48000 фреймов в секунду. Согласно теореме Котельникова-Шеннона, она позволяет сохранять сигналы частотой до 24000 Гц. Также часто используется частота дискретизации 44100 фреймов в секунду. Такая частота будет использоваться и в вашей рации.
За один раз рация читает 65000 8-битных фреймов в формате моно. Это чуть меньше, чем полторы секунды звука. Получается вектор из 65000 байт. Его можно передать одной датаграммой. Фактически от рекордера может быть получено меньшее количество байт, поэтому следует учитывать длину сообщения.
Для чтения и воспроизведения звука в файле audio.h есть классы Recorder и Player. Вам будут полезны следующие методы:
Метод Record класса Recorder. Получает звук с микрофона и возвращает в виде vector<char>. Также он возвращает количество прочитанных фреймов. Метод принимает максимальное количество фреймов для записи и время записи.
Метод PlayBuffer класса Player. Воспроизводит звук из vector<char>. В качестве параметров передаётся вектор семплов, количество фреймов и время воспроизведения.
Требования
Включайте файл audio.h первым. Из-за плохой совместимости на некоторых ОС возможны проблемы при другом порядке включения.
Не меняйте файл audio.h.
В конструкторы классов Recorder и Player передавайте параметры ma_format_u8, 1, что означает 8-битные моно-семплы.
Сервер работает только на приём датаграмм и не отвечает клиенту. Клиент только отправляет данные.
В данных условиях размер одного фрейма — один байт, поэтому длина полученного сообщения равна количеству фреймов. Не полагайтесь на это, программа должна работать и при фреймах другого размера.
Как будет тестироваться ваша программа
Тестирование мы доверяем вам. Запустите клиент и сервер на разных компьютерах и передайте несколько аудиосообщений.
Подсказки

Реализуйте решение в функциях void StartServer(uint16_t port) и void StartClient(uint16_t port).
Вычислите количество байт, которые нужно передать: умножьте количество фреймов, полученное от класса Recorder, на размер одного фрейма, полученный методом GetFrameSize() этого же класса. Будьте готовы к тому, что размер фрейма может быть не равен одному байту.
Вычислите количество фреймов в сообщении: разделите его длину на значение, полученное методом GetFrameSize() класса Player.
Вы можете конструировать boost::asio::buffer из вектора для чтения и передачи семплов.*/

#include "audio.h"
#include <boost/asio.hpp>

#include <iostream>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

// Максимальное количество фрэймов
static const int max_n_frames = 65000;
// Максимальный размер буфера в байтах
static const size_t max_buffer_size = 65000;

void PrintBadArgsMessage(const char *taskName){
    std::cout << "Usage: "sv << taskName << " server|client"sv
              << " <connection port>"sv << std::endl;
}

void StartServer(uint16_t port) {
    try {
        // Создаём io_context, который необходим для работы с сокетами
        boost::asio::io_context io_context;

        // Открываем сокет для приёма данных. Указываем протокол (IPv4) и порт, на котором будет работать сервер
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Создаём объект Player для воспроизведения звука. Указываем формат семплов и количество каналов (моно).
        Player player(ma_format_u8, 1);

        // Создаём буфер достаточного размера, чтобы вместить датаграмму.
        std::array<char, max_buffer_size> recv_buf;

        // Endpoint для хранения адреса клиента, от которого пришло сообщение
        udp::endpoint remote_endpoint;

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Получаем не только данные, но и endpoint клиента
            // size - размер полученных данных. Количество фрэймов может быть другим
            auto datagram_size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            int frame_size = player.GetFrameSize(); // Размер одного фрейма в байтах
            size_t n_frames = datagram_size / frame_size; // Количество фреймов в полученной датаграмме

            // Воспроизводим принятые данные
            player.PlayBuffer(recv_buf.data(), n_frames, 1.5s);
            std::cout << "Playing done" << std::endl;
        }
    }
    catch (std::exception &e) {
        std::cerr << "Server exception: " << e.what() << std::endl;
    }
}

void StartClient(uint16_t port) {
    try {
        // По заданию через аргументы передаётся только порт. Прописываем IP сервера прямо в коде
        // Использую localhost для тестирования локально и чтобы не светить IP на GitHub
        static const char server_IP[] = "localhost";

        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет.
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        Recorder recorder(ma_format_u8, 1);
        

        while (true)
        {
            std::string server_ip;
            std::cout << "Enter server IP address: ";
            std::getline(std::cin, server_ip);

            // Промежуточная строка для получения ввода от пользователя
            std::string str; 
            std::cout << "Press Enter to record message..." << std::endl;
            std::getline(std::cin, str);

            // Производим запись в rec_result
            auto rec_result = recorder.Record(max_n_frames, 1.5s);
            std::cout << "Recording done, " << rec_result.frames << " frames recorded" << std::endl;

            // Вычисляем количество байт для отправки
            int frame_size = recorder.GetFrameSize(); // Размер одного фрейма в байтах
            size_t n_frames = rec_result.frames; // Количество фреймов, полученное от рекордера
            size_t datagram_size = frame_size * n_frames; // Количество байт для отправки

            if (datagram_size > max_buffer_size) {
                std::cout << "Can't send datagram: size record" << datagram_size << " grater then max buffer size " << max_buffer_size << std::endl;
                return;
            }

            // Отправляем данные на сервер
            boost::system::error_code ec;
            auto endpoint = udp::endpoint(net::ip::make_address(server_ip, ec), port);
            socket.send_to(net::buffer(rec_result.data, datagram_size), endpoint);

            std::cout << "Sent " << datagram_size << " bytes to " << server_ip << std::endl;
        }
    }
    catch (std::exception &e) {
        std::cerr << "Client exception: " << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    // Проверка запуска
    if (argc != 3)
    {
        PrintBadArgsMessage(argv[0]);
        return 1;
    }

    std::string appType = std::string(argv[1]);
    
    if (appType != "server"sv && appType != "client"sv)
    {
        PrintBadArgsMessage(argv[0]);
        return 1;
    }

    int port = -1;
    try {
        port = std::stoi(std::string(argv[2]));
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cout << "Bad input: " << argv[2] << " isn't port number" << std::endl;
        return 1;
    }

    if (appType == "client"sv) { // Клиентская часть приложения
        StartClient(port);
    }

    if (appType == "server"sv) { // Серверная часть приложения
        StartServer(port);
    }

    return 0;
}