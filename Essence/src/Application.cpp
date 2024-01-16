#include "Lumina/Essence/Application.hpp"

#include <format>
#include <iostream>

namespace Lumina::Essence {

Application::Application() {
    std::cout << "Initializing Application\n";
}
Application::~Application() {}


void Application::Run() {
    IsRunning = true;
    while (IsRunning){
        Tick(1/60.0f);
    }
}
void Application::Exit(){
    IsRunning = false;
}


void Application::Tick(float dt) {}

}