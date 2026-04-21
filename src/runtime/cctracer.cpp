#include <cstdio>
#include <iostream>
#include <iomanip>
#include <perfetto.h>
#include <memory>
#include <string>
#include <sstream>
#include <ctime>
#include <regex>
#include <unistd.h>
#include "util/cctracer_config.h"
#include "util/ini.h"

namespace cctracer {

/* Tracing Control */
static CCTracerConfig& get_config() {
    static CCTracerConfig config;
    static bool loaded = false;
    if (!loaded) {
        if (!config.load_from_ini(getHomeDir() + "/.cctracer.ini")) {
            std::cerr << "Failed to load CCTracer config from ~/.cctracer.ini,"
                << " using default config." << std::endl;
        }
        loaded = true;
    }
    return config;
}

static bool is_active(const char* func_name) {
    CCTracerConfig& config = get_config();
    if (config.trace_begin.empty() && config.trace_until.empty()) {
        return true; // No timeline means always trace
    }
    static thread_local bool g_active = false;
    static const std::regex reg_begin_pat(config.trace_begin);
    static const std::regex reg_until_pat(config.trace_until);
    if (!config.trace_begin.empty() && std::regex_search(func_name, reg_begin_pat)) {
        g_active = true;
    }
    if (!config.trace_until.empty() && std::regex_search(func_name, reg_until_pat)) {
        g_active = false;
    }
    return g_active;
}

} // namespace cctracer

/* Perfetto Sdk */
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("cctracer").SetDescription("CCTracer Events")
);
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#define PERFETTO_BUFFER_SIZE_KB (1024 * 1024 * 2) // 2GB

static std::unique_ptr<perfetto::TracingSession> g_tracing_session;

void init_perfetto() {
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    args.shmem_size_hint_kb = 1024 * 32;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::TraceConfig cfg;
    auto* buffer = cfg.add_buffers();
    buffer->set_size_kb(PERFETTO_BUFFER_SIZE_KB);
    buffer->set_fill_policy(
        perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy::TraceConfig_BufferConfig_FillPolicy_RING_BUFFER
    );
    cfg.set_write_into_file(true);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  now - std::chrono::system_clock::from_time_t(time_t))
                  .count();
    std::stringstream ss;
    ss << "cctrace." << std::put_time(std::localtime(&time_t), "%Y%m%d%H%M%S") << "." << us << ".pftrace";
    cfg.set_output_path(ss.str());
    cfg.set_write_flush_mode(perfetto::TraceConfig::WriteFlushMode::TraceConfig_WriteFlushMode_WRITE_FLUSH_DISABLED);
    cfg.set_flush_period_ms(20);

    perfetto::DataSourceConfig* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    g_tracing_session = perfetto::Tracing::NewTrace();
    g_tracing_session->Setup(cfg);
    g_tracing_session->StartBlocking();
}

void shutdown_perfetto() {
    g_tracing_session->Flush([](bool success) {
        std::cout << "Final flush " << (success ? "succeeded" : "failed") << std::endl;
    });
    g_tracing_session->StopBlocking();
}

