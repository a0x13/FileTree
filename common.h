#pragma once

#include <string>
#include <chrono>
#include <filesystem>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;
namespace ch = std::chrono;


enum class Precision {
    ns = 0,
    us,
    ms,
    s
};

/*
 *  计时器
 */
template<Precision P = Precision::ns>
class Timer {
public:
    Timer(std::string in_tag) : _tag(in_tag), _start(ch::steady_clock::now()) {}

    ~Timer() {
        if (!spdlog::should_log(spdlog::level::debug)) {
            return;
        }
        auto cost_ns = ch::steady_clock::now() - _start;
        auto [count, unit] = convert(cost_ns);
        spdlog::debug("[Timer] [{}] cost {} {}", _tag, count, unit);
    }

private:
    template<typename Dur>
    static auto convert(Dur ns) noexcept {
        if constexpr (P == Precision::ns) {
            return std::pair{ns.count(), "ns"};
        } else if constexpr (P == Precision::us) {
            return std::pair{duration_cast<ch::microseconds>(ns).count(), "us"};
        } else if constexpr (P == Precision::ms) {
            return std::pair{duration_cast<ch::milliseconds>(ns).count(), "ms"};
        } else {
            return std::pair{duration_cast<ch::seconds>(ns).count(), "s"};
        }
    }

private:
    std::string _tag;
    ch::steady_clock::time_point _start;
};

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)
#define TIMER_SCOPE_NS(tag) Timer<Precision::ns> CONCAT( __func__, __COUNTER__)(tag);
#define TIMER_SCOPE_US(tag) Timer<Precision::us> CONCAT( __func__, __COUNTER__)(tag);
#define TIMER_SCOPE_MS(tag) Timer<Precision::ms> CONCAT( __func__, __COUNTER__)(tag);
#define TIMER_SCOPE_S(tag) Timer<Precision::s> CONCAT( __func__, __COUNTER__)(tag);