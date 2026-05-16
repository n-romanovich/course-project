#include <SFML\Graphics.hpp>        //SFML 3.0
#include "imgui.h"                  //ImGui 1.91.1
#include "imgui-SFML.h"             //ImGui-SFML 3.0

#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <Windows.h>

#include "include/types.h"
#include "include/data.h"
#include "include/utils.h"
#include "include/gui.h"

using namespace std;

vector<Repair>  repairs;
vector<Master>  masters;
vector<Expense> expenses;

float taxPercent = 0.0f;

int main()
{
    initFiles();
    loadRepairs();
    loadMasters();
    loadFinance();
    writeLog(U8("ЗАПУСК"), U8("start"));

    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), L"АРМ Работника компьютерной мастерской");
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    ImGui::StyleColorsDark();

    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        static ImWchar cyrRanges[] = { 0x0020, 0x00FF, 0x0400, 0x04FF, 0 };
        ImFont* fnt = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f, nullptr, cyrRanges);
        if (!fnt)
            fnt = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\times.ttf", 16.0f, nullptr, cyrRanges);
        if (!fnt)
            io.Fonts->AddFontDefault();
        ImGui::SFML::UpdateFontTexture();
    }

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Time dt = clock.restart();

        while (auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
        }

        ImGui::SFML::Update(window, dt);

        showGUI();

        if (gQuit) { writeLog(U8("ВЫХОД"), U8("exit")); window.close(); }

        window.clear(sf::Color(10, 10, 10));

        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}