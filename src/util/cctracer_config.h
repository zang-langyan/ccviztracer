#pragma once
#include <iostream>
#include <regex>
#include "ini.h"
namespace cctracer {

struct CCTracerRules {
    std::vector<std::string> instrument_include_functions;
    std::vector<std::string> instrument_exclude_functions;
    std::vector<std::string> instrument_include_files;
    std::vector<std::string> instrument_exclude_files;

    std::vector<std::string> runtime_include_functions;
    std::vector<std::string> runtime_exclude_functions;
    std::vector<std::string> runtime_include_files;
    std::vector<std::string> runtime_exclude_files;

    bool should_instrument(const std::string& func_name, const std::string& file_name) const {
        if (instrument_include_functions.empty() && instrument_include_files.empty() 
            && instrument_exclude_functions.empty() && instrument_exclude_files.empty()) {
            return true; // No rules means include all
        }
        try {
            // Include rules take precedence over exclude rules
            // if none of include rules match, then return false
            if (!instrument_include_functions.empty()) {
                bool match = false;
                for (const auto& pattern : instrument_include_functions) {
                    const std::regex regex_pattern(pattern);
                    if (std::regex_match(func_name, regex_pattern)) {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    return false;
                }
            }
            if (!instrument_include_files.empty()) {
                bool match = false;
                for (const auto& pattern : instrument_include_files) {
                    const std::regex regex_pattern(pattern);
                    if (std::regex_match(file_name, regex_pattern)) {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    return false;
                }
            }
            if (!instrument_exclude_functions.empty()) {
                for (const auto& pattern : instrument_exclude_functions) {
                    if (std::regex_match(func_name, std::regex(pattern))) {
                        return false;
                    }
                }
            }
            if (!instrument_exclude_files.empty()) {
                for (const auto& pattern : instrument_exclude_files) {
                    if (std::regex_match(file_name, std::regex(pattern))) {
                        return false;
                    }
                }
            }
        } catch (const std::regex_error& e) {
            std::cerr << "regex_error caught: " << e.what() 
                << " Check your instrument_*_* configs. refuse to instrument." << '\n';
            return false;
        } catch (...) {
            std::cerr << "Unknown Error When checking should instrument. "
                "Check your instrument_*_* configs. refuse to instrument." << std::endl;
            return false;
        }
        return true;
    }

    bool should_trace(const std::string& func_name, const std::string& file_name) const {
        if (runtime_include_functions.empty() && runtime_include_files.empty() 
            && runtime_exclude_functions.empty() && runtime_exclude_files.empty()) {
            return true; // No rules means include all
        }
        try {
            // Include rules take precedence over exclude rules
            // if none of include rules match, then return false
            if (!runtime_include_functions.empty()) {
                bool match = false;
                for (const auto& pattern : runtime_include_functions) {
                    const std::regex regex_pattern(pattern);
                    if (std::regex_match(func_name, regex_pattern)) {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    return false;
                }
            }
            if (!runtime_include_files.empty()) {
                bool match = false;
                for (const auto& pattern : runtime_include_files) {
                    const std::regex regex_pattern(pattern);
                    if (std::regex_match(file_name, regex_pattern)) {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    return false;
                }
            }
            if (!file_name.empty() && !runtime_exclude_functions.empty()) {
                for (const auto& pattern : runtime_exclude_functions) {
                    if (std::regex_match(func_name, std::regex(pattern))) {
                        return false;
                    }
                }
            }
            if (!file_name.empty() && !runtime_exclude_files.empty()) {
                for (const auto& pattern : runtime_exclude_files) {
                    if (std::regex_match(file_name, std::regex(pattern))) {
                        return false;
                    }
                }
            }
        } catch (const std::regex_error& e) {
            std::cerr << "regex_error caught: " << e.what() 
                << " Check your runtime_*_* configs. refuse to trace." << '\n';
            return false;
        } catch (...) {
            std::cerr << "Unknown Error When checking should trace. "
                "Check your runtime_*_* configs. refuse to trace." << std::endl;
            return false;
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const CCTracerRules& rules) {
        if (rules.instrument_include_functions.empty() && rules.instrument_include_files.empty() 
            && rules.instrument_exclude_functions.empty() && rules.instrument_exclude_files.empty()
            && rules.runtime_include_functions.empty() && rules.runtime_include_files.empty()
            && rules.runtime_exclude_functions.empty() && rules.runtime_exclude_files.empty()) {
            os << "<no rules>";
        }
        os << "\n\t\tCCTracerRules:\n"
           << (rules.instrument_include_functions.empty() ? "" : "\t\t\tinstrument_include_functions=\n");
        for (const auto& pattern : rules.instrument_include_functions) {
            os << "\t\t\t\t" << pattern << "\n";
        }

        os << (rules.instrument_exclude_functions.empty() ? "" : "\t\t\tinstrument_exclude_functions=\n");
        for (const auto& pattern : rules.instrument_exclude_functions) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        
        os << (rules.instrument_include_files.empty() ? "" : "\t\t\tinstrument_include_files=\n");
        for (const auto& pattern : rules.instrument_include_files) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        os << (rules.instrument_exclude_files.empty() ? "" : "\t\t\tinstrument_exclude_files=\n");
        for (const auto& pattern : rules.instrument_exclude_files) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        os << (rules.runtime_include_functions.empty() ? "" : "\t\t\truntime_include_functions=\n");
        for (const auto& pattern : rules.runtime_include_functions) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        os << (rules.runtime_exclude_functions.empty() ? "" : "\t\t\truntime_exclude_functions=\n");
        for (const auto& pattern : rules.runtime_exclude_functions) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        os << (rules.runtime_include_files.empty() ? "" : "\t\t\truntime_include_files=\n");
        for (const auto& pattern : rules.runtime_include_files) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        os << (rules.runtime_exclude_files.empty() ? "" : "\t\t\truntime_exclude_files=\n");
        for (const auto& pattern : rules.runtime_exclude_files) {
            os << "\t\t\t\t" << pattern << "\n";
        }
        return os;
    }
};

struct CCTracerConfig {
    bool enable_tracing = false;
    bool use_perfetto = false;
    CCTracerRules rules;
    std::string trace_begin;
    std::string trace_until;
    bool enable_llvm_log = false;

    bool load_from_ini(const std::string& path) {
        cctracer::config::Ini ini{path};
        if (!ini.load()) {
            return false;
        }
        if (ini.sections_kv[ini.sections["DEFAULT"]].count("enable_tracing")) {
            enable_tracing = ini.sections_kv[ini.sections["DEFAULT"]]["enable_tracing"] == "1";
            const char* env_enable = getenv("CCTRACER_ENABLE");
            if (env_enable) {
                enable_tracing = std::string(env_enable) == "1";
            }
        }
        if (ini.sections_kv[ini.sections["DEFAULT"]].count("use_perfetto")) {
            use_perfetto = ini.sections_kv[ini.sections["DEFAULT"]]["use_perfetto"] == "1";
            const char* env_perfetto = getenv("CCTRACER_PERFETTO");
            if (env_perfetto) {
                use_perfetto = std::string(env_perfetto) == "1";
            }
        }
        if (ini.sections_kv[ini.sections["DEFAULT"]].count("enable_llvm_log")) {
            enable_llvm_log = ini.sections_kv[ini.sections["DEFAULT"]]["enable_llvm_log"] == "1";
        }
        if (ini.sections.count("trace_filters") > 0) {
            auto& kv = ini.sections_kv[ini.sections["trace_filters"]];
            if (kv.count("instrument_include_functions") > 0) {
                rules.instrument_include_functions = split_string(kv["instrument_include_functions"], ',');
            }
            if (kv.count("instrument_exclude_functions") > 0) {
                rules.instrument_exclude_functions = split_string(kv["instrument_exclude_functions"], ',');
            }
            if (kv.count("instrument_include_files") > 0) {
                rules.instrument_include_files = split_string(kv["instrument_include_files"], ',');
            }
            if (kv.count("instrument_exclude_files") > 0) {
                rules.instrument_exclude_files = split_string(kv["instrument_exclude_files"], ',');
            }
            if (kv.count("runtime_include_functions") > 0) {
                rules.runtime_include_functions = split_string(kv["runtime_include_functions"], ',');
            }
            if (kv.count("runtime_exclude_functions") > 0) {
                rules.runtime_exclude_functions = split_string(kv["runtime_exclude_functions"], ',');
            }
            if (kv.count("runtime_include_files") > 0) {
                rules.runtime_include_files = split_string(kv["runtime_include_files"], ',');
            }
            if (kv.count("runtime_exclude_files") > 0) {
                rules.runtime_exclude_files = split_string(kv["runtime_exclude_files"], ',');
            }
        }
        if (ini.sections.count("timeline") > 0) {
            auto& kv = ini.sections_kv[ini.sections["timeline"]];
            if (kv.count("trace_begin") > 0) {
                trace_begin = kv["trace_begin"];
            }
            if (kv.count("trace_until") > 0) {
                trace_until = kv["trace_until"];
            }
        }

        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const CCTracerConfig& config) {
        os << "CCTracerConfig:\n"
           << "\tenable_tracing=" << config.enable_tracing << "\n"
           << "\tuse_perfetto=" << config.use_perfetto << "\n"
           << "\tenable_llvm_log" << config.enable_llvm_log << "\n"
           << "\trules=" << config.rules << "\n"
           << "\ttrace_begin=" << config.trace_begin << "\n"
           << "\ttrace_until=" << config.trace_until << "\n";
        return os;
    }
};
} // namespace cctracer