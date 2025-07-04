#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "common.h"


void single_thread_travel(const std::string &path)
{
    TIMER_SCOPE_MS("single thread travel file");

    std::queue<fs::directory_entry> dir_queue;
    dir_queue.emplace(path);

    int count = 0;

    while (!dir_queue.empty()) {
        fs::directory_entry dir = dir_queue.front();
        dir_queue.pop();

        for (const auto &entry: fs::directory_iterator(dir.path())) {
            spdlog::info("travel: {}", entry.path().c_str());

            if (entry.is_directory() && !entry.is_symlink()) {
                count++;
                dir_queue.push(entry);
            }
        }
    }

    spdlog::info("---------------------------------------");
    spdlog::info("directory count: {}", count);
    
}

void multi_thread_travel(const std::string &path)
{
    // 线程会提前结束，最终还是会回到单线程执行
    TIMER_SCOPE_MS("multi thread travel file");

    std::mutex queue_mut;
    std::queue<fs::directory_entry> dir_queue;

    auto travel = [&] () {
        fs::directory_entry dir;
        while (true) {
            {
                std::lock_guard lock(queue_mut);
                if (dir_queue.empty()) {
                    break;
                }
                dir = dir_queue.front();
                dir_queue.pop();
            }

            for (const auto &entry: fs::directory_iterator(dir.path())) {
                // spdlog::info("travel: {}", entry.path().c_str());

                if (entry.is_directory() && !entry.is_symlink()) {
                    dir_queue.push(entry);
                }
            }
        }

        spdlog::info("-----> thread exit");
    };

    dir_queue.emplace(path);
    std::thread t1(travel);
    std::thread t2(travel);

    t1.join();
    t2.join();
}


int main(int argc, char **argv)
{
    spdlog::set_level(spdlog::level::debug);

    if (argc < 2) {
        spdlog::error("Usage: ft <file path>");
        return -1;
    }

    if (!fs::exists(argv[1])) {
        spdlog::error("input '{}' not exist", argv[1]);
        return -1;
    }

    if (!fs::is_directory(argv[1])) {
        spdlog::error("input {} is not directory", argv[1]);
        return -1;
    }
    
    // single_thread_travel(argv[1]);
    multi_thread_travel(argv[1]);
    return 0;
}