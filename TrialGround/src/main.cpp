#include <iostream>

#include "Lumina/Essence/WindowApplication.hpp"

using namespace Lumina;

class TrialGroundApplication : public Essence::WindowApplication {
public:
    TrialGroundApplication(): WindowApplication({800, 600}, "Lumina Trial Ground") {
        std::cout << "Hi!\n";
    }

    void Tick(float dt) override {
        WindowApplication::Tick(dt);
    }

private:
};

int main() {
    TrialGroundApplication app;
    app.Run();

    return 0;
}