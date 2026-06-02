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
#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace cctracer {

inline std::string getHomeDir() {
    return std::getenv("HOME");
}

inline void ltrim(std::string& s){
    s.erase(
        s.begin(), 
        std::find_if(s.begin(), s.end(), [](unsigned char ch){
            return !std::isspace(ch);
        })
    );
}

inline void rtrim(std::string& s){
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){
            return !std::isspace(ch);
        }).base(),
        s.end()
    );
}

inline void trim(std::string& s){
    ltrim(s);
    rtrim(s);
}

inline std::vector<std::string> split_string(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

namespace config {

struct Ini {
    std::string path;
    std::unordered_map<std::string, int> sections;
    std::vector<std::unordered_map<std::string, std::string>> sections_kv;

    bool load() {
        std::ifstream infile(path);
        if (!infile.is_open()) {
            std::cerr << "Failed to open INI file: " << path << std::endl;
            return false;
        }
        std::string line;
        std::string cur_section = "DEFAULT";
        sections[cur_section] = sections_kv.size();
        sections_kv.emplace_back();
        while (std::getline(infile, line)) {
            trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue; // Skip comments and empty lines
            }
            if (line.front() == '[' && line.back() == ']') {
                cur_section = line.substr(1, line.size() - 2);
                sections[cur_section] = sections_kv.size();
                sections_kv.emplace_back();
                continue;
            }
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                trim(key);
                std::string value = line.substr(pos + 1);
                trim(value);
                sections_kv[sections[cur_section]][key] = value;
            }
        }
        infile.close();
        return true;
    }
};

}
}