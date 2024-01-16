#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"

namespace Lumina::Essence {

class Application : NonCopyable{
public:
    Application();
    virtual ~Application();

    virtual void Tick(float dt);

    void Run();
    void Exit();

private:
    bool IsRunning = false;
};

}