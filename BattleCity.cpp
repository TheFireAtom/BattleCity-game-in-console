#include <iostream>
#include <thread>      // Для std::this_thread::sleep_for()
#include <chrono>      // Для std::chrono::milliseconds()
#include <windows.h>   // Для GetAsyncKeyState()
#include <conio.h>     // Альтернатива GetAsyncKeyState(), чтобы всё не ломалось
#include <mutex>       // Напоминалка что нужно будет почитать про mutex-ы и про thread в целом
#include <atomic>      // Для создания атомарных переменных для предотвращения гонок данных

// Глобальные переменные и другие данные
const int WIDTH = 15;       // Ширина карты
const int HEIGHT = 11;      // Высота карты
bool needRedraw(false);    // Флаг изменения состояния отрисовки карты
bool isRunning(true);   // Флаг изменения состояния потока обработки снаряда (игрока)
std::mutex mtx; // Мьютекс для фикса бага с неуничтожением первой преграды если танк стоит к ней вплотную

// Карта из символов
char map[HEIGHT][WIDTH + 1] = {
    "###############",  // Границы карты обозначены знаками # 
    "#.............#",
    "#.@@@.....@@@.#",  // Разрушаемые стены обозначены знаками @
    "#.@@@.@@@.@@@.#",
    "#.....@@@.....#",
    "#.@@@.@@@.@@@.#", 
    "#.@@@.....@@@.#",
    "#.....^.......#",  // Игрок обозначен знаком ^
    "#.............#",  // Помимо этого в будущем добавятся танки врага, которые будут обозначаться другими символами
    "###############"
};

// Структуры были использованы вместо классов из-за того, что по умолчанию у всех элементов выставлены public поля

// Структура танка
struct Tank {
    int x, y;
    char direction;
};

// Структура снаряда
struct Projectile {
    int x, y;
    char direction;
};

// Объявление объектов структур
Projectile playerProjectile = { -1, -1, '\0'};
Projectile playerTank = { 6, 7, '^'};

// Функция для движения танка в определённую сторону и отрисовки соответственного символа
void drawDirSym(int newX, int newY, char directionSymbol) {
    //std::lock_guard<std::mutex> lock(mtx);
    if (map[newY][newX] == '.') {
        map[playerTank.y][playerTank.x] = '.';                // Очистка старой клетки с T
        playerTank.x = newX;
        playerTank.y = newY;
        map[playerTank.y][playerTank.x] = directionSymbol;    // Замена новой клетки с . на нужный символ
    } else {
        map[playerTank.y][playerTank.x] = directionSymbol;
    }
}

// Функция движения танка
void moveTank(char direction) {
    int newX = playerTank.x, newY = playerTank.y;

    // Определение направления движения танка
    if (direction == 'W' || direction == 'w') { 
        newY--; // Вверх
        drawDirSym(newX, newY, '^');
        playerTank.direction = '^';
    }

    else if (direction == 'S' || direction == 's') {
        newY++; // Вниз
        drawDirSym(newX, newY, 'v');
        playerTank.direction = 'v';
    }

    else if (direction == 'A' || direction == 'a') {
        newX--; // Влево
        drawDirSym(newX, newY, '<');
        playerTank.direction = '<';

    }

    else if (direction == 'D' || direction == 'd') {
        newX++; // Вправо
        drawDirSym(newX, newY, '>');
        playerTank.direction = '>';
    }

    needRedraw = true;

}

// Функция движения снаряда
void moveProjectile() {
    //std::lock_guard<std::mutex> lock(mtx);
    if (playerProjectile.x == -1) return;   // Если снаряд не был выстрелен, функция не изменит его положения

    if (playerProjectile.direction == '^') {    
        playerProjectile.y--;
    }

    else if (playerProjectile.direction == '<') {
        playerProjectile.x--;
    }

    else if (playerProjectile.direction == 'v') {
        playerProjectile.y++;
    }

    else if (playerProjectile.direction == '>') {
        playerProjectile.x++;
    }

    if (playerProjectile.x <= 0 || playerProjectile.x >= WIDTH - 1 || playerProjectile.y <= 0 || playerProjectile.y >= HEIGHT - 2) {
        playerProjectile.x = -1;
        playerProjectile.y = -1;
    }

    needRedraw = true;
}

