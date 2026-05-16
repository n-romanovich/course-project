#include "imgui.h"
#include "imgui-SFML.h"

#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <string>

#include "include/types.h"
#include "include/data.h"
#include "include/utils.h"
#include "include/report.h"

using namespace std;

bool windowRecords = false;
bool windowAdd = false;
bool windowReport = false;
bool windowEditRepair = false;
bool windowManageMasters = false;
bool windowEditMaster = false;
bool windowLog = false;
bool windowAbout = false;
bool gQuit = false;

bool showMessage = false;
char messageText[256] = "";

char inputName[50] = "";
char inputDevice[30] = "";
char inputComment[120] = "";
int  inputCost = 0;
int  inputDays = 0;
int  inputMasterIdx = 0;
int  addStatusIdx = 0;

char searchNameBuf[50] = "";
int  filterStatusIdx = 0;

int  editRepairIdx = -1;
char editName[50] = "";
char editDevice[30] = "";
char editMaster[30] = "";
char editStatus[20] = "";
char editComment[120] = "";
int  editCost = 0;
int  editDays = 0;
int  editStatusIdx = 0;
int  editMasterIdx = 0;

int   editMasterListIdx = -1;
char  editMasterName[50] = "";
int   editMasterSalType = 0;
int   editMasterSalFixed = 0;
float editMasterSalPercent = 0.0f;

char  newMasterName[50] = "";
int   newMasterSalType = 0;
int   newMasterSalFixed = 0;
float newMasterSalPercent = 0.0f;

char expenseComment[100] = "";
int  expenseAmount = 0;

int pendingDeleteIdx = -1;

static const char* STATUS_LABELS[3] = {
    reinterpret_cast<const char*>(u8"принят"),
    reinterpret_cast<const char*>(u8"в работе"),
    reinterpret_cast<const char*>(u8"готов")
};

static float W = 1280.0f;
static float H = 720.0f;
static float TH = 50.0f;

static ImVec4 statusColor(const Repair& r)
{
    if (isOverdue(r))
        return ImVec4(1.0f, 0.25f, 0.25f, 1.0f);

    const char* s = r.status;

    if (strcmp(s, reinterpret_cast<const char*>(u8"принят")) == 0) return ImVec4(0.95f, 0.80f, 0.2f, 1.0f);
    if (strcmp(s, reinterpret_cast<const char*>(u8"в работе")) == 0) return ImVec4(0.3f, 0.75f, 1.0f, 1.0f);
    if (strcmp(s, reinterpret_cast<const char*>(u8"готов")) == 0) return ImVec4(0.35f, 1.0f, 0.45f, 1.0f);

    return ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
}



