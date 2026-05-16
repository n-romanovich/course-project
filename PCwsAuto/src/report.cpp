#include <fstream>
#include <string>
#include <cstdio>
#include <ctime>
#include <Windows.h>

#include "include/types.h"
#include "include/data.h"
#include "include/report.h"

using namespace std;

#define U8(s) reinterpret_cast<const char*>(u8##s)

//Экранированные строки для RTF
static string rtfStr(const char* input)
{
    if (input == nullptr || input[0] == '\0') return "";

    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, NULL, 0);

    wstring ws;
    if (wlen > 0) {
        ws.resize(wlen, 0);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, &ws[0], wlen);
    }
    else {
        wlen = MultiByteToWideChar(1251, 0, input, -1, NULL, 0);

        if (wlen <= 0) return "";
        ws.resize(wlen, 0);
        MultiByteToWideChar(1251, 0, input, -1, &ws[0], wlen);
    }

    string out;
    for (int i = 0; i < (int)ws.size(); i++) {
        wchar_t c = ws[i];
        if (c == 0) break;

        if (c == L'\\') { out += "\\\\"; }
        else if (c == L'{') { out += "\\{"; }
        else if (c == L'}') { out += "\\}"; }
        else if (c < 128) { out += (char)c; }
        else {
            char buf[24];
            int n = (int)(short)(unsigned short)c;
            sprintf_s(buf, 24, "\\u%d?", n);
            out += buf;
        }
    }
    return out;
}

//Запись строки таблицы заявок в RTF
static void writeRepairRow(ofstream& f, const string cols[], bool isHeader)
{
    int cellX[8] = { 505, 2122, 3681, 5002, 6084, 7584, 8376, 9248 };

    f << "\\trowd\\trgaph108\\trleft0\n";
    for (int i = 0; i < 8; i++) {
        f << "\\clbrdrt\\brdrs\\brdrw10"
            << "\\clbrdrl\\brdrs\\brdrw10"
            << "\\clbrdrb\\brdrs\\brdrw10"
            << "\\clbrdrr\\brdrs\\brdrw10"
            << "\\cellx" << cellX[i] << " ";
    }
    f << "\n";

    for (int i = 0; i < 8; i++) {
        if (isHeader)
            f << "\\pard\\intbl\\f0\\fs28\\b " << cols[i] << "\\b0\\cell\n";
        else
            f << "\\pard\\intbl\\f0\\fs28 " << cols[i] << "\\cell\n";
    }
    f << "\\row\n";
}

//Запись строки двухколоночной финансовой таблицы в RTF
static void writeFinRow(ofstream& f, const string& label, const string& value, int w1, int w2)
{
    f << "\\trowd\\trgaph108\\trleft0\n";
    f << "\\clbrdrt\\brdrs\\brdrw10"
        << "\\clbrdrl\\brdrs\\brdrw10"
        << "\\clbrdrb\\brdrs\\brdrw10"
        << "\\clbrdrr\\brdrs\\brdrw10"
        << "\\cellx" << w1 << " ";
    f << "\\clbrdrt\\brdrs\\brdrw10"
        << "\\clbrdrl\\brdrs\\brdrw10"
        << "\\clbrdrb\\brdrs\\brdrw10"
        << "\\clbrdrr\\brdrs\\brdrw10"
        << "\\cellx" << (w1 + w2) << "\n";

    f << "\\pard\\intbl\\f0\\fs28 " << label << "\\cell\n";
    f << "\\pard\\intbl\\f0\\fs28 " << value << "\\cell\n";
    f << "\\row\n";
}

