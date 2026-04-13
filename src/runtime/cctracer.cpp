#include <iomanip>
#include <perfetto.h>
#include <memory>
#include <string>
#include <sstream>
#include <ctime>
#include <atomic>
#include <unistd.h>

static std::atomic<bool> g_tracing_enabled{false};

static bool should_enable_tracing() {
    static const char* env = getenv("CCTRACER_ENABLE");
    return env && std::string(env) == "1";
}

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("cctracer").SetDescription("CCTracer Events")
);
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#define PERFETTO_BUFFER_SIZE_KB 1024 * 512

static std::unique_ptr<perfetto::TracingSession> g_tracing_session;

void __attribute__((constructor)) init_perfetto() {
    if (!should_enable_tracing()) {
        return;
    }
    g_tracing_enabled = true;
    
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::TraceConfig cfg;
    auto* buffer = cfg.add_buffers();
    buffer->set_size_kb(PERFETTO_BUFFER_SIZE_KB);
    buffer->set_fill_policy(
        perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy::TraceConfig_BufferConfig_FillPolicy_DISCARD
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

    perfetto::DataSourceConfig* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    g_tracing_session = perfetto::Tracing::NewTrace();
    g_tracing_session->Setup(cfg);
    g_tracing_session->StartBlocking();
}

void __attribute__((destructor)) shutdown_perfetto() {
    if (g_tracing_session) {
        g_tracing_session->StopBlocking();
    }
}

extern "C" {

    void __cctracer_function_entry(const char* func_name, const char* file_name, int line, int column) {
        if (!g_tracing_enabled) return;
        TRACE_EVENT_BEGIN("cctracer", perfetto::StaticString(func_name),
                    "file", file_name, "line", line, "column", column);
        // TRACE_EVENT_BEGIN("cctracer", perfetto::DynamicString(func_name));
    }

    void __cctracer_function_exit(const char* func_name, const char* file_name, int line, int column) {
        if (!g_tracing_enabled) return;
        // TRACE_EVENT_END("cctracer",
        //             "file", file_name, "line", line, "column", column);
        TRACE_EVENT_END("cctracer");
    }

}