namespace cctracer {

/* Block and Write to json */
#define THREAD_BUFFER_SIZE (1024 * 1024 * 10)
#define FLUSH_THRESHOLD    (THREAD_BUFFER_SIZE - 4096) 

static FILE* g_trace_file = NULL;
static pthread_mutex_t g_file_lock = PTHREAD_MUTEX_INITIALIZER;

static thread_local char tls_buffer[THREAD_BUFFER_SIZE];
static thread_local size_t tls_pos = 0;
static thread_local int tls_first_event = 1; 

static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

static void append_event(const char* event_json, size_t len) {
    if (tls_pos + len + 1 > THREAD_BUFFER_SIZE) {
        pthread_mutex_lock(&g_file_lock);
        if (tls_pos > 0) {
            fprintf(g_trace_file, "%.*s", (int)tls_pos, tls_buffer);
            tls_pos = 0;
        }
        fprintf(g_trace_file, "%.*s", (int)len, event_json);
        pthread_mutex_unlock(&g_file_lock);
        tls_first_event = 0;
        return;
    }
    
    memcpy(tls_buffer + tls_pos, event_json, len);
    tls_pos += len;
    tls_first_event = 0;
    if (tls_pos >= FLUSH_THRESHOLD) {
        pthread_mutex_lock(&g_file_lock);
        fprintf(g_trace_file, "%.*s", (int)tls_pos, tls_buffer);
        tls_pos = 0;
        pthread_mutex_unlock(&g_file_lock);
    }
}

static void send_begin(const char* func_name, const char* file_name, int line, int column, uint64_t ts) {
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
                       "{\"ph\":\"B\",\"pid\":%d,\"tid\":%ld,\"ts\":%llu,\"name\":\"%s\",\"args\":{\"file\":\"%s\",\"line\":%d,\"column\":%d}},",
                       getpid(), (long)pthread_self(), (unsigned long long)ts,
                       func_name, file_name, line, column);
    append_event(buffer, len);
}

static void send_end(uint64_t ts) {
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "{\"ph\":\"E\",\"pid\":%d,\"tid\":%ld,\"ts\":%llu},",
                       getpid(), (long)pthread_self(), (unsigned long long)ts);
    append_event(buffer, len);
}


/* cctracer */
void __attribute__((constructor)) init_cctracer() {
    if (!get_config().enable_tracing) {
        return;
    }
    if (get_config().use_perfetto) {
        init_perfetto();
        return;
    } 
    g_trace_file = fopen("result.json", "w");
    if (!g_trace_file) {
        std::cerr << "Failed to open trace output file" << std::endl;
        return;
    }
    fprintf(g_trace_file, "{\"traceEvents\":[");
}

void __attribute__((destructor)) shutdown_cctracer() {
    if (!get_config().enable_tracing) {
        return;
    }
    if (get_config().use_perfetto) {
        if (!g_tracing_session) {
            std::cerr << "No tracing session to shut down." << std::endl;
            return;
        }
        shutdown_perfetto();
        return;
    }
    if (!g_trace_file) {
        std::cerr << "Trace file not open, cannot write trace output" << std::endl;
        return;
    }
    pthread_mutex_lock(&g_file_lock);
    if (tls_pos > 0) {
        fprintf(g_trace_file, "%.*s", (int)tls_pos, tls_buffer);
        tls_pos = 0;
    }
    fprintf(g_trace_file, "]}");
    fclose(g_trace_file);
    g_trace_file = NULL;
    pthread_mutex_unlock(&g_file_lock);
}

} // namespace cctracer

extern "C" {

    bool __cctracer_function_entry(const char* func_name, const char* file_name, int line, int column) {
        if (!cctracer::get_config().enable_tracing) return false;
        if (!cctracer::get_config().rules.should_trace(func_name, file_name)) {
           return false;   
        }

        /* check if is within timeline */
        if (!cctracer::is_active(func_name)) {
            return false;
        }

        /* Emit Event */
        if (cctracer::get_config().use_perfetto) {
             TRACE_EVENT_BEGIN("cctracer", perfetto::StaticString(func_name),
                    "file", file_name, "line", line, "column", column);
            return true;
        }
        uint64_t ts = cctracer::get_timestamp_us();
        cctracer::send_begin(func_name, file_name, line, column, ts);
        return true;
    }

    void __cctracer_function_exit(const char* func_name, const char* file_name, int line, int column, bool has_begun) {
        if (!has_begun) return;

        /* Emit Event */
        if (cctracer::get_config().use_perfetto) {
            TRACE_EVENT_END("cctracer");
            return;
        }
        uint64_t ts = cctracer::get_timestamp_us();
        cctracer::send_end(ts);
    }

}