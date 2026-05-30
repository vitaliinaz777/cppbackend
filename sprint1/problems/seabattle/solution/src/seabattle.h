#pragma once
#include <array>
#include <cassert>
#include <optional>
#include <iostream>
#include <random>
#include <set>

// Класс SeabattleField представляет собой игровое поле для морского боя.
// Он содержит методы для создания случайного поля, обработки выстрелов и отображения состояния поля. 
// Класс SeabattleAgent отвечает за логику игры, включая обработку ходов и взаимодействие с сокетом 
// для обмена данными между игроками. В функции main реализуется логика запуска сервера или клиента
// в зависимости от аргументов командной строки.
class SeabattleField {
public:
    enum class State {
        UNKNOWN,
        EMPTY,
        KILLED,
        SHIP
    };

    static const size_t field_size = 8;

    SeabattleField(State default_elem = State::UNKNOWN) {
        field_.fill(default_elem);
    }

    // T&& означает универсальную ссылку, которая может принимать как lvalue, так и rvalue объекты. 
    // Это позволяет функции работать с различными типами генераторов случайных чисел, не требуя конкретного типа.
    template <class T>
    static SeabattleField GetRandomField(T&& random_engine) { 
        std::optional<SeabattleField> res;
        do {
            res = TryGetRandomField(random_engine);
        } while (!res);

        return *res;
    }

private:
    // Метод TryGetRandomField пытается создать случайное поле, размещая корабли на игровом поле. 
    // Он использует алгоритм, который случайным образом выбирает позиции и направления для размещения кораблей, 
    // проверяя при этом, что они не пересекаются и не выходят за границы поля.
    template<class T>
    static std::optional<SeabattleField> TryGetRandomField(T&& random_engine) {
        SeabattleField result{State::EMPTY}; // Инициализируем поле пустыми клетками

        std::set<std::pair<size_t, size_t>> availableElements; // Множество доступных клеток для размещения кораблей
        std::vector ship_sizes = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1}; // Размеры кораблей, которые нужно разместить

        // Лямбда-функция для пометки клетки и её окрестностей как занятых, чтобы предотвратить размещение кораблей рядом друг с другом.
        const auto mark_cell_as_used = [&](size_t x, size_t y) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    availableElements.erase({x + dx, y + dy});
                }
            }
        };

        // Лямбда-функция для получения смещения по координатам в зависимости от направления размещения корабля.
        const auto dir_to_dxdy = [](size_t dir) -> std::pair<int, int> {
            int dx = dir == 1 ? 1 : dir == 3 ? -1 : 0;
            int dy = dir == 0 ? 1 : dir == 2 ? -1 : 0;

            return {dx, dy};
        };

        // Лямбда-функция для проверки возможности размещения корабля заданной длины в определённом направлении от начальной позиции (x, y).
        const auto check_ship_available = [&](size_t x, size_t y, size_t dir, int ship_length) {
            auto [dx, dy] = dir_to_dxdy(dir);

            for (int i = 0; i < ship_length; ++i) {
                size_t cx = x + dx * i;
                size_t cy = y + dy * i;

                if (cx >= field_size || cy >= field_size) {
                    return false;
                }
                if (availableElements.count({cx, cy}) == 0) {
                    return false;
                }
            }

            return true;
        };

        // Лямбда-функция для размещения корабля на поле, помечая его клетки как занятые и обновляя множество доступных клеток.
        const auto mark_ship = [&](size_t x, size_t y, size_t dir, int ship_length){
            auto [dx, dy] = dir_to_dxdy(dir);

            for (int i = 0; i < ship_length; ++i) {
                size_t cx = x + dx * i;
                size_t cy = y + dy * i;

                result.Get(cx, cy) = State::SHIP;
                mark_cell_as_used(cx, cy);
            }
        };

        // Инициализируем множество доступных клеток всеми координатами поля.
        for (size_t y = 0; y < field_size; ++y) {
            for (size_t x = 0; x < field_size; ++x) {
                availableElements.insert({x, y});
            }
        }

        using Distr = std::uniform_int_distribution<size_t>; // Тип генератора случайных чисел для выбора случайных позиций и направлений размещения кораблей.
        using Param = Distr::param_type; // Тип параметров для генератора случайных чисел.

        static const int max_attempts = 100; // Максимальное количество попыток для размещения корабля, чтобы избежать бесконечного цикла в случае невозможности разместить корабль.

        // Проходим по каждому размеру корабля и пытаемся разместить его на поле, выбирая случайные позиции и направления до тех пор, пока не удастся успешно разместить корабль.
        for (int length : ship_sizes) {
            std::pair<size_t, size_t> pos;
            size_t direction;
            int attempt = 0;

            Distr d;
            do {
                if (attempt++ >= max_attempts || availableElements.empty()) {
                    return std::nullopt;
                }

                size_t pos_index = d(random_engine, Param(0, availableElements.size() - 1));
                direction = d(random_engine, Param(0, 3));
                pos = *std::next(availableElements.begin(), pos_index);
            } while (!check_ship_available(pos.first, pos.second, direction, length));
            mark_ship(pos.first, pos.second, direction, length);
        }

        return result; // Возвращаем успешно созданное игровое поле с размещёнными кораблями.
    }

    // Метод IsKilledInDirection проверяет, уничтожен ли корабль в определённом направлении от клетки (x, y).
    bool IsKilledInDirection(size_t x, size_t y, int dx, int dy) const {
        for (; x < field_size && y < field_size; x += dx, y += dy) {
            if (Get(x, y) == State::EMPTY) {
                return true;
            }
            if (Get(x, y) != State::KILLED) {
                return false;
            }
        }
        return true;
    }

    // Метод MarkKillInDirection помечает клетки в определённом направлении от клетки (x, y) как пустые, если они находятся 
    // в пределах поля и не были уже открыты.
    void MarkKillInDirection(size_t x, size_t y, int dx, int dy) {
        auto mark_cell_empty = [this](size_t x, size_t y) {
            if (x >= field_size || y >= field_size) {
                return;
            }
            if (Get(x, y) != State::UNKNOWN) {
                return;
            }
            Get(x, y) = State::EMPTY;
        };
        for (; x < field_size && y < field_size; x += dx, y += dy) {
            mark_cell_empty(x + dy, y + dx);
            mark_cell_empty(x - dy, y - dx);
            mark_cell_empty(x, y);
            if (Get(x, y) != State::KILLED) {
                return;
            }
        }
    }

