#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <clocale>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const char EMPTY = '.';
const char WALL = '#';
const char PC_CHAR = 'P';
const char VRAG_CHAR = 'E';
const char TRAP = '^';
const char TREASURE = 'T';

struct Character {
    int x, y;
    int hp;
    int mp;
    int arm;
    int dmg;
    bool isBoss;
    int bonusChance;
    vector<string> inventory;
};

struct Zadacha {
    string name;
    string description;
    bool completed;
};

class IInteractive {
public:
    virtual void interact(Character &pc) = 0;
    virtual ~IInteractive() {}
};

class ItemPickup : public IInteractive {
    string itemName;
    int bonus;
    bool picked;
public:
    ItemPickup(const string &name, int bonusValue) : itemName(name), bonus(bonusValue), picked(false) {}
    void interact(Character &pc) override {
        if (!picked) {
            cout << "Вы подобрали " << itemName << "! (Бонус +" << bonus << "%)" << endl;
            picked = true;
            pc.bonusChance += bonus;
            pc.inventory.push_back(itemName);
        }
    }
};

class Apteka : public IInteractive {
    bool used;
public:
    Apteka() : used(false) {}
    void interact(Character &pc) override {
        if (!used) {
            cout << "Вы подобрали аптечку! +20 HP" << endl;
            used = true;
            pc.hp += 20;
            pc.inventory.push_back("Аптечка");
        }
    }
};

