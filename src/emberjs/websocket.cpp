#include "websocket.hpp"

#include "../utility.hpp"

using json = nlohmann::json;

namespace emberjs {

    void from_json(const json& j, event_type& type) {
        type = static_cast<event_type>(j.get<int>());
    }

    websocket::websocket(const std::string& address, int port) :
        id(ember_ws_open(address.c_str(), port))
    {}

    websocket::websocket(websocket&& other) :
        id(std::exchange(other.id, 0)),
        message_callback(std::move(other.message_callback))
    {}

    websocket& websocket::operator=(websocket&& other) {
        this->~websocket();
        new (this) websocket(std::move(other));
        return *this;
    }

    websocket::~websocket() {
        ember_ws_close(id);
    }

    void websocket::on_open(std::function<void()> callback) {
        open_callback = std::move(callback);
    }

    void websocket::on_message(std::function<void(const std::string&)> callback) {
        message_callback = std::move(callback);
    }

    void websocket::on_close(std::function<void(const std::string&)> callback) {
        close_callback = std::move(callback);
    }

    void websocket::poll() {
        while (auto msg = ember_ws_poll(id)) {
            EMBER_DEFER { free(msg); };
            auto jmsg = json::parse(msg);
            switch ((event_type)jmsg["type"]) {
                case event_type::OPEN:
                    open_callback();
                    break;
                case event_type::MESSAGE:
                    message_callback(jmsg["message"]);
                    break;
                case event_type::CLOSE:
                    close_callback(jmsg["message"]);
                    break;
            }
        }
    }

    void websocket::send(const std::string msg) {
        ember_ws_send(id, msg.c_str());
    }

} //namespace emberjs
