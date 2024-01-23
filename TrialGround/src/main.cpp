#include <iostream>

#include "Lumina/Essence/Application.hpp"

using namespace Lumina;

class TrialGroundApplication : public Essence::Application {
public:
    TrialGroundApplication(): Application({800, 600}, "Trial Ground") {}

    void Initialize() override {
        Application::Initialize();
        std::cout << "Hello, World!\n";
    }

private:
};

int main() {
    TrialGroundApplication app;
    app.Initialize();
    app.Run();

    return 0;
}