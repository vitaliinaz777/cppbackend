/*
Задание 2 — Морской бой по TCP
Возьмите заготовку кода из папки sprint1/problems/seabattle своего репозитория.
Правила игры: каждый игрок на поле 8х8 располагает 10 кораблей: 1 четырёхпалубный (занимающий 4 клетки), 2 трёхпалубных, 3 двухпалубных и 4 однопалубных. Корабли не могут накладываться друг на друга, а также соприкасаться по горизонтали, диагонали и вертикали.
Игроки делают выстрелы, не видя расположение кораблей соперника. При промахе ход передаётся другому игроку. При попадании даётся право ещё одного выстрела. Соперник сообщает результат выстрела — «мимо», «попал», «убил». Последний статус означает, что поражены все клетки, занимаемые кораблём.
Расстановка кораблей производится случайно, она определяется сидом, который передаётся при запуске игры.
Механика игрового поля реализована в классе SeabattleField, задающем игровое поле. Этот класс реализован полностью, менять его не нужно. Вам понадобятся следующие методы:
Статический метод GetRandomField, принимающий генератор случайных чисел. Используется для инициализации поля игрока.
Конструктор. Используется для инициализации поля соперника.
Shoot(size_t x, size_t y) — произвести выстрел по сектору: получить результат выстрела. Используется для своего поля при выстреле соперника.
MarkMiss(size_t x, size_t y), MarkHit(size_t x, size_t y), MarkKill(size_t x, size_t y). Отметить результат выстрела. Используется для поля соперника.
IsLoser — возвращает true, если все корабли на поле уничтожены.
Механика игры частично реализована в классе SeabattleAgent. Вам пригодятся следующие методы и поля класса:
Конструктор — принимает поле игрока с уже расставленными кораблями.
StartGame (заготовка) — этот метод должен содержать всю логику игры. Вам предстоит его реализовать.
ParseMove — преобразует текстовое представление клетки, состоящее из буквы и цифры (например, B6) в координаты. Возвращает пару чисел, в которой первый элемент соответствует букве (значение от 0 до 7), а второй - цифре (также от 0 до 7). В случае некорректной клетки возвращает std::nullopt.
MoveToString — преобразует координаты в текстовое представление клетки.
PrintFields — выводит в cout два поля: игрока и соперника.
IsGameEnded — возвращает true, если игра завершена.
my_field_ — поле игрока.
other_field_ — поле соперника.
Каждая клетка поля может находиться в одном из следующих статусов:
UNKNOWN — содержимое неизвестно. Применяется для поля соперника по умолчанию.
EMPTY — клетка пуста. Применяется как для поля игрока, так и для открытых клеток соперника.
KILLED — поражённая клетка с кораблём. Применяется как для поля игрока, так и для открытых клеток соперника.
SHIP — клетка с непоражённым кораблём. Применяется для поля игрока.
Клиент и сервер будут играть по сети. Каждый из них независимо выбирает сид. Сид — это значение, которое позволяет инициализировать счётчик псевдослучайных чисел и определяет все последующие значения. Если запустить приложение, использующее в своей работе псевдослучайные числа, с известным сидом, результат работы можно предсказать. Сид определяет расположение кораблей и задаётся параметром командной строки.
Начинает клиент.
Протокол взаимодействия
При выстреле игрок передаёт другому игроку два байта, содержащие текстовое представление координаты, например, C7. Соперник сообщает ответ, содержащий один байт — результат выстрела. Он кодируется преобразованием значения типа SeabattleField::ShotResult к char.
Параметры командной строки
Клиент и сервер — одна программа. Различие будет в параметрах командной строки.
Сервер принимает два параметра — сид и порт.
Клиент принимает три параметра — сид, IP-сервера, порт.

Подсказки:
Реализуйте функции StartClient и StartServer, которые создают экземпляр класса SeabattleAgent и вызывают его метод StartGame, а также инициализируют подключение.
Реализуйте методы ReadMove, ReadResult для получения хода и результата выстрела через сокет, а также методы SendResult и SendMove для отправки этих данных.
В методе StartGame используйте булев параметр, который определяет, чей сейчас ход: игрока или соперника.
В методе StartGame используйте цикл с условием !IsGameEnded().
*/

#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        // TODO: реализуйте самостоятельно
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно


    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    // Если указано 3 аргумента, то запускаем сервер, иначе - клиент
    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
