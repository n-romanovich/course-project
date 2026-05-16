#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <Windows.h>

#include "include/utils.h"
#include "include/data.h"

using namespace std;

string logContent = "";

//Записать действие в лог
void writeLog(const char* action, const char* detail)
{
    time_t now = time(NULL);
    struct tm t;

    localtime_s(&t, &now);
    char timeBuf[32];

    sprintf_s(timeBuf, 32, "[%02d.%02d.%04d %02d:%02d:%02d]",
        t.tm_mday, t.tm_mon + 1, t.tm_year + 1900,
        t.tm_hour, t.tm_min, t.tm_sec);

    ofstream f("data/log.txt", ios::app);

    if (!f.is_open()) return;
    f << timeBuf << " " << action << " | " << detail << "\n";

    f.close();
}

//Загрузить лог из файла
void loadLog()
{
    logContent.clear();
    ifstream f("data/log.txt");

    if (!f.is_open()) { logContent = "data/log.txt not found"; return; }

    string line;
    while (getline(f, line)) { 
        logContent += line; logContent += "\n"; 
    }

    f.close();

    if (logContent.empty()) logContent = "(log is empty)";
}

//Получить текущую дату
void getCurrentDate(char* buf, int bufSize)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    sprintf_s(buf, bufSize, "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
}

//Преобразовать строку даты в time_t
time_t parseDate(const char* s)
{
    int d = 0, m = 0, y = 0;
    sscanf_s(s, "%d.%d.%d", &d, &m, &y);

    if (y < 1970) return 0;

    struct tm t = {};
    t.tm_mday = d; t.tm_mon = m - 1; t.tm_year = y - 1900;

    return mktime(&t);
}

//Проверка заявки на просроченность
bool isOverdue(const Repair& r)
{
    if (strcmp(r.status, reinterpret_cast<const char*>(u8"готов")) == 0) return false;

    if (r.days <= 0 || r.dateAdded[0] == '\0') return false;
    time_t added = parseDate(r.dateAdded);

    if (added == 0) return false;
    int daysPassed = (int)(difftime(time(NULL), added) / 86400.0);

    return daysPassed > r.days;
}

//Сортировка пузырьком
void bubbleSort()
{
    int n = (int)repairs.size();
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (repairs[j].cost > repairs[j + 1].cost) {
                Repair tmp = repairs[j];
                repairs[j] = repairs[j + 1];
                repairs[j + 1] = tmp;
            }
}

//Построить строку для combo мастеров
void buildMastersComboStr(char* buf, int bufSize)
{
    buf[0] = '\0';
    int pos = 0;

    for (int i = 0; i < (int)masters.size(); i++) {
        int len = (int)strlen(masters[i].name);

        if (pos + len + 2 >= bufSize) { break; }

        memcpy(buf + pos, masters[i].name, len);

        pos += len;
        buf[pos++] = '\0';
    }

    buf[pos] = '\0';
}

//Инициализация файлов программы
void initFiles()
{
    system("mkdir data");

    { ifstream c("data/repairs.txt"); if (!c.is_open()) { ofstream f("data/repairs.txt"); } }

    { ifstream c("data/masters.txt"); if (!c.is_open()) { ofstream f("data/masters.txt"); } }

    { ifstream c("data/finance.txt"); if (!c.is_open()) { ofstream f("data/finance.txt"); f << "0\n"; } }

    { ifstream c("data/log.txt");     if (!c.is_open()) { ofstream f("data/log.txt"); } }
}