public:    
    // Перечисление ShotResult определяет возможные результаты выстрела: промах (MISS), попадание (HIT) и уничтожение (KILL).
    enum class ShotResult {
        MISS = 0,
        HIT  = 1,
        KILL = 2
    };

    // Метод Shoot обрабатывает выстрел по координатам (x, y) и возвращает результат выстрела в виде перечисления ShotResult,
    // которое может быть MISS, HIT или KILL.
    ShotResult Shoot(size_t x, size_t y) {
        if (Get(x, y) != State::SHIP) return ShotResult::MISS;

        Get(x, y) = State::KILLED;
        --weight_;

        return IsKilled(x, y) ? ShotResult::KILL : ShotResult::HIT;
    }

    // Метод MarkMiss помечает клетку (x, y) как пустую, если она была неизвестной.
    void MarkMiss(size_t x, size_t y) {
        if (Get(x, y) != State::UNKNOWN) {
            return;
        }
        Get(x, y) = State::EMPTY;
    }

    // Метод MarkHit помечает клетку (x, y) как поражённую, если она была неизвестной, и уменьшает вес кораблей на единицу.
    void MarkHit(size_t x, size_t y) {
        if (Get(x, y) != State::UNKNOWN) {
            return;
        }
        --weight_;
        Get(x, y) = State::KILLED;
    }

    // Метод MarkKill помечает клетку (x, y) как поражённую и помечает все клетки в направлении от (x, y) как пустые, 
    // если они были неизвестными.
    void MarkKill(size_t x, size_t y) {
        if (Get(x, y) != State::UNKNOWN) {
            return;
        }
        MarkHit(x, y);
        MarkKillInDirection(x, y, 1, 0);
        MarkKillInDirection(x, y, -1, 0);
        MarkKillInDirection(x, y, 0, 1);
        MarkKillInDirection(x, y, 0, -1);
    }

    // Оператор () позволяет обращаться к клеткам игрового поля по координатам (x, y) и получать их состояние.
    State operator()(size_t x, size_t y) const {
        return Get(x, y);
    }

    // Метод IsKilled проверяет, уничтожен ли корабль в клетке (x, y) и во всех четырёх направлениях от неё.
    bool IsKilled(size_t x, size_t y) const {
        return IsKilledInDirection(x, y, 1, 0) && IsKilledInDirection(x, y, -1, 0)
            && IsKilledInDirection(x, y, 0, 1) && IsKilledInDirection(x, y, 0, -1);
    }

    // Метод Print выводит игровое поле в заданный поток вывода, отображая состояние каждой клетки с помощью символов.
    static void PrintDigitLine(std::ostream& out) {
        out << "  1 2 3 4 5 6 7 8  ";
    }

    // Метод PrintLine выводит строку игрового поля с индексом y, отображая состояние каждой клетки в этой строке.
    void PrintLine(std::ostream& out, size_t y) const {
        std::array<char, field_size * 2 - 1> line;
        for (size_t x = 0; x < field_size; ++x) {
            line[x * 2] = Repr((*this)(x, y));
            if (x + 1 < field_size) {
                line[x * 2 + 1] = ' ';
            }
        }

        char line_char = static_cast<char>('A' + y);

        out.put(line_char);
        out.put(' ');
        out.write(line.data(), line.size());
        out.put(' ');
        out.put(line_char);
    }

    bool IsLoser() const {
        return weight_ == 0;
    }

private:

    // Метод Get возвращает ссылку на состояние клетки на игровом поле по координатам (x, y).
    State& Get(size_t x, size_t y) {
        return field_[x + y * field_size];
    }

    // Константная версия метода Get возвращает значение состояния клетки на игровом поле по координатам (x, y).
    State Get(size_t x, size_t y) const {
        return field_[x + y * field_size];
    }

    // Метод Repr преобразует состояние клетки в символ для отображения на игровом поле.
    static char Repr(State state) {
        switch (state) {
            case State::UNKNOWN:
                return '?';
            case State::EMPTY:
                return '.';
            case State::SHIP:
                return 'o';
            case State::KILLED:
                return 'x';
        }

        return '\0';
    }

private:
    // Одномерный массив для хранения состояния каждой клетки игрового поля.
    std::array<State, field_size * field_size> field_; 

    // Вес кораблей, который уменьшается при попадании и уничтожении кораблей. 
    // Изначально он равен сумме количества клеток, занимаемых всеми кораблями.
    int weight_ = 1 * 4 + 2 * 3 + 3 * 2 + 4 * 1;  
};
