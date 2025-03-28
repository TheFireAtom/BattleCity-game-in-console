#include <iostream>
#include <thread>      // Для std::this_thread::sleep_for()
#include <chrono>      // Для std::chrono::milliseconds()
#include <windows.h>   // Для функций GetAsyncKeyState() и setColor()
#include <conio.h>     // Альтернатива GetAsyncKeyState(), чтобы всё не ломалось
#include <vector>      // Для создания вектора (динамического массива) вражеских танков
// #include <mutex>       // Напоминалка что нужно будет почитать про mutex-ы и про thread в целом
// #include <atomic>      // Для создания атомарных переменных для предотвращения гонок данных
#include "RandomNum.h" // Кастомная библиотека для генерации случайных чисел (в диапазоне до 2^32 или в заданном диапазоне, который меньше 2^32)

using namespace std::chrono;    // Пространство имён для удобства записи данных, связанных со временем

// Глобальные переменные и другие данные
const int WIDTH = 15;       // Ширина карты
const int HEIGHT = 11;      // Высота карты
bool needRedraw(false);    // Флаг изменения состояния отрисовки карты
bool isRunning(false);   // Флаг изменения состояния потока обработки снаряда (игрока)
// std::mutex mtx; // Мьютекс для фикса бага с неуничтожением первой преграды если танк стоит к ней вплотную
XorShift32 rnd; // Создание объекта класса для создание случайных чисел

void setColor(int color) {      // Функция для установки цвета в командной строке
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
} 

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

    // Tank(int startX, int startY, char dir) : x(startX), y(startY), direction(dir) {
    //     std::cout << "Tank created at (" << x << ", " << y << ") facing " << direction << std::endl;
    // }

    // ~Tank() {
    //     std::cout << "Tank at (" << x << ", " << y << ") destroyed!" << std::endl;
    // }
};

// Структура снаряда
struct Projectile {
    int x, y;
    char direction;
};

// Объявление объектов структур и вектора танков
Projectile playerProjectile = {-1, -1, '\0'};
Tank playerTank = {6, 7, '^'};
std::vector<Tank> enemyTanks; // Глобальный вектор вражеских танков

// Функция для движения танка в определённую сторону и отрисовки соответственного символа
void drawDirSym(int newX, int newY, char directionSymbol, Tank& tank) {
    map[tank.y][tank.x] = '.';                // Очистка старой клетки с символа танка
    tank.x = newX;
    tank.y = newY;
    map[tank.y][tank.x] = directionSymbol;    // Замена новой клетки с . на нужный символ
}

// Функция движения танка
void moveTank(char direction, Tank& tank) {
    int newX = tank.x, newY = tank.y;

    // Определение направления движения танка и проверка на отсутствие коллизии со стенами
    if ((direction == 'W' || direction == 'w')) {
        if (map[tank.y - 1][tank.x] == '.') {
            newY--; // Вверх
        }
        drawDirSym(newX, newY, '^', tank);
        tank.direction = '^';
    } 

    else if ((direction == 'S' || direction == 's')) { 
        if (map[tank.y + 1][tank.x] == '.') {
            newY++; // Вниз
        }
        drawDirSym(newX, newY, 'v', tank);
        tank.direction = 'v';
    } 

    else if (direction == 'A' || direction == 'a') {
        if (map[tank.y][tank.x - 1] == '.') {
            newX--; // Влево
        }
        drawDirSym(newX, newY, '<', tank);
        tank.direction = '<';
    } 

    else if (direction == 'D' || direction == 'd') {
        if (map[tank.y][tank.x + 1] == '.') {
           newX++; // Вправо 
        }
        drawDirSym(newX, newY, '>', tank);
        tank.direction = '>';
    }

    needRedraw = true;

}

// Функция движения снаряда
void moveProjectile(Projectile& projectile) {
    //std::lock_guard<std::mutex> lock(mtx);
    if (projectile.x == -1) return;   // Если снаряд не был выстрелен, функция не изменит его положения

    if (projectile.direction == '^') {    
        projectile.y--;
    }

    else if (projectile.direction == '<') {
        projectile.x--;
    }

    else if (projectile.direction == 'v') {
        projectile.y++;
    }

    else if (projectile.direction == '>') {
        projectile.x++;
    }

    if (projectile.x <= 0 || projectile.x >= WIDTH - 1 || projectile.y <= 0 || projectile.y >= HEIGHT - 2) {
        projectile.x = -1;
        projectile.y = -1;
    }

    needRedraw = true;
}

// Функция выстрела снаряда
void fireProjectile(char tankDir, Projectile& projectile, Tank& tank) {

    if (tankDir == '^') {
        projectile.x = tank.x;
        projectile.y = tank.y; // +
    }

    if (tankDir == 'v') {
        projectile.x = tank.x;
        projectile.y = tank.y; // -
    }

    if (tankDir == '<') {
        projectile.x = tank.x;
        projectile.y = tank.y;
    }

    if (tankDir == '>') {
        projectile.x = tank.x;
        projectile.y = tank.y;
    }

    projectile.direction = tank.direction;

}

// Функция соприкасания снаряда с объектом
void projectileCollision(Projectile& projectile) {
    if (map[projectile.y][projectile.x] == '@') {
        map[projectile.y][projectile.x] = '.';
        projectile.y = -1;
        projectile.x = -1;
    } 
    else if (map[projectile.y][projectile.x] == '#') {
        return;
    }

    needRedraw = true;
}

