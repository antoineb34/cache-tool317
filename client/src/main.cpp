#include <iostream>
#include <stdexcept>
#include <string>

#include "ClientApp.h"

int main(int argc, char* argv[]) {
    std::string cachePath = "./cache";
    int regionId = 12850;
    bool smokeTest = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--smoke-test") {
            smokeTest = true;
        } else if (arg == "--region") {
            if (i + 1 >= argc) {
                std::cerr << "--region requires a region id\n";
                return 1;
            }
            regionId = std::stoi(argv[++i]);
        } else if (arg.rfind("--region=", 0) == 0) {
            regionId = std::stoi(arg.substr(9));
        } else {
            cachePath = arg;
        }
    }

    ClientApp app(cachePath, regionId, smokeTest);
    return app.run();
}
