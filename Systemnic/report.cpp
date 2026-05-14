#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <Windows.h>

//Макрос для кириллицы
#define U8(s) reinterpret_cast<const char*>(u8##s)

using namespace std;

//Структура заявки на ремонт
struct Repair
{
    char clientName[50];    //Имя клиента
    char deviceType[30];    //Тип устройства
    char masterName[30];    //Имя мастера
    char status[20];        //Статус заявки
    char comment[120];      //Комментарий к заявке
    char dateAdded[12];     //dd.mm.yyyy
    int  cost;              //Цена
    int  days;              //Срок
};

//Структура расхода
struct Expense
{
    char comment[100];  //Комментарий к расходу
    int  amount;        //Сумма
};

//Структура мастера
struct Master
{
    char  name[50];         //Имя мастера
    int   salaryType;       //0 = фиксированная, 1 = процент
    int   salaryFixed;      //Фиксированная зарплата
    float salaryPercent;    //Процент от дохода
};

extern vector<Repair>  repairs;
extern vector<Master>  masters;
extern vector<Expense> expenses;
extern float taxPercent;



/////////////////////////////////////////////////////////
//             КОНВЕРТАЦИЯ СТРОКИ В RTF                //
/////////////////////////////////////////////////////////

//Переводит UTF-8 строку в RTF Unicode-escapes (\uNNNN?)
string rtfStr(const char* input)
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



/////////////////////////////////////////////////////////
//          ЗАПИСЬ ОДНОЙ СТРОКИ ТАБЛИЦЫ RTF            //
/////////////////////////////////////////////////////////
void writeTableRow(ofstream& f, const string cols[], int colCount, bool isHeader)
{
    int cellX[8] = { 500, 2500, 4200, 5900, 7200, 9000, 10200, 11000 };

    f << "\\trowd\\trgaph108\\trleft-108\n";
    for (int i = 0; i < colCount; i++) {
        f << "\\clbrdrt\\brdrs\\brdrw10"
            << "\\clbrdrl\\brdrs\\brdrw10"
            << "\\clbrdrb\\brdrs\\brdrw10"
            << "\\clbrdrr\\brdrs\\brdrw10"
            << "\\cellx" << cellX[i] << " ";
    }
    f << "\n";

    for (int i = 0; i < colCount; i++) {
        if (isHeader)
            f << "\\pard\\intbl\\b " << cols[i] << "\\b0\\cell\n";
        else
            f << "\\pard\\intbl " << cols[i] << "\\cell\n";
    }
    f << "\\row\n";
}



