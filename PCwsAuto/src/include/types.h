#pragma once

#define U8(s) reinterpret_cast<const char*>(u8##s)

//Структура заявки на ремонт
struct Repair
{
    char clientName[50];    //Имя клиенте
	char deviceType[30];    //Тип устройства
	char masterName[30];    //Имя мастера
    char status[20];        //Статус заявки
    char comment[120];      //Комментарий
    char dateAdded[12];     //Дата добавления
    int  cost;              //Стоимость
    int  days;              //Срок выполнения
};

//Структура расходов
struct Expense
{
    char comment[100];      //Комментарий
    int  amount;            //Сумма
};

//Структура мастера
struct Master
{
    char  name[50];         //Имя мастера
    int   salaryType;       //Тип зарплаты
    int   salaryFixed;      //Фиксированная зарплата
    float salaryPercent;    //Процент от дохода
};