#pragma once

#include <string>
#include <chrono>
#include <filesystem>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;
namespace ch = std::chrono;


/*
 *  计时器
 */
class Timer {
public:
    enum class Precision {
        ns = 0,
        us,
        ms,
        s
    };

public:
    Timer(std::string in_tag, Precision in_p = Precision::ns) 
        : _tag(in_tag), _precision(in_p) {
        _start = ch::high_resolution_clock::now();
    }

    ~Timer() {
        auto cost_ns = ch::high_resolution_clock::now() - _start;
        int64_t count;
        const char *unit;
        switch (_precision) {
        case Precision::us:
            unit = "us";
            count = ch::duration_cast<ch::microseconds>(cost_ns).count();
            break;

        case Precision::ms:
            unit = "ms";
            count = ch::duration_cast<ch::milliseconds>(cost_ns).count();
            break;

        case Precision::s:
            unit = "s";
            count = ch::duration_cast<ch::seconds>(cost_ns).count();
            break;

        default:
            unit = "ns";
            count = cost_ns.count();
            break;
        }
        spdlog::debug("[Timer] [{}] cost {} {}", _tag, count, unit);
    }

private:
    Precision _precision;
    std::string _tag;
    ch::high_resolution_clock::time_point _start;
};

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)
#define TIMER_SCOPE_NS(tag) Timer CONCAT( __func__, __LINE__)(tag, Timer::Precision::ns);
#define TIMER_SCOPE_US(tag) Timer CONCAT( __func__, __LINE__)(tag, Timer::Precision::us);
#define TIMER_SCOPE_MS(tag) Timer CONCAT( __func__, __LINE__)(tag, Timer::Precision::ms);
#define TIMER_SCOPE_S(tag) Timer CONCAT( __func__, __LINE__)(tag, Timer::Precision::s);