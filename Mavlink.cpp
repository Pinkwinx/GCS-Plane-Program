#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <memory>
#include <thread>
#include <string>

using namespace mavsdk;

std::string flight_mode_to_string(Telemetry::FlightMode mode) {
    switch (mode) {
        case Telemetry::FlightMode::Unknown: return "Unknown";
        case Telemetry::FlightMode::Manual: return "Manual";
        case Telemetry::FlightMode::Stabilized: return "Stabilized";
        // Add other cases as necessary
        default: return "Invalid";
    }
}

std::string landed_state_to_string(Telemetry::LandedState state) {
    switch (state) {
        case Telemetry::LandedState::Unknown: return "Unknown";
        case Telemetry::LandedState::OnGround: return "On Ground";
        case Telemetry::LandedState::TakingOff: return "Taking Off";
        case Telemetry::LandedState::Landing: return "Landing";
        // Add other cases as necessary
        default: return "Invalid";
    }
}

std::string connection_result_to_string(ConnectionResult result) {
    switch (result) {
        case ConnectionResult::Success: return "Success";
        case ConnectionResult::ConnectionError: return "Connection Error";
        case ConnectionResult::Timeout: return "Timeout";
        // Add other cases as necessary
        default: return "Unknown Connection Result";
    }
}

void print_position(Telemetry::Position position) {
    std::cout << "Position: "
              << "Latitude: " << position.latitude_deg
              << ", Longitude: " << position.longitude_deg
              << ", Altitude: " << position.absolute_altitude_m
              << std::endl;
}

void print_battery(Telemetry::Battery battery) {
    std::cout << "Battery: "
              << "Remaining: " << battery.remaining_percent * 100 << "%"
              << ", Voltage: " << battery.voltage_v << "V"
              << std::endl;
}

void print_flight_mode(Telemetry::FlightMode flight_mode) {
    std::cout << "Flight Mode: " << flight_mode_to_string(flight_mode) << std::endl;
}

void print_landed_state(Telemetry::LandedState landed_state) {
    std::cout << "Landed State: " << landed_state_to_string(landed_state) << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <connection_url>" << std::endl;
        return 1;
    }

    Mavsdk mavsdk; // Ensure to initialize correctly
    const auto connection_url = argv[1];
    auto result = mavsdk.add_any_connection(connection_url);
    if (result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result_to_string(result) << std::endl;
        return 1;
    }

    std::cout << "Waiting for system to connect..." << std::endl;
    auto system = mavsdk.systems().at(0);
    if (!system) {
        std::cerr << "No system found!" << std::endl;
        return 1;
    }

    Telemetry telemetry(system);

    telemetry.subscribe_position(print_position);
    telemetry.subscribe_battery(print_battery);
    telemetry.subscribe_flight_mode(print_flight_mode);
    telemetry.subscribe_landed_state(print_landed_state);

    std::cout << "Listening for telemetry data..." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
