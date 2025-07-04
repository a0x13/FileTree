#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <filesystem>
#include <vector>
#include <atomic>
#include <algorithm>
#include <numeric>

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
            // spdlog::info("travel: {}", entry.path().c_str());

            if (entry.is_directory() && !entry.is_symlink()) {
                count++;
                dir_queue.push(entry);
            }
        }
    }

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

template<typename T>
class SafeQueue
{
public:
    SafeQueue() = default;

    SafeQueue(SafeQueue &&other) noexcept {
        std::lock_guard lock(other._mut);
        _queue = std::move(other._queue);
    }

    SafeQueue& operator=(SafeQueue &&other) noexcept {
        if (this != &other) {
            std::scoped_lock scope(_mut, other._mut);
            _queue = std::move(other._queue);
        }

        return *this;
    }

    void enqueue(const T &t) {
        std::lock_guard lock(_mut);
        _queue.emplace(t);
    }

    bool dequeue(T &t) {
        std::lock_guard lock(_mut);
        if (_queue.empty()) {
            return false;
        }
        t = _queue.front();
        _queue.pop();
        return true;
    }

    inline size_t size() const {
        return _queue.size();
    }

private:
    std::queue<T> _queue;
    std::mutex _mut;
};

template<typename T>
class QueueManger 
{
public:
    QueueManger(int n) {
        _queues.resize(n);
        _idxs.resize(n);
        std::iota(_idxs.begin(), _idxs.end(), 0);
    }

    void enqueue(const T &t) {
        // 找到队列最小的加入队中
        SafeQueue<T> &queue = get_min_queue();
        queue.enqueue(t);
        _count++;
    }

    bool dequeue(int idx, T &t, std::function<void(void)> on_dequeue_sucess) {
        if (idx < 0 || idx >= _queues.size()) {
            return false;
        }

        bool sucess = _queues[idx].dequeue(t);
        if (sucess) {
            on_dequeue_sucess();
            _count--;
        }
        return sucess;
    }

    inline size_t size() const {
        return _count;
    }

    void monitor() {
        std::ostringstream msg;
        for (const auto &q: _queues) {
            msg << static_cast<int>(q.size()) << " ";
        }
        msg << " | ";
        for (const auto &i: _idxs) {
            msg << i << " ";
        }
        spdlog::debug("queue info: {}", msg.str());
    }

private:
    SafeQueue<T>& get_min_queue() {
        std::lock_guard lock(_mut);
        for (int i = _idxs.size() - 1; i > 0; i--) {
            int p = i / 2;
            if (_queues[_idxs[p]].size() > _queues[_idxs[i]].size()) {
                std::swap(_idxs[i], _idxs[p]);
            }
        }

        return _queues[_idxs[0]];
    }

private:
    std::vector<SafeQueue<T>> _queues;
    std::vector<int> _idxs;
    std::atomic<int> _count = 0;
    std::mutex _mut;
};

void queue_travel(const std::string &path)
{
    TIMER_SCOPE_MS("queue travel");

    int queue_num = 10;
    std::atomic<int> signal = 0;
    QueueManger<fs::directory_entry> qm(queue_num);
    std::atomic<int> count;

    qm.enqueue(fs::directory_entry(path));
    auto travel = [&qm, &signal, &count](int idx) {
        fs::directory_entry dir;
        while (signal > 0 || qm.size() > 0) {
            bool is_sucess = qm.dequeue(idx, dir, [&signal](){signal++;});
            if (is_sucess) {
                for (const auto &entry: fs::directory_iterator(dir.path())) {
                    // spdlog::info("travel: {}", entry.path().c_str());

                    if (entry.is_directory() && !entry.is_symlink()) {
                        count++;
                        qm.enqueue(entry);
                    }
                }
                signal--;
            }
        }
        // spdlog::debug("thread {} exit", idx);
        
    };

    // 监控每个队列的数量
    // std::thread monitor([&qm, &signal](){
    //     while (qm.size() > 0 || signal > 0) {
    //         qm.monitor();
    //         std::this_thread::sleep_for(ch::milliseconds(20));
    //     }
    // });

    std::vector<std::thread> ths;
    for (int i = 0; i < queue_num; i++) {
        ths.emplace_back(travel, i);
    }

    for (auto &t: ths) {
        t.join();
    }
    spdlog::info("directory num: {}", count);
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
    // multi_thread_travel(argv[1]);
    queue_travel(argv[1]);
    return 0;
}