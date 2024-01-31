#pragma once

#include <functional>
#include <deque>
#include <tuple>
#include <string>

namespace Lumina::Essence {

class DeletionQueue {
public:
    ~DeletionQueue();

    void PushBack(std::function<void()> const&& deletor, std::string const&& name);
    void Flush();


private:
    std::deque<std::pair<std::function<void()>, std::string>> queue;
};

}