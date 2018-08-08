#ifndef CLIENT_EMBERJS_WEBSOCKET_HPP
#define CLIENT_EMBERJS_WEBSOCKET_HPP

#include "../json.hpp"

#include <functional>
#include <string>
#include <utility>

namespace emberjs {

    extern "C" {
        extern int ember_ws_open(const char* addr, int port);
        extern char* ember_ws_poll(int id);
        extern void ember_ws_send(int id, const char* msg);
        extern void ember_ws_close(int id);
    }

    enum class event_type {
        OPEN,
        MESSAGE,
        CLOSE,
    };

    void from_json(const nlohmann::json& j, event_type& type);

    class websocket {
    public:
        websocket() = default;

        websocket(const std::string& address, int port);

        websocket(const websocket&) = delete;

        websocket(websocket&& other);

        websocket& operator=(const websocket&) = delete;

        websocket& operator=(websocket&& other);

        ~websocket();

        void on_open(std::function<void()> callback);

        void on_message(std::function<void(const std::string&)> callback);

        void on_close(std::function<void(const std::string&)> callback);

        void poll();

        void send(const std::string msg);

    private:
        int id = 0;

        std::function<void()> open_callback = []{};

        std::function<void(const std::string&)> message_callback = [](const std::string&){};

        std::function<void(const std::string&)> close_callback = [](const std::string&){};
    };

} //namespace emberjs

#endif //CLIENT_EMBERJS_WEBSOCKET_HPP
