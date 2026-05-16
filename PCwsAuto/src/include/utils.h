#pragma once
#include <string>
#include <ctime>
#include "types.h"

extern std::string logContent;

void writeLog(const char* action, const char* detail);
void loadLog();
void getCurrentDate(char* buf, int bufSize);
time_t parseDate(const char* s);
bool isOverdue(const Repair& r);
void bubbleSort();
void initFiles();
void buildMastersComboStr(char* buf, int bufSize);