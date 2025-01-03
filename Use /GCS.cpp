#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <cmath> 
#include <vector>
#include <mutex>

using namespace mavsdk;
using namespace std;

void usage(const string& bin_name)
{
    cerr << "Usage : " << bin_name << " <connection_url>\n"
         << "Connection URL format should be:\n"
         << "  For UDP  : udp://[bind_host][:bind_port]\n"
         << "  For TCP  : tcp://[server_host][:server_port]\n"
         << "  For Serial: serial:///path/to/serial/dev[:baudrate]\n"
         << "Example: udp://:14540\n";
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }


    Mavsdk mavsdk{Mavsdk::Configuration{1, MAV_COMP_ID_ONBOARD_COMPUTER, false}};
    const string connection_url = argv[1];
    const ConnectionResult connection_result = mavsdk.add_any_connection(connection_url);

    if (connection_result != ConnectionResult::Success) {
        cerr << "Connection failed: " << connection_result << endl;
        return 1;
    }


    shared_ptr<System> system = nullptr;
    cout << "Waiting for system to connect...\n";

    for (int i = 0; i < 30; ++i) { 
        if (!mavsdk.systems().empty()) {
            system = mavsdk.systems().at(0);
            break;
        }
        cout << "Attempt " << i + 1 << ": No system detected yet.\n";
        this_thread::sleep_for(chrono::seconds(1));
    }

    if (!system) {
        cerr << "Timed out waiting for a system to connect.\n";
        return 1;
    }

    cout << "System connected, System ID: " << system->get_system_id() << endl;

    auto mavlink_passthrough = MavlinkPassthrough{system};

   
    double roll_deg = 0.0;
    double pitch_deg = 0.0;
    double yaw_deg = 0.0;
    vector<double> battery_voltages(10, -1.0);
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    double altitude_m = 0.0;

  
    mutex data_mutex;

  
    mavlink_passthrough.subscribe_message(
        MAVLINK_MSG_ID_ATTITUDE,
        [&roll_deg, &pitch_deg, &yaw_deg, &data_mutex](const mavlink_message_t& message) {
            mavlink_attitude_t attitude;
            mavlink_msg_attitude_decode(&message, &attitude);

            lock_guard<mutex> lock(data_mutex);
            roll_deg = attitude.roll * 180.0 / M_PI;
            pitch_deg = attitude.pitch * 180.0 / M_PI;
            yaw_deg = attitude.yaw * 180.0 / M_PI;
        });

 
    mavlink_passthrough.subscribe_message(
        MAVLINK_MSG_ID_BATTERY_STATUS,
        [&battery_voltages, &data_mutex](const mavlink_message_t& message) {
            mavlink_battery_status_t battery_status;
            mavlink_msg_battery_status_decode(&message, &battery_status);

            lock_guard<mutex> lock(data_mutex);
            for (int i = 0; i < 10; ++i) {
                if (battery_status.voltages[i] != std::numeric_limits<uint16_t>::max()) {
                    battery_voltages[i] = battery_status.voltages[i] / 1000.0; // Convert mV to V
                }
            }
        });


    mavlink_passthrough.subscribe_message(
        MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        [&latitude_deg, &longitude_deg, &altitude_m, &data_mutex](const mavlink_message_t& message) {
            mavlink_global_position_int_t global_position;
            mavlink_msg_global_position_int_decode(&message, &global_position);

            lock_guard<mutex> lock(data_mutex);
            latitude_deg = global_position.lat / 1e7;    
            longitude_deg = global_position.lon / 1e7;  
            altitude_m = global_position.alt / 1000.0;  
            // unit conversions ^^
        });


    cout << "Listening for attitude, battery, and GPS data... ";
    while (true) {
        {
            lock_guard<mutex> lock(data_mutex);
            double total_voltage = 0; 
      
            cout << "\033[2J\033[H"; 

            cout << "Pitch: " << pitch_deg << "°" << endl;
            if (pitch_deg > 10) {
                cout << "WARNING: STALL ANGLE EXCEEDED!" << endl;
            }
            cout << "Roll: " << roll_deg << "°" << endl;
            cout << "Yaw: " << yaw_deg << "°" << endl;

            cout << "Battery Voltages: ";
            for (double voltage : battery_voltages) {
                if (voltage >= 0.0) {
                    cout << voltage << " V ";
                    total_voltage += voltage;
                }
            }
            cout << endl;
            cout << "Total Battery Voltage: " << total_voltage << endl;
            cout << endl;
            cout << "GPS Coordinates: " << endl;
            cout << "  Latitude: " << latitude_deg << "°" << endl;
            cout << "  Longitude: " << longitude_deg << "°" << endl;
            cout << "  Altitude: " << altitude_m << " m" << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(500)); // Update every 500 ms
    }

    return 0;
}
