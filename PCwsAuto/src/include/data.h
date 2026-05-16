#pragma once
#include <vector>
#include "types.h"

extern std::vector<Repair>  repairs;
extern std::vector<Master>  masters;
extern std::vector<Expense> expenses;
extern float taxPercent;

void loadRepairs();
void saveRepairs();
void loadMasters();
void saveMasters();
void loadFinance();
void saveFinance();