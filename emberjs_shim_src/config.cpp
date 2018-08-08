#include <cstdlib>
#include <cstring>
#include <fstream>

namespace emberjs {

extern "C" {

char* ember_config_get() {
    const char* config = R"({
        "display": {
            "width": 640,
            "height": 480
        },
        "volume": 0.3
    })";
    auto str = (char*)malloc(strlen(config) + 1);
    strcpy(str, config);
    return str;
}

}

}
