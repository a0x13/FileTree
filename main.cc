#include <iostream>
#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>


int main(int argc, char **argv)
{
    spdlog::info("{}", 1e9+7);
    for (int i = 0; i < 24; i++)
    {
        auto t = std::thread([]() {
            int64_t sum = 0;
            for (int64_t i = 0; i < INT64_MAX; i++) {
                sum += i;
            }
            spdlog::info("hello world {}", std::chrono::steady_clock::now().time_since_epoch().count());
        });
        
        t.detach();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1000));
    return 0;
}