// Функция для создания объекта вражеского танка
void spawnEnemyTank() {
    uint32_t tankPosition = rnd.rangeNum(1, 4);
    Tank enemyTank {0, 0, '>'};

    if (tankPosition == 1) enemyTank = {1, 1, '>'};
    else if (tankPosition == 2) enemyTank = {13, 1, '>'};
    else if (tankPosition == 3) enemyTank = {1, 8, '>'};
    else if (tankPosition == 4) enemyTank = {13, 8, '>'};

    enemyTanks.push_back(enemyTank); // Добавление танка в глобальный список

    //drawDirSym(enemyTank.x, enemyTank.y, enemyTank.direction, enemyTank);

    return;
}

void enemyTankAi() {
    for (size_t i = 0; i < enemyTanks.size(); ++i) {
        enemyTanks[i].x++;
        drawDirSym(enemyTanks[i].x, enemyTanks[i].y, enemyTanks[i].direction, enemyTanks[i]);
    }

}

// Функция создания задержки между появлением вражеских танков
void spawnDelay(auto& lastSpawnTime, int spawnInterval) {  
    auto now = steady_clock::now();
    auto timeElapsed = duration_cast<milliseconds>(now - lastSpawnTime).count();

    if (timeElapsed >= spawnInterval) {
        spawnEnemyTank();
        lastSpawnTime = now;
    }

    std::cout << enemyTanks.size() << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

// Функция отрисовки карты
void drawMap() {
    if (needRedraw == true) {
        std::cout << "\033[H\033[J";  // ANSI escape code для очистки экрана

        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                bool enemyDrawn = false;    // булева переменная для проверки состояния отрисовки
                for (size_t k = 0; k < enemyTanks.size(); k++) {
                    if (enemyTanks[k].x == j && enemyTanks[k].y == i) {
                        std::cout << enemyTanks[k].direction;
                        enemyDrawn = true;
                        break;  // Выход из цикла после отрисовки танка
                    }
                }

                if (!enemyDrawn) {
                    if (playerProjectile.x == j && playerProjectile.y == i) {
                        if (map[playerProjectile.y][playerProjectile.x] != '#' && map[playerProjectile.y][playerProjectile.x] != playerTank.direction) {
                            std::cout << '*';
                        } 
                        else {
                            std::cout << playerTank.direction;
                        }
                    } 
                    else {
                        std::cout << map[i][j]; 
                    }
                }
                //std::cout << std::endl;
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

    // Начальные значения для подсчёта кол-ва прошедшего времени для спавна вражеских танков с определённым интервалом (2000 мс)
    auto lastSpawnTime = steady_clock::now();
    int spawnInterval = 5000;

    // Очистка консоли (на всякий случай)
    //system("cls");

    // Начальное текстовое меню для удобства пользователя
    setColor(7);    // Обычный (белый) цвет консольного шрифта
    std::cout << "Welcome to BattleCity.exe!" << std::endl << "\n";
    setColor(2);
    std::cout << "Controls: " << std::endl << "\n";
    setColor(2);
    std::cout << "W: move tank one cell up" << std::endl;
    std::cout << "A: move tank one cell left" << std::endl;
    std::cout <<"S: move tank one cell down" << std::endl;
    std::cout <<"D: move tank one cell right" << std::endl;
    std::cout <<"Space: fire a projectile that destroys one enemy or obstacle upon collision" << std::endl;
    std::cout <<"Escape: exit the game" << std::endl << std::endl;

    setColor(4);
    std::cout << "Press 'Enter' key to start the game!" << std::endl;
    setColor(7);
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //std::cout << rnd.randNum() << std::flush;

    //spawnDelay();

    // Реализация начала игры после нажатия кнопки enter
    while (true) {
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            isRunning = true;
            setColor(7);
            break;
        } 
        else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {  // Кнопка для выхода из игры (выполняемой программы)
            isRunning = false;
            system("cls");
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // char startKey = getPressedKey();

    // if (startKey == 'n') { 
    //     isRunning = true;
    // }

   // std::thread playerProjectileThread(asyncMoveProjectile);    // Здесь создаётся отдельный поток для движения снаряда
   // std::thread playerTankThread(asyncMoveTank);                // Аналогично с танком игрока
   
   // std::this_thread::sleep_for(std::chrono::seconds(1));        

    while (isRunning) {

        //std::this_thread::sleep_for(std::chrono::seconds(1));  
        spawnDelay(lastSpawnTime, spawnInterval); // Функция для создания вражеских танков
        enemyTankAi();

        projectileCollision(playerProjectile);
        char key = getPressedKey();

        if (playerProjectile.x != -1) { // Проверка на наличие снаряда на карте
            moveProjectile(playerProjectile);
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));    // Задержка для замедления полёта снаряда
        }

        // Функция обработки нажатия кнопок, новая версия
       
        if (key != '\0') { 
            if (key == 'p') {    // Кнопка для выстрела из танка
                fireProjectile(playerTank.direction, playerProjectile, playerTank);
            }

            else if (key == 'e') {  // Кнопка для выхода из игры (выполняемой программы)
                isRunning = false;
                system("cls");
                exit(0);
            }

            else {
                moveTank(key, playerTank); // Если были нажаты кнопки "w" || "a" || "s" || "d", то танк двигается с места (либо меняет символ в сторону направления 
                                           // движения если не может двинуться из-за преграды)
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

 