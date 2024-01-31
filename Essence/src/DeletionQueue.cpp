#include "Lumina/Essence/DeletionQueue.hpp"

#include <iostream>

namespace Lumina::Essence {

DeletionQueue::~DeletionQueue() {
    Flush();
}
void DeletionQueue::PushBack(std::function<void()> const&& deletor, std::string const&& name) {
    queue.emplace_back(deletor, name);
}
void DeletionQueue::Flush() {
    for (auto it = queue.rbegin(); it != queue.rend(); it++) {
        std::get<0> (*it)();
        std::cout << "Deleted " << std::get<1>(*it) << "\n";
    }

    queue.clear();
}
}