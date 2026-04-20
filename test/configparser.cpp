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