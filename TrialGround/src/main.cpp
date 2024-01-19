#include <iostream>

#include "Lumina/Essence/Application.hpp"

using namespace Lumina;

class TrialGroundApplication : public Essence::Application {
public:
    TrialGroundApplication(): Application({800, 600}, "Lumina Trial Ground") {
        std::cout << "Hi!\n";
    }

    void Tick(float dt) override {
        Application::Tick(dt);
    }

private:
};

int main() {
    TrialGroundApplication app;
    app.Initialize();
    app.Run();

    return 0;
}