// Функция выстрела снаряда
void fireProjectile(char tankDir) {

        if (tankDir == '^') {
            playerProjectile.x = playerTank.x;
            playerProjectile.y = playerTank.y - 1; // +
        }

        if (tankDir == 'v') {
            playerProjectile.x = playerTank.x;
            playerProjectile.y = playerTank.y + 1; // -
        }

        if (tankDir == '<') {
            playerProjectile.x = playerTank.x - 1;
            playerProjectile.y = playerTank.y;
        }

        if (tankDir == '>') {
            playerProjectile.x = playerTank.x + 1;
            playerProjectile.y = playerTank.y;
        }

        playerProjectile.direction = playerTank.direction;

}

// Функция соприкасания снаряда с объектом
void projectileCollision() {
    if (map[playerProjectile.y][playerProjectile.x] == '@') {
        map[playerProjectile.y][playerProjectile.x] = '.';
        playerProjectile.y = -1;
        playerProjectile.x = -1;
    }

    needRedraw = true;
}

// Функция отрисовки карты
void drawMap() {
    std::lock_guard<std::mutex> lock(mtx);
    if (needRedraw == true) {
        system("cls");
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                if (playerProjectile.x == j && playerProjectile.y == i) {
                    std::cout << '*';
                } 
                // else if (playerTank.x == j && playerTank.y == i) {
                //     std::cout << playerTank.direction;
                // }
                else {
                    std::cout << map[i][j]; 
                }
            }
            std::cout << std::endl;
        }
        needRedraw = false;
    }
}

// Функция обработки нажатия кнопок
char getPressedKey() {
    if (GetAsyncKeyState('W') & 0x8000) return 'w'; // 0x8000 это по сути шестнадцатеричная побитовая маска
    if (GetAsyncKeyState('S') & 0x8000) return 's';
    if (GetAsyncKeyState('A') & 0x8000) return 'a';
    if (GetAsyncKeyState('D') & 0x8000) return 'd';

    if (GetAsyncKeyState(' ') & 0x8000) return 'p'; // Пробел отвечает за выстрел танка игрока

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return 'e'; // VK_ESCAPE это esc на клавиатуре, возвращает символ e

    return '\0';    // В случае отсутствия возврата кнопки возвращается пустой символ
}

// В данной версии игры многопоточность не нужна, она только усложняет процесс написания кода и добавляет новые баги
// Функция создания дополнительного фонового потока для независимого движения снаряда
// void asyncMoveProjectile() {
//     //std::lock_guard<std::mutex> lock(mtx);
//     while (isRunning) {
//         if (playerProjectile.x != -1) { // Проверка на наличие снаряда на карте
//             moveProjectile();
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Задержка для замедления полёта снаряда
//         }
//     }
// }

// void asyncMoveTank() {
//     //std::lock_guard<std::mutex> lock(mtx);
//     while (isRunning) {
//         char key = getPressedKey();
//         if (key != '\0') {
//             moveTank(key);
//         }
        
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Задержка для замедления полёта снаряда
//     }
// }

int main() {
    std::cout << "Press 'w' key to start the game!" << '\n';

   // std::thread playerProjectileThread(asyncMoveProjectile);    // Здесь создаётся отдельный поток для движения снаряда
   // std::thread playerTankThread(asyncMoveTank);                // Аналогично с танком игрока
                                                                  
    while (isRunning) {

        projectileCollision();
        char key = getPressedKey();

        if (playerProjectile.x != -1) { // Проверка на наличие снаряда на карте
            moveProjectile();
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));    // Задержка для замедления полёта снаряда
        }

        // Функция обработки нажатия кнопок, новая версия
       
        if (key != '\0') { 
            if (key == 'p') {    // Кнопка для выстрела из танка
                fireProjectile(playerTank.direction);
            }

            else if (key == 'e') {  // Кнопка для выхода из игры (выполняемой программы)
                isRunning = false;
                system("cls");
                exit(0);
            }

            else {
                moveTank(key);
            }
        }

        if (needRedraw) {
            drawMap();
            needRedraw = false; // Зануляем флаг
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    }
    // playerProjectileThread.join();
    // playerTankThread.join();
    return 0;
}

