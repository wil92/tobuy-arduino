#include <cassert>
#include <string>

#include "wifi_connection.h"

struct FakeWifi {
    bool mode_ok = true;
    bool reconnect_ok = true;
    bool persistent_value = true;
    int mode_calls = 0;
    int reconnect_calls = 0;
    int begin_calls = 0;
    int status_calls = 0;
    wl_status_t begin_result = WL_IDLE_STATUS;
    wl_status_t current_status = WL_IDLE_STATUS;
    std::string ssid;
    std::string password;

    void persistent(bool value) { persistent_value = value; }
    bool mode(wifi_mode_t value) {
        assert(value == WIFI_STA);
        ++mode_calls;
        return mode_ok;
    }
    bool setAutoReconnect(bool value) {
        assert(value);
        ++reconnect_calls;
        return reconnect_ok;
    }
    wl_status_t begin(const char *network, const char *pass) {
        ++begin_calls;
        ssid = network;
        password = pass;
        return begin_result;
    }
    wl_status_t status() {
        ++status_calls;
        return current_status;
    }
    const char *localIP() const { return "192.0.2.1"; }
};

struct FakeLogger {
    std::string output;
    void print(const char *value) { output += value; }
    void println(const char *value) { output += std::string(value) + "\n"; }
    void println(int value) { output += std::to_string(value) + "\n"; }
};

int main() {
    FakeWifi disabled_wifi;
    FakeLogger disabled_log;
    WifiConnection<FakeWifi, FakeLogger> disabled(disabled_wifi, disabled_log);
    disabled.begin("", "");
    disabled.update();
    assert(disabled_wifi.mode_calls == 0 && disabled_wifi.status_calls == 0);
    assert(disabled_log.output.find("Wi-Fi disabled") != std::string::npos);

    FakeWifi mode_wifi;
    FakeLogger mode_log;
    mode_wifi.mode_ok = false;
    WifiConnection<FakeWifi, FakeLogger> mode_failure(mode_wifi, mode_log);
    mode_failure.begin("network", "password");
    mode_failure.update();
    assert(mode_wifi.begin_calls == 0 && mode_wifi.status_calls == 0);
    assert(mode_log.output.find("station mode") != std::string::npos);

    FakeWifi reconnect_wifi;
    FakeLogger reconnect_log;
    reconnect_wifi.reconnect_ok = false;
    WifiConnection<FakeWifi, FakeLogger> reconnect_failure(reconnect_wifi, reconnect_log);
    reconnect_failure.begin("network", "password");
    assert(reconnect_wifi.begin_calls == 0);
    assert(reconnect_log.output.find("automatic reconnection") != std::string::npos);

    FakeWifi failed_wifi;
    FakeLogger failed_log;
    failed_wifi.begin_result = WL_CONNECT_FAILED;
    WifiConnection<FakeWifi, FakeLogger> start_failure(failed_wifi, failed_log);
    start_failure.begin("network", "password");
    start_failure.update();
    assert(failed_wifi.status_calls == 0);
    assert(failed_log.output.find("could not start connection") != std::string::npos);

    FakeWifi wifi;
    FakeLogger log;
    WifiConnection<FakeWifi, FakeLogger> connection(wifi, log);
    connection.begin("network", "");
    assert(!wifi.persistent_value && wifi.mode_calls == 1 && wifi.reconnect_calls == 1);
    assert(wifi.begin_calls == 1 && wifi.ssid == "network" && wifi.password.empty());
    connection.update();
    const std::string initial_log = log.output;
    connection.update();
    assert(log.output == initial_log);
    wifi.current_status = WL_CONNECTED;
    connection.update();
    assert(log.output.find("Wi-Fi connected; IP address: 192.0.2.1") != std::string::npos);
    wifi.current_status = WL_DISCONNECTED;
    connection.update();
    assert(log.output.find("Wi-Fi status: " + std::to_string(WL_DISCONNECTED)) != std::string::npos);
}
