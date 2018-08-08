
#include <exception>

namespace emberjs {

    extern "C" {
        int ember_ws_open(const char* addr, int port) { std::terminate(); }
        char* ember_ws_poll(int id) { std::terminate(); }
        void ember_ws_send(int id, const char* msg) { std::terminate(); }
        void ember_ws_close(int id) { std::terminate(); }
    }

}