//Генерация отчета Word
void generateReport()
{
    ofstream f("report.doc", ios::binary);
    if (!f.is_open()) return;

    f << "{\\rtf1\\ansi\\uc1\\deff0\n";
    f << "{\\fonttbl"
        << "{\\f0\\froman\\fcharset0 Times New Roman;}"
        << "}\n";

    f << "\\margl1701\\margr850\\margt1134\\margb1134\n";

    f << "\\f0\\fs28\n";

    /////////////////////////////////////////////////////////
    //                    ЗАГОЛОВОК                        //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc\\fs36 "
        << rtfStr(U8("ФИНАНСОВЫЙ ОТЧЕТ"))
        << "\\fs28\\par\\par\n";

    /////////////////////////////////////////////////////////
    //                  ТАБЛИЦА ЗАЯВОК                     //
    /////////////////////////////////////////////////////////
    f << "\\pard "
        << rtfStr(U8("Заявки на ремонт:"))
        << "\\par\n";

    string header[8] = {
        rtfStr(U8("№")),
        rtfStr(U8("Клиент")),
        rtfStr(U8("Устройство")),
        rtfStr(U8("Мастер")),
        rtfStr(U8("Статус")),
        rtfStr(U8("Комм-й")),
        rtfStr(U8("BYN")),
        rtfStr(U8("Срок (дн.)")),
    };
    writeRepairRow(f, header, true);

    int totalIncome = 0;
    for (int i = 0; i < (int)repairs.size(); i++) {
        char numBuf[10];  sprintf_s(numBuf, 10, "%d", i + 1);
        char costBuf[20]; sprintf_s(costBuf, 20, "%d", repairs[i].cost);
        char daysBuf[10]; sprintf_s(daysBuf, 10, "%d", repairs[i].days);

        string row[8] = {
            string(numBuf),
            rtfStr(repairs[i].clientName),
            rtfStr(repairs[i].deviceType),
            rtfStr(repairs[i].masterName),
            rtfStr(repairs[i].status),
            rtfStr(repairs[i].comment),
            string(costBuf),
            string(daysBuf),
        };
        writeRepairRow(f, row, false);
        totalIncome += repairs[i].cost;
    }

    f << "\\pard\\par\\par\n";

    /////////////////////////////////////////////////////////
    //                  ФИНАНСОВЫЙ РАЗДЕЛ                  //
    /////////////////////////////////////////////////////////
    int totalExpense = 0;
    for (int i = 0; i < (int)expenses.size(); i++)
        totalExpense += expenses[i].amount;

    int totalSalary = 0;
    for (int i = 0; i < (int)masters.size(); i++) {
        if (masters[i].salaryType == 0)
            totalSalary += masters[i].salaryFixed;
        else
            totalSalary += (int)((float)totalIncome * masters[i].salaryPercent / 100.0f);
    }

    int profit = totalIncome - totalExpense - totalSalary;
    int taxAmount = (int)((float)profit * taxPercent / 100.0f);
    int netProfit = profit - taxAmount;

    char incBuf[20];  sprintf_s(incBuf, 20, "%d", totalIncome);
    char expBuf[20];  sprintf_s(expBuf, 20, "%d", totalExpense);
    char salBuf[20];  sprintf_s(salBuf, 20, "%d", totalSalary);
    char taxPBuf[20]; sprintf_s(taxPBuf, 20, "%.1f", taxPercent);
    char taxABuf[20]; sprintf_s(taxABuf, 20, "%d", taxAmount);
    char netBuf[20];  sprintf_s(netBuf, 20, "%d", netProfit);

    //Ширины двухколоночных таблиц (из оригинала)
    int wideL = 1696, wideR = 7649;   //доходы и расходы
    int halfL = 4672, halfR = 4673;   //зарплаты, налог, итого

    /////////////////////////////////////////////////////////
    //                      ДОХОДЫ                         //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc "
        << rtfStr(U8("ДОХОДЫ:"))
        << "\\par\n";

    writeFinRow(f, rtfStr(U8("Заявки")), string(incBuf) + " BYN", wideL, wideR);

    f << "\\pard\\par\\par\n";

    /////////////////////////////////////////////////////////
    //                     РАСХОДЫ                         //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc "
        << rtfStr(U8("РАСХОДЫ:"))
        << "\\par\n";

    for (int i = 0; i < (int)expenses.size(); i++) {
        char amtBuf[20]; sprintf_s(amtBuf, 20, "%d", expenses[i].amount);
        writeFinRow(f, rtfStr(expenses[i].comment), string(amtBuf) + " BYN", wideL, wideR);
    }
    writeFinRow(f, rtfStr(U8("ИТОГО")), string(expBuf) + " BYN", wideL, wideR);

    f << "\\pard\\par\\par\n";

    /////////////////////////////////////////////////////////
    //                ЗАРПЛАТА МАСТЕРОВ                   //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc "
        << rtfStr(U8("ЗАРПЛАТА МАСТЕРОВ:"))
        << "\\par\n";

    for (int i = 0; i < (int)masters.size(); i++) {
        int sal = (masters[i].salaryType == 0)
            ? masters[i].salaryFixed
            : (int)((float)totalIncome * masters[i].salaryPercent / 100.0f);
        char saliBuf[20]; sprintf_s(saliBuf, 20, "%d", sal);

        f << "\\pard\\tx1440 "
            << rtfStr(masters[i].name) << ":"
            << "\\tab " << saliBuf << " BYN"
            << "\\par\n";
    }

    writeFinRow(f, rtfStr(U8("ИТОГО")), string(salBuf) + " BYN", halfL, halfR);

    f << "\\pard\\par\n";

    /////////////////////////////////////////////////////////
    //                       НАЛОГ                         //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc "
        << rtfStr(U8("НАЛОГ"))
        << "\\par\n";

    writeFinRow(f, rtfStr(U8("Ставка")), string(taxPBuf) + "%", halfL, halfR);
    writeFinRow(f, rtfStr(U8("ИТОГО")), string(taxABuf) + " BYN", halfL, halfR);

    f << "\\pard\\par\n";

    /////////////////////////////////////////////////////////
    //                       ИТОГО                         //
    /////////////////////////////////////////////////////////
    f << "\\pard\\qc "
        << rtfStr(U8("ИТОГО"))
        << "\\par\n";

    writeFinRow(f, rtfStr(U8("ПРИБЫЛЬ")), string(netBuf) + " BYN", halfL, halfR);

    f << "\\pard\\par\\par\n";

    /////////////////////////////////////////////////////////
    //                  ДАТА И ПОДПИСЬ                     //
    /////////////////////////////////////////////////////////
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    char dateBuf[30];
    sprintf_s(dateBuf, 30, "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);

    f << "\\pard\\qr "
        << rtfStr(U8("Дата: "))
        << dateBuf << "\\par\n";

    f << "\\pard "
        << rtfStr(U8("Ответственный: "))
        << "___________________\\par\n";

    f << "}\n";
    f.close();
}