/////////////////////////////////////////////////////////
//            ГЛАВНАЯ ФУНКЦИЯ ОТЧЁТА                   //
/////////////////////////////////////////////////////////
void generateReport()
{
    ofstream f("report.doc", ios::binary);
    if (!f.is_open()) return;

    //RTF-заголовок
    f << "{\\rtf1\\ansi\\uc1\\deff0\n";
    f << "{\\fonttbl"
        << "{\\f0\\froman\\fcharset0 Times New Roman;}"
        << "{\\f1\\fswiss\\fcharset0 Arial;}"
        << "}\n";
    f << "{\\colortbl;"
        << "\\red0\\green0\\blue0;"
        << "\\red0\\green0\\blue128;"
        << "\\red180\\green0\\blue0;"
        << "}\n";

    f << "\\f1\\fs24\n";
    f << "\\margl1440\\margr1440\\margt1440\\margb1440\n";

    //Заголовок документа
    f << "\\pard\\qc\\cf2\\b\\fs32 "
        << rtfStr(U8("Компьютерная мастерская"))
        << "\\par\n";
    f << "\\pard\\qc\\b\\fs26 "
        << rtfStr(U8("Финансовый отчёт"))
        << "\\b0\\fs24\\par\\par\n";

    //Дата
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    char dateBuf[30];
    sprintf_s(dateBuf, 30, "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    f << "\\pard\\qr "
        << rtfStr(U8("Дата: "))
        << dateBuf << "\\par\\par\n";



    /////////////////////////////////////////////////////////
    //                  ТАБЛИЦА ЗАЯВОК                     //
    /////////////////////////////////////////////////////////
    f << "\\pard\\b "
        << rtfStr(U8("Заявки на ремонт:"))
        << "\\b0\\par\n";

    string header[8] = {
        rtfStr(U8("№")),
        rtfStr(U8("Клиент")),
        rtfStr(U8("Устройство")),
        rtfStr(U8("Мастер")),
        rtfStr(U8("Статус")),
        rtfStr(U8("Комментарий")),
        rtfStr(U8("Стоимость (руб.)")),
        rtfStr(U8("Срок (дн.)")),
    };
    writeTableRow(f, header, 8, true);

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
        writeTableRow(f, row, 8, false);
        totalIncome += repairs[i].cost;
    }

    f << "\\pard\\par\n";



    /////////////////////////////////////////////////////////
    //                ФИНАНСОВЫЙ РАЗДЕЛ                    //
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

    //Доходы
    f << "\\pard\\b\\cf2 "
        << rtfStr(U8("ДОХОДЫ"))
        << "\\b0\\cf1\\par\n";
    char incBuf[60]; sprintf_s(incBuf, 60, "%d", totalIncome);
    f << "\\pard "
        << rtfStr(U8("Доход из заявок: "))
        << incBuf
        << rtfStr(U8(" руб."))
        << "\\par\n";

    //Расходы
    f << "\\pard\\par\\b\\cf3 "
        << rtfStr(U8("РАСХОДЫ"))
        << "\\b0\\cf1\\par\n";
    for (int i = 0; i < (int)expenses.size(); i++) {
        char amtBuf[20]; sprintf_s(amtBuf, 20, "%d", expenses[i].amount);
        f << "\\pard   "
            << rtfStr(expenses[i].comment)
            << ":  " << amtBuf
            << rtfStr(U8(" руб."))
            << "\\par\n";
    }
    char expBuf[60]; sprintf_s(expBuf, 60, "%d", totalExpense);
    f << "\\pard\\b "
        << rtfStr(U8("Итого расходов: "))
        << "\\b0 " << expBuf
        << rtfStr(U8(" руб."))
        << "\\par\n";

    //Зарплаты мастеров
    f << "\\pard\\par\\b "
        << rtfStr(U8("ЗАРПЛАТЫ МАСТЕРОВ"))
        << "\\b0\\par\n";
    for (int i = 0; i < (int)masters.size(); i++) {
        int sal = (masters[i].salaryType == 0)
            ? masters[i].salaryFixed
            : (int)((float)totalIncome * masters[i].salaryPercent / 100.0f);
        char salBuf[20]; sprintf_s(salBuf, 20, "%d", sal);
        f << "\\pard   "
            << rtfStr(masters[i].name)
            << ":  " << salBuf
            << rtfStr(U8(" руб."))
            << "\\par\n";
    }
    char salTBuf[60]; sprintf_s(salTBuf, 60, "%d", totalSalary);
    f << "\\pard\\b "
        << rtfStr(U8("Итого зарплаты: "))
        << "\\b0 " << salTBuf
        << rtfStr(U8(" руб."))
        << "\\par\n";

    //Налог
    char taxPBuf[20]; sprintf_s(taxPBuf, 20, "%.1f", taxPercent);
    char taxABuf[20]; sprintf_s(taxABuf, 20, "%d", taxAmount);
    f << "\\pard\\par\\b "
        << rtfStr(U8("НАЛОГ"))
        << "\\b0\\par\n";
    f << "\\pard   "
        << rtfStr(U8("Ставка: "))
        << taxPBuf << "%\\par\n";
    f << "\\pard   "
        << rtfStr(U8("Сумма налога: "))
        << taxABuf
        << rtfStr(U8(" руб."))
        << "\\par\n";

    //Итог
    char profBuf[20]; sprintf_s(profBuf, 20, "%d", netProfit);
    f << "\\pard\\par\\b\\fs28 "
        << rtfStr(U8("ЧИСТАЯ ПРИБЫЛЬ: "))
        << profBuf
        << rtfStr(U8(" руб."))
        << "\\b0\\fs24\\par\n";

    //Подпись
    f << "\\pard\\par\\par "
        << rtfStr(U8("Ответственный: "))
        << "________________________________\\par\n";

    f << "}\n";
    f.close();
}