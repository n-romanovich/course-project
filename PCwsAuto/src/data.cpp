#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>

#include "include/data.h"

using namespace std;

//Загрузка и сохранение данных о мастерах
void loadMasters()
{
    masters.clear();
    ifstream f("data/masters.txt");

    if (!f.is_open()) return;

    string s; Master m; int field = 0;

    while (getline(f, s)) {
        if (s.empty()) { if (field == 4) { masters.push_back(m); field = 0; } continue; }

        switch (field) {
        case 0: strncpy_s(m.name, 50, s.c_str(), _TRUNCATE); break;
        case 1: m.salaryType = atoi(s.c_str()); break;
        case 2: m.salaryFixed = atoi(s.c_str()); break;
        case 3: m.salaryPercent = (float)atof(s.c_str()); break;
        }
        field++;
    }

    if (field == 4) masters.push_back(m);
    f.close();
}

//Сохранение данных о мастерах
void saveMasters()
{
    ofstream f("data/masters.txt");

    for (int i = 0; i < (int)masters.size(); i++)
        f << masters[i].name << "\n" << masters[i].salaryType << "\n"
        << masters[i].salaryFixed << "\n" << masters[i].salaryPercent << "\n\n";

    f.close();
}

//Загрузка и сохранение данных о финансах
void loadFinance()
{
    expenses.clear(); taxPercent = 0.0f;
    ifstream f("data/finance.txt");

    if (!f.is_open()) return;

    string s;

    if (getline(f, s)) taxPercent = (float)atof(s.c_str());

    Expense e; int field = 0;

    while (getline(f, s)) {
        if (s.empty()) { if (field == 2) { expenses.push_back(e); field = 0; } continue; }

        switch (field) {
        case 0: strncpy_s(e.comment, 100, s.c_str(), _TRUNCATE); break;
        case 1: e.amount = atoi(s.c_str()); break;
        }
        field++;
    }

    if (field == 2) expenses.push_back(e);
    f.close();
}

//Сохранение данных о финансах
void saveFinance()
{
    ofstream f("data/finance.txt");

    f << taxPercent << "\n";

    for (int i = 0; i < (int)expenses.size(); i++)
        f << expenses[i].comment << "\n" << expenses[i].amount << "\n\n";

    f.close();
}

//Загрузка и сохранение данных о заявках на ремонт
void loadRepairs()
{
    repairs.clear();
    ifstream f("data/repairs.txt");

    if (!f.is_open()) return;

    string s; Repair r; int field = 0;

    while (getline(f, s)) {
        if (s.empty()) { if (field == 8) { repairs.push_back(r); field = 0; } continue; }

        switch (field) {
        case 0: strncpy_s(r.clientName, 50, s.c_str(), _TRUNCATE); break;
        case 1: strncpy_s(r.deviceType, 30, s.c_str(), _TRUNCATE); break;
        case 2: strncpy_s(r.masterName, 30, s.c_str(), _TRUNCATE); break;
        case 3: strncpy_s(r.status, 20, s.c_str(), _TRUNCATE); break;
        case 4: strncpy_s(r.comment, 120, s.c_str(), _TRUNCATE); break;
        case 5: strncpy_s(r.dateAdded, 12, s.c_str(), _TRUNCATE); break;
        case 6: r.cost = atoi(s.c_str()); break;
        case 7: r.days = atoi(s.c_str()); break;
        }
        field++;
    }

    if (field == 8) repairs.push_back(r);
    f.close();
}

//Сохранение данных о заявках на ремонт
void saveRepairs()
{
    ofstream f("data/repairs.txt");

    for (int i = 0; i < (int)repairs.size(); i++)
        f << repairs[i].clientName << "\n" << repairs[i].deviceType << "\n"
        << repairs[i].masterName << "\n" << repairs[i].status << "\n"
        << repairs[i].comment << "\n" << repairs[i].dateAdded << "\n"
        << repairs[i].cost << "\n" << repairs[i].days << "\n\n";

    f.close();
}