/* Copyright (C) 2026 zang-langyan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. */
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <string>
#include <ctime>
#include <regex>
#include <unistd.h>
#include "dumper.h"
#include "util/cctracer_config.h"
#include "util/ini.h"

namespace cctracer {

/* Tracing Control */
static CCTracerConfig g_config;
static bool load_config() {
    if (!g_config.load_from_ini(getHomeDir() + "/.cctracer.ini")) {
        std::cerr << "Failed to load CCTracer config from ~/.cctracer.ini,"
            << " using default config." << std::endl;
        return false;
    }
    return true;
}

static bool is_active(const char* func_name) {
    CCTracerConfig& config = g_config;
    if (config.trace_begin.empty() && config.trace_until.empty()) {
        return true; // No timeline means always trace
    }
    static thread_local bool g_active = false;
    if (!config.trace_begin.empty() && std::regex_search(func_name, g_config.trace_begin_regex)) {
        g_active = true;
    }
    if (!config.trace_until.empty() && std::regex_search(func_name, g_config.trace_until_regex)) {
        g_active = false;
    }
    return g_active;
}

} // namespace cctracer

namespace cctracer {

static Dumper& get_dumper() {
    static Dumper instance;
    return instance;
}
#define DUMPER get_dumper()

static FILE* g_trace_file = NULL;



static uint32_t get_thread_id() {
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return static_cast<uint32_t>(tid);
}

static uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

class TlsFlushGuard {
private:
    std::vector<TraceEvent> events;
public:
    TlsFlushGuard() {
        events.reserve(10240);
    }

    ~TlsFlushGuard() {
        if (events.size() > 0) {
            dump();
        }
    }

    const std::vector<TraceEvent>& getEvents() {
        return events;
    }

    void append(const TraceEvent& e) {
        events.emplace_back(e);
    }

    void dump() {
        get_dumper().dumpEvents(std::move(events));
        events.clear();
    }

    const size_t size() {
        return events.size();
    }

    const size_t capacity() {
        return events.capacity();
    }
};


static void emit_event(uint64_t begin_t, uint64_t end_t, const char* func_name, const char* file_name, int line, int column) {
    static thread_local TlsFlushGuard tls;
    static thread_local int cached_pid = getpid();
    static thread_local uint32_t cached_tid = get_thread_id();
    TraceEvent e {
        TraceEvent::eventType::X,
        cached_pid,
        cached_tid,
        begin_t,
        end_t - begin_t,
        line,
        column,
        func_name,
        file_name
    };
    if (tls.size() == tls.capacity()) {
        tls.dump();
    }
    tls.append(e);
}


/* cctracer */
static std::atomic<bool> g_cctracer_initialized{false};
static std::atomic<bool> g_cctracer_enabled{false};
void __attribute__((constructor)) init_cctracer() {
    if (!load_config()) {
        std::cerr << "Fail to load config, using default (disable tracing)";
        return;
    }
    if (!g_config.enable_tracing) {
        return;
    }
    g_cctracer_enabled.store(true, std::memory_order_release);

    g_trace_file = fopen("result.json", "w");
    if (!g_trace_file) {
        std::cerr << "Failed to open trace output file" << std::endl;
        return;
    }
    DUMPER.preinit(g_trace_file);
    DUMPER.initialize();

    g_cctracer_initialized.store(true, std::memory_order_release);
}

void __attribute__((destructor)) shutdown_cctracer() {
    if (!g_cctracer_initialized) {
        return;
    }
    if (!g_cctracer_enabled) {
        return;
    }
    
    g_cctracer_enabled.store(false, std::memory_order_release);
    DUMPER.stop();
}

} // namespace cctracer

extern "C" {

    uint64_t __cctracer_function_entry(const char* func_name, const char* file_name, int line, int column) {
        if (!cctracer::g_cctracer_initialized.load(std::memory_order_acquire)) {
            return 0;
        }
        if (!cctracer::g_cctracer_enabled.load(std::memory_order_acquire)) {
            return 0;
        }
        if (!cctracer::g_config.rules.should_trace(func_name, file_name)) {
           return 0;   
        }

        /* check if is within timeline */
        if (!cctracer::is_active(func_name)) {
            return 0;
        }

        /* Emit Event */
        uint64_t ts = cctracer::get_timestamp();
        return ts;
    }

    void __cctracer_function_exit(const char* func_name, const char* file_name, int line, int column, uint64_t begin_time) {
        if (!begin_time) {
            return;
        }

        /* Emit Event */
        uint64_t ts = cctracer::get_timestamp();
        cctracer::emit_event(begin_time, ts, func_name, file_name, line, column);
    }

}