void animateNPCBattle() {
    vector<string> frames = {
        "  [*]   ",
        " [***]  ",
        "[*****] ",
        " [***]  ",
        "  [*]   ",
        " Бой с врагом!"
    };
    for (const auto &frame : frames) {
        cout << frame << endl;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
}

void animateBossCrazenie() {
    vector<string> frames = {
        "  _____   ",
        " /     \\  ",
        "| BOSS! | ",
        " \\_____/  ",
        " Финальная битва!"
    };
    for (const auto &frame : frames) {
        cout << frame << endl;
        this_thread::sleep_for(chrono::milliseconds(250));
    }
}

class Location {
protected:
    string name;
    vector<vector<char>> map;
    int mapWidth, mapHeight;
    vector<Character> vragi;
    vector<IInteractive*> interactives;
public:
    Zadacha zadacha;
    Location(const string &locName, int width, int height)
        : name(locName), mapWidth(width), mapHeight(height)
    {
        map.assign(mapHeight, vector<char>(mapWidth, EMPTY));
        for (int i = 0; i < mapHeight; i++) {
            map[i][0] = WALL;
            map[i][mapWidth-1] = WALL;
        }
        for (int j = 0; j < mapWidth; j++) {
            map[0][j] = WALL;
            map[mapHeight-1][j] = WALL;
        }
        zadacha.completed = false;
    }
    virtual ~Location() {
        for (auto obj : interactives)
            delete obj;
    }
    // Публичный геттер для доступа к врагам
    vector<Character>& getEnemies() {
        return vragi;
    }
    virtual void init() = 0;
    virtual void render(const Character &pc) {
        for (int i = 0; i < mapHeight; i++) {
            for (int j = 0; j < mapWidth; j++) {
                if (i == pc.y && j == pc.x)
                    cout << PC_CHAR;
                else {
                    bool printed = false;
                    for (const auto &vrag : vragi) {
                        if (vrag.x == j && vrag.y == i) {
                            cout << VRAG_CHAR;
                            printed = true;
                            break;
                        }
                    }
                    if (!printed)
                        cout << map[i][j];
                }
            }
            cout << endl;
        }
        cout << "Локация: " << name << endl;
        cout << "Квест: " << zadacha.name << " - " << (zadacha.completed ? "Выполнен" : zadacha.description) << endl;
    }
    virtual void updateNPCs() {
        for (auto &vrag : vragi) {
            int dx = 0, dy = 0;
            int move = rand() % 4;
            switch (move) {
                case 0: dx = 1; break;
                case 1: dx = -1; break;
                case 2: dy = 1; break;
                case 3: dy = -1; break;
            }
            int newX = vrag.x + dx;
            int newY = vrag.y + dy;
            if (isWalkable(newX, newY))
                vrag.x = newX, vrag.y = newY;
        }
    }
    bool isWalkable(int x, int y) {
        if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
            return false;
        return map[y][x] != WALL;
    }
    virtual void movePC(Character &pc, int dx, int dy) {
        int newX = pc.x + dx;
        int newY = pc.y + dy;
        if (isWalkable(newX, newY)) {
            pc.x = newX;
            pc.y = newY;
            if (map[newY][newX] == TRAP) {
                int trapDamage = 10;
                pc.hp -= trapDamage;
                cout << "Попадание в ловушку! Урон " << trapDamage << " HP." << endl;
            }
            if (!zadacha.completed && map[newY][newX] == TREASURE) {
                zadacha.completed = true;
                cout << "Вы выполнили квест: " << zadacha.name << "!" << endl;
                map[newY][newX] = EMPTY;
                pc.bonusChance += 20;
            }
        } else {
            cout << "Нельзя туда пройти!" << endl;
        }
    }
    virtual void processInteraction(Character &pc) {
        for (auto obj : interactives)
            obj->interact(pc);
    }
    virtual Location* nextLocation() {
        return nullptr;
    }
    virtual bool isBossArena() { return false; }
    virtual bool attackNPC(Character &pc) {
        for (size_t i = 0; i < vragi.size(); i++) {
            if (abs(pc.x - vragi[i].x) + abs(pc.y - vragi[i].y) == 1) {
                animateNPCBattle();
                int dmg = pc.dmg - vragi[i].arm;
                if (dmg < 0) dmg = 0;
                vragi[i].hp -= dmg;
                cout << "Вы нанесли " << dmg << " урона врагу!" << endl;
                if (vragi[i].hp <= 0) {
                    cout << "Враг повержен!" << endl;
                    pc.bonusChance += 5;
                    vragi.erase(vragi.begin() + i);
                }
                return true;
            }
        }
        cout << "Врагов в зоне атаки нет!" << endl;
        return false;
    }
};

class ObuchaemayaZona : public Location {
public:
    ObuchaemayaZona(int width, int height) : Location("Обучающая зона", width, height) {}
    void init() override {
        for (int i = 1; i < mapHeight - 1; i++)
            for (int j = 1; j < mapWidth - 1; j++)
                if (rand() % 100 < 10)
                    map[i][j] = WALL;
        map[3][3] = TREASURE;
        zadacha.name = "Сбор трав";
        zadacha.description = "Найдите и соберите травы (объект обозначен символом T)";
        interactives.push_back(new ItemPickup("Травы", 10));
        // Добавляем слабых врагов и аптечки
        Character vrag;
        vrag.x = 6; vrag.y = 4;
        vrag.hp = 20; vrag.mp = 0; vrag.arm = 1; vrag.dmg = 4; vrag.isBoss = false;
        vragi.push_back(vrag);
        interactives.push_back(new Apteka());
    }
    Location* nextLocation() override;
};

class Derevnya : public Location {
public:
    Derevnya() : Location("Деревня", 15, 15) {}
    void init() override {
        map[2][2] = 'S';
        map[2][12] = 'W';
        map[10][7] = 'H';
        map[13][7] = 'D';
        zadacha.name = "Крысы в подвале";
        zadacha.description = "Помогите изгнать крыс из подвала деревни! (вход обозначен 'D')";
        Character vrag;
        vrag.x = 7; vrag.y = 13;
        vrag.hp = 30; vrag.mp = 0; vrag.arm = 1; vrag.dmg = 5; vrag.isBoss = false;
        vragi.push_back(vrag);
        interactives.push_back(new ItemPickup("Меч", 15));
        interactives.push_back(new Apteka());
    }
    Location* nextLocation() override;
};

class LesOkhoty : public Location {
public:
    LesOkhoty() : Location("Лес охоты", 12, 12) {}
    void init() override {
        for (int i = 1; i < mapHeight - 1; i++)
            for (int j = 1; j < mapWidth - 1; j++) {
                int roll = rand() % 100;
                if (roll < 10)
                    map[i][j] = WALL;
                else if (roll >= 10 && roll < 15)
                    map[i][j] = TRAP;
            }
        map[4][4] = TREASURE;
        zadacha.name = "Сбор шкур";
        zadacha.description = "Найдите и соберите шкуры (объект обозначен символом T)";
        interactives.push_back(new ItemPickup("Шкура", 20));
        // Дополнительные враги и аптечки
        Character vrag;
        vrag.x = 3; vrag.y = 6;
        vrag.hp = 25; vrag.mp = 0; vrag.arm = 1; vrag.dmg = 4; vrag.isBoss = false;
        vragi.push_back(vrag);
        interactives.push_back(new Apteka());
    }
    Location* nextLocation() override;
};

class BossSrazhenie : public Location {
public:
    BossSrazhenie() : Location("Арена Босса", 10, 10) {}
    void init() override {
        for (int i = 1; i < mapHeight - 1; i++)
            for (int j = 1; j < mapWidth - 1; j++)
                map[i][j] = EMPTY;
        Character boss;
        boss.x = mapWidth / 2;
        boss.y = mapHeight / 2;
        boss.hp = 150;
        boss.mp = 50;
        boss.arm = 10;
        boss.dmg = 30;
        boss.isBoss = true;
        boss.bonusChance = 0;
        vragi.push_back(boss);
        zadacha.name = "Битва с боссом";
        zadacha.description = "Победите босса, чтобы стать ЧЕМПИОНОМ!";
    }
    Location* nextLocation() override { return nullptr; }
    bool isBossArena() override { return true; }
};

Location* ObuchaemayaZona::nextLocation() {
    return new Derevnya();
}
Location* Derevnya::nextLocation() {
    return new LesOkhoty();
}
Location* LesOkhoty::nextLocation() {
    return new BossSrazhenie();
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

class IgraManager {
private:
    Character pc;
    Location* currentLocation;
    bool gameOver;
    string nickname;
public:
    IgraManager() : currentLocation(nullptr), gameOver(false) {
        pc.x = 5; pc.y = 5;
        pc.hp = 100; pc.mp = 50;
        pc.arm = 5; pc.dmg = 20;
        pc.isBoss = false;
        pc.bonusChance = 0;
    }
    ~IgraManager() {
        if (currentLocation)
            delete currentLocation;
    }
    void init() {
        cout << "Введите ваш никнейм: ";
        cin >> nickname;
        int w, h;
        cout << "Задайте размеры обучающей зоны (ширина высота): ";
        cin >> w >> h;
        currentLocation = new ObuchaemayaZona(w, h);
        currentLocation->init();
    }
    void run() {
        while (!gameOver && pc.hp > 0) {
#ifdef _WIN32
            SetConsoleCP(65001);
            SetConsoleOutputCP(65001);
#endif
            clearScreen();
            currentLocation->render(pc);
            showInterface();
            int action;
            cin >> action;
            processAction(action);
            if (!gameOver)
                currentLocation->updateNPCs();
        }
        if (pc.hp <= 0)
            cout << "\n" << nickname << ", вы проиграли! PC погиб." << endl;
    }
    void showInterface() {
        cout << "\n" << nickname << " | HP: " << pc.hp << " | MP: " << pc.mp 
             << " | Бонус шанса: " << pc.bonusChance << "%" << endl;
        cout << "Инвентарь: ";
        if (pc.inventory.empty())
            cout << "пусто";
        else {
            for (const auto &item : pc.inventory)
                cout << item << " ";
        }
        cout << "\nВыберите действие:\n"
             << "1. Двигаться (w, a, s, d)\n"
             << "2. Взаимодействовать с объектами\n"
             << "3. Переход в следующую локацию\n"
             << "4. Показать инвентарь\n"
             << "5. Атаковать NPC\n"
             << "6. Выход\n";
    }
    void bossCrazenie() {
        animateBossCrazenie();
        vector<Character>& enemies = currentLocation->getEnemies();
        Character boss = currentLocation->isBossArena() && !enemies.empty() ? enemies[0] : Character();
        if (pc.bonusChance > 60) {
            cout << "Ваш бонусный шанс выше 60%! Босс ослаблен!" << endl;
            boss.hp = pc.hp - 1;
            boss.dmg = pc.dmg - 1;
            enemies[0] = boss;
        }
        while (pc.hp > 0 && boss.hp > 0) {
            int pdmg = pc.dmg - boss.arm;
            if (pdmg < 0) pdmg = 0;
            boss.hp -= pdmg;
            cout << "Вы нанесли " << pdmg << " урона боссу. (Босс HP: " << boss.hp << ")" << endl;
            if (boss.hp <= 0) break;
            int bdmg = boss.dmg - pc.arm;
            if (bdmg < 0) bdmg = 0;
            pc.hp -= bdmg;
            cout << "Босс нанес " << bdmg << " урона вам. (Ваш HP: " << pc.hp << ")" << endl;
            this_thread::sleep_for(chrono::milliseconds(500));
            if (pc.hp <= 0) break;
        }
        if (pc.hp > 0) {
            cout << "\nЧЕМПИОН! Вы победили босса!" << endl;
        } else {
            cout << "\nБосс победил. Вы проиграли битву!" << endl;
        }
        gameOver = true;
    }
    void processAction(int action) {
        switch (action) {
            case 1: {
                cout << "Введите направление (w=вверх, s=вниз, a=влево, d=вправо): ";
                char dir;
                cin >> dir;
                if (dir == 'w') currentLocation->movePC(pc, 0, -1);
                else if (dir == 's') currentLocation->movePC(pc, 0, 1);
                else if (dir == 'a') currentLocation->movePC(pc, -1, 0);
                else if (dir == 'd') currentLocation->movePC(pc, 1, 0);
                break;
            }
            case 2: {
                currentLocation->processInteraction(pc);
                break;
            }
            case 3: {
                if (currentLocation->isBossArena()) {
                    bossCrazenie();
                } else {
                    Location* next = currentLocation->nextLocation();
                    if (next != nullptr) {
                        delete currentLocation;
                        currentLocation = next;
                        currentLocation->init();
                    } else {
                        cout << "Переход невозможен! Вы на последней локации." << endl;
#ifdef _WIN32
                        system("pause");
#else
                        system("read -n 1 -s -p \"Нажмите любую клавишу для продолжения...\"; echo");
#endif
                    }
                }
                break;
            }
            case 4: {
                cout << "\nИнвентарь: ";
                if (pc.inventory.empty())
                    cout << "пусто";
                else {
                    for (const auto &item : pc.inventory)
                        cout << item << " ";
                }
                cout << "\nНажмите любую клавишу для продолжения...";
#ifdef _WIN32
                system("pause");
#else
                system("read -n 1 -s -p \"\"; echo");
#endif
                break;
            }
            case 5: {
                currentLocation->attackNPC(pc);
#ifdef _WIN32
                system("pause");
#else
                system("read -n 1 -s -p \"Нажмите любую клавишу для продолжения...\"; echo");
#endif
                break;
            }
            case 6: {
                gameOver = true;
                break;
            }
            default: {
                cout << "Неверный выбор!" << endl;
#ifdef _WIN32
                system("pause");
#else
                system("read -n 1 -s -p \"Нажмите любую клавишу для продолжения...\"; echo");
#endif
                break;
            }
        }
    }
};

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    srand(static_cast<unsigned>(time(0)));
    IgraManager game;
    game.init();
    game.run();
    return 0;
}
