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
#include "util/ini.h"
#include "util/cctracer_config.h"
#include <iostream>


int main() {
    cctracer::CCTracerConfig config;
    if (!config.load_from_ini(".cctracer.ini")) {
        std::cerr << "Failed to load .cctracer.ini" << std::endl;
        return 1;
    }
    std::cout << config << std::endl;
    return 0;
}