/////////////////////////////////////////////////////////
//              ПОПАП: СООБЩЕНИЕ                       //
/////////////////////////////////////////////////////////
static void showPopupMessage()
{
    if (showMessage) ImGui::OpenPopup(U8("Сообщение"));

    if (ImGui::BeginPopupModal(U8("Сообщение"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Spacing();
        ImGui::Text("%s", messageText);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        if (ImGui::Button("   OK   ")) { showMessage = false; ImGui::CloseCurrentPopup(); }

        ImGui::EndPopup();
    }
}



/////////////////////////////////////////////////////////
//                  ОКНО: ЗАПИСИ                       //
/////////////////////////////////////////////////////////
static void showWindowRecords()
{
    if (!windowRecords) return;

    ImGui::SetNextWindowPos(ImVec2(20.0f, TH + 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(W - 40.0f, H - TH - 20.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Записи"), &windowRecords);

    ImGui::Text(U8("Поиск:"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##srch", searchNameBuf, 50);
    ImGui::SameLine();
    ImGui::Text(U8("  Статус:"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);

    static const char* filterLabels[5] = {
        reinterpret_cast<const char*>(u8"все"),
        reinterpret_cast<const char*>(u8"принят"),
        reinterpret_cast<const char*>(u8"в работе"),
        reinterpret_cast<const char*>(u8"готов"),
        reinterpret_cast<const char*>(u8"просрочен")
    };
    ImGui::Combo("##flt", &filterStatusIdx, filterLabels, 5);
    ImGui::SameLine();
    ImGui::Text(U8("  Всего: %d"), (int)repairs.size());
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGuiTableFlags tflags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    float tableH = ImGui::GetContentRegionAvail().y - 8.0f;
    int   deleteIdx = -1;

    if (ImGui::BeginTable("##tblRec", 9, tflags, ImVec2(0.0f, tableH))) {

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("N", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn(U8("Клиент"), ImGuiTableColumnFlags_WidthFixed, 155.0f);
        ImGui::TableSetupColumn(U8("Устройство"), ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn(U8("Мастер"), ImGuiTableColumnFlags_WidthFixed, 115.0f);
        ImGui::TableSetupColumn(U8("Статус"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(U8("Комментарий"), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(U8("BYN"), ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn(U8("Дней"), ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 26.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)repairs.size(); i++) {
            bool overdue = isOverdue(repairs[i]);

            if (filterStatusIdx == 1 && strcmp(repairs[i].status, reinterpret_cast<const char*>(u8"принят")) != 0) continue;
            if (filterStatusIdx == 2 && strcmp(repairs[i].status, reinterpret_cast<const char*>(u8"в работе")) != 0) continue;
            if (filterStatusIdx == 3 && strcmp(repairs[i].status, reinterpret_cast<const char*>(u8"готов")) != 0) continue;
            if (filterStatusIdx == 4 && !overdue) continue;

            if (searchNameBuf[0] != '\0' && strstr(repairs[i].clientName, searchNameBuf) == NULL) continue;

            ImVec4 col = statusColor(repairs[i]);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            char selId[24]; sprintf_s(selId, 24, "##sel%d", i);
            bool clicked = ImGui::Selectable(selId, false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                ImVec2(0.0f, 0.0f));
            if (clicked) {
                editRepairIdx = i;
                strncpy_s(editName, 50, repairs[i].clientName, _TRUNCATE);
                strncpy_s(editDevice, 30, repairs[i].deviceType, _TRUNCATE);
                strncpy_s(editMaster, 30, repairs[i].masterName, _TRUNCATE);
                strncpy_s(editStatus, 20, repairs[i].status, _TRUNCATE);
                strncpy_s(editComment, 120, repairs[i].comment, _TRUNCATE);
                editCost = repairs[i].cost;
                editDays = repairs[i].days;

                editStatusIdx = 0;

                for (int s = 0; s < 3; s++)
                    if (strcmp(editStatus, STATUS_LABELS[s]) == 0) { editStatusIdx = s; break; }

                editMasterIdx = 0;

                for (int mi = 0; mi < (int)masters.size(); mi++)
                    if (strcmp(masters[mi].name, repairs[i].masterName) == 0) { editMasterIdx = mi; break; }

                windowEditRepair = true;
            }
            ImGui::SameLine();
            ImGui::TextColored(col, "%d", i + 1);

            ImGui::TableSetColumnIndex(1); ImGui::TextColored(col, "%s", repairs[i].clientName);
            ImGui::TableSetColumnIndex(2); ImGui::TextColored(col, "%s", repairs[i].deviceType);
            ImGui::TableSetColumnIndex(3); ImGui::TextColored(col, "%s", repairs[i].masterName);
            ImGui::TableSetColumnIndex(4);

            if (overdue) ImGui::TextColored(col, U8("ПРОСРОЧЕН"));
            else         ImGui::TextColored(col, "%s", repairs[i].status);

            ImGui::TableSetColumnIndex(5); ImGui::TextColored(col, "%s", repairs[i].comment);
            ImGui::TableSetColumnIndex(6); ImGui::TextColored(col, "%d", repairs[i].cost);
            ImGui::TableSetColumnIndex(7); ImGui::TextColored(col, "%d", repairs[i].days);

            ImGui::TableSetColumnIndex(8);
            char xId[20]; sprintf_s(xId, 20, "X##x%d", i);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));

            if (ImGui::SmallButton(xId)) { deleteIdx = i; windowEditRepair = false; }
            ImGui::PopStyleColor(2);
        }
        ImGui::EndTable();
    }

    if (deleteIdx >= 0) { pendingDeleteIdx = deleteIdx; ImGui::OpenPopup(U8("Подтвердите удаление")); }

    if (ImGui::BeginPopupModal(U8("Подтвердите удаление"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Spacing();
        ImGui::Text(U8("Удалить заявку?"));

        if (pendingDeleteIdx >= 0 && pendingDeleteIdx < (int)repairs.size()) {
            ImGui::Text(U8("Клиент: %s"), repairs[pendingDeleteIdx].clientName);
            ImGui::Text(U8("Устройство: %s"), repairs[pendingDeleteIdx].deviceType);
        }
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));

        if (ImGui::Button(U8("Удалить"), ImVec2(100.0f, 0.0f))) {

            if (pendingDeleteIdx >= 0 && pendingDeleteIdx < (int)repairs.size()) {
                char logBuf[80]; sprintf_s(logBuf, 80, "%s", repairs[pendingDeleteIdx].clientName);
                writeLog(U8("УДАЛЕНИЕ"), logBuf);
                repairs.erase(repairs.begin() + pendingDeleteIdx);
                saveRepairs();
                strcpy_s(messageText, U8("Запись удалена!"));
                showMessage = true;
            }
            pendingDeleteIdx = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (ImGui::Button(U8("Отмена"), ImVec2(90.0f, 0.0f))) { pendingDeleteIdx = -1; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    ImGui::End();
}



/////////////////////////////////////////////////////////
//           ОКНО: РЕДАКТИРОВАНИЕ ЗАЯВКИ               //
/////////////////////////////////////////////////////////
static void showWindowEditRepair()
{
    if (!windowEditRepair || editRepairIdx < 0 || editRepairIdx >= (int)repairs.size()) return;

    ImGui::SetNextWindowPos(ImVec2(220.0f, TH + 30.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(460.0f, 460.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Редактировать заявку"), &windowEditRepair);

    ImGui::Text(U8("Имя клиента:"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##eName", editName, 50);
    ImGui::Spacing();
    ImGui::Text(U8("Тип устройства:"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##eDev", editDevice, 30);
    ImGui::Spacing();
    ImGui::Text(U8("Мастер:"));
    ImGui::SetNextItemWidth(-1.0f);

    if (!masters.empty()) {
        static char masterComboBuf[2048];
        buildMastersComboStr(masterComboBuf, 2048);

        if (ImGui::Combo("##eMaster", &editMasterIdx, masterComboBuf))
            strncpy_s(editMaster, 30, masters[editMasterIdx].name, _TRUNCATE);
    }
    else ImGui::InputText("##eMasterTxt", editMaster, 30);

    ImGui::Spacing();
    ImGui::Text(U8("Статус:"));
    ImGui::SetNextItemWidth(-1.0f);

    if (ImGui::Combo("##eStatus", &editStatusIdx, STATUS_LABELS, 3))
        strncpy_s(editStatus, 20, STATUS_LABELS[editStatusIdx], _TRUNCATE);

    ImGui::Spacing();
    ImGui::Text(U8("Комментарий:"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##eComment", editComment, 120);
    ImGui::Spacing();
    ImGui::Text(U8("Стоимость (BYN):"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputInt("##eCost", &editCost);
    ImGui::Spacing();
    ImGui::Text(U8("Срок (дней):"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputInt("##eDays", &editDays);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button(U8("Сохранить"), ImVec2(140.0f, 0.0f))) {

        if (editName[0] == '\0') {
            strcpy_s(messageText, U8("Ошибка: имя клиента не может быть пустым!")); showMessage = true;
        }
        else {
            char logBuf[60]; sprintf_s(logBuf, 60, "%s", editName);
            writeLog(U8("ИЗМЕНЕНИЕ ЗАЯВКИ"), logBuf);
            strncpy_s(repairs[editRepairIdx].clientName, 50, editName, _TRUNCATE);
            strncpy_s(repairs[editRepairIdx].deviceType, 30, editDevice, _TRUNCATE);
            strncpy_s(repairs[editRepairIdx].masterName, 30,
                masters.empty() ? editMaster : masters[editMasterIdx].name, _TRUNCATE);
            strncpy_s(repairs[editRepairIdx].status, 20, STATUS_LABELS[editStatusIdx], _TRUNCATE);
            strncpy_s(repairs[editRepairIdx].comment, 120, editComment, _TRUNCATE);
            repairs[editRepairIdx].cost = editCost;
            repairs[editRepairIdx].days = editDays;
            bubbleSort(); saveRepairs();
            strcpy_s(messageText, U8("Заявка обновлена!"));
            showMessage = true; windowEditRepair = false;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(U8("Отмена"), ImVec2(100.0f, 0.0f))) windowEditRepair = false;

    ImGui::End();
}



/////////////////////////////////////////////////////////
//             ОКНО: ДОБАВИТЬ ЗАЯВКУ                   //
/////////////////////////////////////////////////////////
static void showWindowAdd()
{
    if (!windowAdd) return;

    ImGui::SetNextWindowPos(ImVec2(220.0f, TH + 30.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(460.0f, 460.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Добавить заявку"), &windowAdd);

    ImGui::Text(U8("Имя клиента: *"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##addName", inputName, 50);
    ImGui::Spacing();
    ImGui::Text(U8("Тип устройства: *"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##addDevice", inputDevice, 30);
    ImGui::Spacing();
    ImGui::Text(U8("Мастер: *"));
    ImGui::SetNextItemWidth(-1.0f);

    if (!masters.empty()) {
        static char masterComboBuf[2048];
        buildMastersComboStr(masterComboBuf, 2048);
        ImGui::Combo("##addMaster", &inputMasterIdx, masterComboBuf);
    }
    else ImGui::TextColored(ImVec4(1, 0.4f, 0.1f, 1), U8("Нет мастеров. Откройте 'Мастера'."));

    ImGui::Spacing();
    ImGui::Text(U8("Статус: *"));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##addStatus", &addStatusIdx, STATUS_LABELS, 3);
    ImGui::Spacing();
    ImGui::Text(U8("Комментарий (поломка и т.д.):"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##addComment", inputComment, 120);
    ImGui::Spacing();
    ImGui::Text(U8("Стоимость (BYN): *"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputInt("##addCost", &inputCost);
    ImGui::Spacing();
    ImGui::Text(U8("Срок выполнения (дней):"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputInt("##addDays", &inputDays);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), U8("* - обязательные поля"));
    ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button(U8("Сохранить"), ImVec2(140.0f, 0.0f))) {
        bool ok = true;
        if (inputName[0] == '\0') { strcpy_s(messageText, U8("Ошибка: заполните имя клиента!"));       showMessage = true; ok = false; }
        else if (inputDevice[0] == '\0') { strcpy_s(messageText, U8("Ошибка: заполните тип устройства!"));    showMessage = true; ok = false; }
        else if (masters.empty()) { strcpy_s(messageText, U8("Ошибка: сначала добавьте мастера!"));    showMessage = true; ok = false; }
        else if (inputCost <= 0) { strcpy_s(messageText, U8("Ошибка: стоимость должна быть > 0!"));   showMessage = true; ok = false; }

        if (ok) {
            Repair r;
            strncpy_s(r.clientName, 50, inputName, _TRUNCATE);
            strncpy_s(r.deviceType, 30, inputDevice, _TRUNCATE);
            strncpy_s(r.masterName, 30, masters[inputMasterIdx].name, _TRUNCATE);
            strncpy_s(r.status, 20, STATUS_LABELS[addStatusIdx], _TRUNCATE);
            strncpy_s(r.comment, 120, inputComment, _TRUNCATE);
            getCurrentDate(r.dateAdded, 12);
            r.cost = inputCost; r.days = inputDays;

            char logBuf[60]; sprintf_s(logBuf, 60, "%s", r.clientName);
            writeLog(U8("ДОБАВЛЕНИЕ ЗАЯВКИ"), logBuf);
            repairs.push_back(r);
            bubbleSort(); saveRepairs();
            strcpy_s(messageText, U8("Запись успешно добавлена!"));
            showMessage = true; windowAdd = false;
            inputName[0] = inputDevice[0] = inputComment[0] = '\0';
            inputCost = 0; inputDays = 0; inputMasterIdx = 0; addStatusIdx = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(U8("Отмена"), ImVec2(100.0f, 0.0f))) windowAdd = false;

    ImGui::End();
}



/////////////////////////////////////////////////////////
//                  ОКНО: МАСТЕРА                      //
/////////////////////////////////////////////////////////
static void showWindowManageMasters()
{
    if (!windowManageMasters) return;

    ImGui::SetNextWindowPos(ImVec2(100.0f, TH + 30.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(660.0f, 530.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Мастера"), &windowManageMasters);

    if (masters.empty()) {
        ImGui::Text(U8("Мастеров нет."));
    }
    else {
        ImGuiTableFlags mflags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("##tblM", 6, mflags)) {
            ImGui::TableSetupColumn(U8("Имя"), ImGuiTableColumnFlags_WidthFixed, 155.0f);
            ImGui::TableSetupColumn(U8("Тип ЗП"), ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn(U8("Фикс. (BYN)"), ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn(U8("% дохода"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn(U8("Изменить"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn(U8("Удалить"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            int delIdx = -1;
            for (int i = 0; i < (int)masters.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s", masters[i].name);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", masters[i].salaryType == 0
                    ? reinterpret_cast<const char*>(u8"Фиксированная")
                    : reinterpret_cast<const char*>(u8"% от дохода"));
                ImGui::TableSetColumnIndex(2);

                if (masters[i].salaryType == 0) ImGui::Text("%d", masters[i].salaryFixed);
                else ImGui::Text("-");
                ImGui::TableSetColumnIndex(3);

                if (masters[i].salaryType == 1) ImGui::Text("%.1f%%", masters[i].salaryPercent);
                else ImGui::Text("-");

                ImGui::TableSetColumnIndex(4);
                char editId[24]; sprintf_s(editId, 24, "Ed.##m%d", i);

                if (ImGui::SmallButton(editId)) {
                    editMasterListIdx = i;
                    strncpy_s(editMasterName, 50, masters[i].name, _TRUNCATE);
                    editMasterSalType = masters[i].salaryType;
                    editMasterSalFixed = masters[i].salaryFixed;
                    editMasterSalPercent = masters[i].salaryPercent;
                    windowEditMaster = true;
                }
                ImGui::TableSetColumnIndex(5);
                char delId[32]; sprintf_s(delId, 32, U8("Удал##dm%d"), i);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));
                if (ImGui::SmallButton(delId)) delIdx = i;
                ImGui::PopStyleColor(2);
            }

            if (delIdx >= 0) {
                char logBuf[60]; sprintf_s(logBuf, 60, "%s", masters[delIdx].name);
                writeLog(U8("УДАЛЕНИЕ МАСТЕРА"), logBuf);
                masters.erase(masters.begin() + delIdx);
                saveMasters();
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing(); ImGui::Separator();
    ImGui::Text(U8("Добавить мастера:"));
    ImGui::Spacing();
    ImGui::Text(U8("Имя:"));
    ImGui::SetNextItemWidth(200.0f); ImGui::InputText("##newMN", newMasterName, 50);
    ImGui::Spacing();
    ImGui::Text(U8("Тип зарплаты:"));
    ImGui::RadioButton(U8("Фиксированная"), &newMasterSalType, 0); ImGui::SameLine();
    ImGui::RadioButton(U8("% от дохода"), &newMasterSalType, 1);
    ImGui::Spacing();

    if (newMasterSalType == 0) {
        ImGui::Text(U8("Сумма (BYN):"));
        ImGui::SetNextItemWidth(150.0f); ImGui::InputInt("##newMSF", &newMasterSalFixed);
    }
    else {
        ImGui::Text(U8("Процент (%):"));
        ImGui::SetNextItemWidth(150.0f); ImGui::InputFloat("##newMSP", &newMasterSalPercent, 0.5f, 1.0f, "%.1f");
    }
    ImGui::Spacing();

    if (ImGui::Button(U8("Добавить мастера"), ImVec2(180.0f, 0.0f))) {

        if (newMasterName[0] == '\0') {
            strcpy_s(messageText, U8("Введите имя мастера!")); showMessage = true;
        }
        else {
            Master m;
            strncpy_s(m.name, 50, newMasterName, _TRUNCATE);
            m.salaryType = newMasterSalType; m.salaryFixed = newMasterSalFixed; m.salaryPercent = newMasterSalPercent;
            masters.push_back(m); saveMasters();
            char logBuf[60]; sprintf_s(logBuf, 60, "%s", m.name);
            writeLog(U8("ДОБАВЛЕНИЕ МАСТЕРА"), logBuf);
            newMasterName[0] = '\0'; newMasterSalFixed = 0; newMasterSalPercent = 0.0f; newMasterSalType = 0;
        }
    }
    ImGui::End();
}



/////////////////////////////////////////////////////////
//           ОКНО: РЕДАКТИРОВАТЬ МАСТЕРА               //
/////////////////////////////////////////////////////////
static void showWindowEditMaster()
{
    if (!windowEditMaster || editMasterListIdx < 0 || editMasterListIdx >= (int)masters.size()) return;

    ImGui::SetNextWindowPos(ImVec2(310.0f, TH + 90.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 270.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Редактировать мастера"), &windowEditMaster);

    ImGui::Text(U8("Имя:"));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##emN", editMasterName, 50);
    ImGui::Spacing();
    ImGui::Text(U8("Тип зарплаты:"));
    ImGui::RadioButton(U8("Фикс##em"), &editMasterSalType, 0); ImGui::SameLine();
    ImGui::RadioButton(U8("% дохода##em"), &editMasterSalType, 1);
    ImGui::Spacing();

    if (editMasterSalType == 0) {
        ImGui::Text(U8("Сумма (BYN):"));
        ImGui::SetNextItemWidth(-1.0f); ImGui::InputInt("##emSF", &editMasterSalFixed);
    }
    else {
        ImGui::Text(U8("Процент (%):"));
        ImGui::SetNextItemWidth(-1.0f); ImGui::InputFloat("##emSP", &editMasterSalPercent, 0.5f, 1.0f, "%.1f");
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button(U8("Сохранить"), ImVec2(130.0f, 0.0f))) {

        if (editMasterName[0] == '\0') {
            strcpy_s(messageText, U8("Имя не может быть пустым!")); showMessage = true;
        }
        else {
            char logBuf[60]; sprintf_s(logBuf, 60, "%s", editMasterName);
            writeLog(U8("ИЗМЕНЕНИЕ МАСТЕРА"), logBuf);
            strncpy_s(masters[editMasterListIdx].name, 50, editMasterName, _TRUNCATE);
            masters[editMasterListIdx].salaryType = editMasterSalType;
            masters[editMasterListIdx].salaryFixed = editMasterSalFixed;
            masters[editMasterListIdx].salaryPercent = editMasterSalPercent;
            saveMasters(); windowEditMaster = false;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(U8("Отмена"), ImVec2(90.0f, 0.0f))) windowEditMaster = false;

    ImGui::End();
}



/////////////////////////////////////////////////////////
//              ОКНО: ОТЧЁТ И ФИНАНСЫ                  //
/////////////////////////////////////////////////////////
static void showWindowReport()
{
    if (!windowReport) return;

    ImGui::SetNextWindowPos(ImVec2(80.0f, TH + 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(680.0f, 650.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Отчет и финансы"), &windowReport);

    int totalIncome = 0;
    for (int i = 0; i < (int)repairs.size(); i++)  totalIncome += repairs[i].cost;

    int totalExpense = 0;
    for (int i = 0; i < (int)expenses.size(); i++) totalExpense += expenses[i].amount;

    int totalSalary = 0;
    for (int i = 0; i < (int)masters.size(); i++) {
        if (masters[i].salaryType == 0) totalSalary += masters[i].salaryFixed;
        else                            totalSalary += (int)((float)totalIncome * masters[i].salaryPercent / 100.0f);
    }

    int profit = totalIncome - totalExpense - totalSalary;
    int taxAmount = (int)((float)profit * taxPercent / 100.0f);
    int netProfit = profit - taxAmount;

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), U8("ДОХОДЫ"));
    ImGui::Separator();
    ImGui::Text(U8("Доход из заявок: %d BYN  (%d заявок)"), totalIncome, (int)repairs.size());
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), U8("РАСХОДЫ"));
    ImGui::Separator();

    if (expenses.empty()) ImGui::Text(U8("Расходов нет."));
    else {

        for (int i = 0; i < (int)expenses.size(); i++) {
            ImGui::Text("  %d.  %s  -  %d %s", i + 1, expenses[i].comment, expenses[i].amount,
                reinterpret_cast<const char*>(u8"BYN"));
            ImGui::SameLine();
            char xId[20]; sprintf_s(xId, 20, "X##e%d", i);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.08f, 0.08f, 1.0f));
            if (ImGui::SmallButton(xId)) { expenses.erase(expenses.begin() + i); saveFinance(); break; }
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();
    ImGui::Text(U8("Добавить расход:"));
    ImGui::SetNextItemWidth(240.0f); ImGui::InputText("##expC", expenseComment, 100);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);  ImGui::InputInt("##expA", &expenseAmount);
    ImGui::SameLine();

    if (ImGui::Button(U8("Добавить##exp"))) {
        if (expenseComment[0] != '\0' && expenseAmount > 0) {
            Expense e;
            strncpy_s(e.comment, 100, expenseComment, _TRUNCATE);
            e.amount = expenseAmount;
            expenses.push_back(e); saveFinance();
            char logBuf[60]; sprintf_s(logBuf, 60, "%s", e.comment);
            writeLog(U8("РАСХОД"), logBuf);
            expenseComment[0] = '\0'; expenseAmount = 0;
        }
    }

    ImGui::Text(U8("Итого расходов: %d BYN"), totalExpense);
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), U8("ЗАРПЛАТЫ МАСТЕРОВ"));
    ImGui::Separator();
    for (int i = 0; i < (int)masters.size(); i++) {
        int sal = (masters[i].salaryType == 0)
            ? masters[i].salaryFixed
            : (int)((float)totalIncome * masters[i].salaryPercent / 100.0f);
        ImGui::Text("  %s:  %d %s", masters[i].name, sal, reinterpret_cast<const char*>(u8"BYN"));
    }
    ImGui::Text(U8("Итого зарплаты: %d BYN"), totalSalary);
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), U8("НАЛОГ"));
    ImGui::Separator();
    ImGui::Text(U8("Ставка налога (проц.):"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);

    if (ImGui::InputFloat("##tax", &taxPercent, 0.5f, 1.0f, "%.1f")) saveFinance();
    ImGui::Text(U8("Сумма налога: %d BYN"), taxAmount);
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
        U8("ИТОГ: %d - %d - %d - %d = %d BYN"), totalIncome, totalExpense, totalSalary, taxAmount, netProfit);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.08f, 0.08f, 1.0f));
    if (ImGui::Button(U8("Новый месяц (сброс)"), ImVec2(210.0f, 0.0f)))
        ImGui::OpenPopup(U8("ConfNewMonth"));
    ImGui::PopStyleColor();

    if (ImGui::BeginPopupModal(U8("ConfNewMonth"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(U8("Удалить все заявки и расходы за месяц?"));
        ImGui::Spacing();
        ImGui::Text(U8("Импортировать данные в Word перед сбросом?"));
        ImGui::Spacing();

        if (ImGui::Button(U8("Да + экспорт в Word"), ImVec2(190.0f, 0.0f))) {
            generateReport();
            ShellExecuteA(NULL, "open", "report.doc", NULL, NULL, SW_SHOWNORMAL);
            writeLog(U8("ЭКСПОРТ"), U8("report.doc"));
            repairs.clear();  saveRepairs();
            expenses.clear(); saveFinance();
            writeLog(U8("НОВЫЙ МЕСЯЦ"), U8("reset"));
            strcpy_s(messageText, U8("Отчёт создан. Месяц сброшен."));
            showMessage = true; ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button(U8("Да, без экспорта"), ImVec2(155.0f, 0.0f))) {
            repairs.clear();  saveRepairs();
            expenses.clear(); saveFinance();
            writeLog(U8("НОВЫЙ МЕСЯЦ"), U8("reset bez eksporta"));
            strcpy_s(messageText, U8("Месяц сброшен."));
            showMessage = true; ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button(U8("Отмена"), ImVec2(80.0f, 0.0f))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(U8("Импортировать в Word"), ImVec2(210.0f, 0.0f))) {
        generateReport();
        ShellExecuteA(NULL, "open", "report.doc", NULL, NULL, SW_SHOWNORMAL);
        writeLog(U8("ЭКСПОРТ"), U8("report.doc"));
        strcpy_s(messageText, U8("Отчёт сохранён: report.doc"));
        showMessage = true;
    }

    ImGui::End();
}



/////////////////////////////////////////////////////////
//             ОКНО: ЖУРНАЛ ИЗМЕНЕНИЙ                  //
/////////////////////////////////////////////////////////
static void showWindowLog()
{
    if (!windowLog) return;

    ImGui::SetNextWindowPos(ImVec2(60.0f, TH + 20.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(740.0f, 500.0f), ImGuiCond_Once);
    ImGui::Begin(U8("Журнал изменений"), &windowLog);

    if (ImGui::Button(U8("Обновить"))) loadLog();
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), U8("все действия записываются в log.txt"));
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGui::BeginChild("##logScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    const char* p = logContent.c_str();
    while (*p != '\0') {
        const char* lineEnd = strchr(p, '\n');
        if (!lineEnd) lineEnd = p + strlen(p);
        char lineBuf[512];

        int len = (int)(lineEnd - p);
        if (len >= 512) { len = 511; }

        memcpy(lineBuf, p, len);
        lineBuf[len] = '\0';

        ImVec4 lc = ImVec4(0.82f, 0.82f, 0.82f, 1.0f);
        if (strstr(lineBuf, reinterpret_cast<const char*>(u8"УДАЛЕНИЕ")) != NULL) lc = ImVec4(1.0f, 0.32f, 0.32f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"ДОБАВЛЕНИЕ")) != NULL) lc = ImVec4(0.32f, 1.0f, 0.42f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"ИЗМЕНЕНИЕ")) != NULL) lc = ImVec4(0.4f, 0.78f, 1.0f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"ЭКСПОРТ")) != NULL) lc = ImVec4(1.0f, 1.0f, 0.35f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"РАСХОД")) != NULL) lc = ImVec4(1.0f, 0.62f, 0.2f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"МЕСЯЦ")) != NULL) lc = ImVec4(0.78f, 0.4f, 1.0f, 1.0f);
        else if (strstr(lineBuf, reinterpret_cast<const char*>(u8"ЗАПУСК")) != NULL
            || strstr(lineBuf, reinterpret_cast<const char*>(u8"ВЫХОД")) != NULL) lc = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);

        ImGui::TextColored(lc, "%s", lineBuf);
        p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}



/////////////////////////////////////////////////////////
//               ОКНО: О ПРОГРАММЕ                     //
/////////////////////////////////////////////////////////
static void showWindowAbout()
{
    if (!windowAbout) return;

    ImGui::SetNextWindowPos(ImVec2(300.0f, TH + 60.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(450.0f, 320.0f), ImGuiCond_Once);
    ImGui::Begin(U8("О программе"), &windowAbout);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), U8("АРМ Работника компьютерной мастерской"));
    ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped(U8("Технический стек: C++, SFML 3.0, ImGui 1.91.1, ImGui-SFML 3.0"));
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped(U8("Автор: Романович Никита"));
    ImGui::TextWrapped(U8("Группа 88ТП"));
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (ImGui::Button(U8("Закрыть"), ImVec2(100.0f, 0.0f))) windowAbout = false;

    ImGui::End();
}



/////////////////////////////////////////////////////////
//                      ТУЛБАР                         //
/////////////////////////////////////////////////////////
static void showToolbar()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(W, TH));

    ImGuiWindowFlags tbFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("##toolbar", nullptr, tbFlags);

    float btnH = 28.0f;
    float padTop = (TH - btnH) * 0.5f - 4.0f;
    ImGui::SetCursorPosY(padTop);

    if (ImGui::Button(U8("Записи"), ImVec2(0.0f, btnH))) {
        loadRepairs(); windowRecords = true;
    }
    ImGui::SameLine();

    if (ImGui::Button(U8("Добавить заявку"), ImVec2(0.0f, btnH))) {
        loadRepairs();
        inputName[0] = inputDevice[0] = inputComment[0] = '\0';
        inputCost = 0; inputDays = 0; inputMasterIdx = 0; addStatusIdx = 0;
        windowAdd = true;
    }
    ImGui::SameLine();

    if (ImGui::Button(U8("Мастера"), ImVec2(0.0f, btnH))) {
        loadMasters(); windowManageMasters = true;
    }
    ImGui::SameLine();

    if (ImGui::Button(U8("Отчет"), ImVec2(0.0f, btnH))) {
        loadRepairs(); loadFinance(); loadMasters(); windowReport = true;
    }
    ImGui::SameLine();

    if (ImGui::Button(U8("Лог"), ImVec2(0.0f, btnH))) {
        loadLog(); windowLog = true;
    }
    ImGui::SameLine();

    if (ImGui::Button(U8("О программе"), ImVec2(0.0f, btnH))) {
        windowAbout = true;
    }
    ImGui::SameLine();

    ImGui::SetCursorPosX(W - 100.0f);
    ImGui::SetCursorPosY(padTop);
    if (ImGui::Button(U8("Выход"), ImVec2(90.0f, btnH)))
        gQuit = true;

    ImGui::End();
}



/////////////////////////////////////////////////////////
//               Отрисовка интерфейса                  //
/////////////////////////////////////////////////////////
void showGUI()
{
    ImGuiIO& io = ImGui::GetIO();
    W = io.DisplaySize.x;
    H = io.DisplaySize.y;

    showPopupMessage();
    showWindowRecords();
    showWindowEditRepair();
    showWindowAdd();
    showWindowManageMasters();
    showWindowEditMaster();
    showWindowReport();
    showWindowLog();
    showWindowAbout();
    showToolbar();
}