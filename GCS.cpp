#include <opencv2/opencv.hpp>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

using namespace cv;
using namespace mavsdk;
using namespace std;

int main() {
   
    setenv("FFMPEG_HTTP_TIMEOUT", "6000000", 1);
    string rtsp_url = "rtsp://192.168.144.25:8554/main.264";
    VideoCapture cap(rtsp_url);

    if (!cap.isOpened()) {
        cerr << "Error: Unable to connect to RTSP stream." << endl;
        return -1;
    }


    cap.set(CAP_PROP_BUFFERSIZE, 1);

    Mat frame;
    atomic<bool> running(true);
    mutex frame_mutex;

 
thread reader([&]() {
    while (running) {
        Mat local_frame;
        cap >> local_frame;

        if (local_frame.empty()) {
            cerr << "Warning: Frame is empty. Skipping..." << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid busy-waiting
            continue; // Skip this iteration and try again
        }

        lock_guard<mutex> lock(frame_mutex);
        frame = local_frame;
    }
});

    
    Mavsdk mavsdk{Mavsdk::Configuration{1, MAV_COMP_ID_ONBOARD_COMPUTER, false}};
    const string connection_url = "udp://192.168.144.12:19856";

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
        running = false;
        reader.join();
        cap.release();
        destroyAllWindows();
        return 1;
    }

    cout << "System connected, System ID: " << system->get_system_id() << endl;

    auto mavlink_passthrough = MavlinkPassthrough{system};
    double roll_deg = 0.0, pitch_deg = 0.0, yaw_deg = 0.0;
    vector<double> battery_voltages(10, -1.0);
    double latitude_deg = 0.0, longitude_deg = 0.0, altitude_m = 0.0;

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
                    battery_voltages[i] = battery_status.voltages[i] / 1000.0;
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
        });


    namedWindow("GCS Display", WINDOW_NORMAL);
while (running) {
    Mat local_frame;
    {
        lock_guard<mutex> lock(frame_mutex);
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Allow some time for a new frame
            continue; // Skip this iteration if no valid frame is available
        }
        local_frame = frame.clone();
    }

    {
        lock_guard<mutex> lock(data_mutex);

        // Update telemetry overlays on the frame
        putText(local_frame, "Pitch: " + to_string(pitch_deg) + " deg", Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Roll: " + to_string(roll_deg) + " deg", Point(10, 60),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Yaw: " + to_string(yaw_deg) + " deg", Point(10, 90),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Lat: " + to_string(latitude_deg), Point(10, 120),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Lon: " + to_string(longitude_deg), Point(10, 150),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Alt: " + to_string(altitude_m) + " m", Point(10, 180),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);

        // Battery calculations
        double total_voltage = 0.0;
        int valid_cells = 0;
        double max_voltage_per_cell = 4.2; // Max voltage for a single cell
        for (double voltage : battery_voltages) {
            if (voltage >= 0.0) {
                total_voltage += voltage;
                valid_cells++;
            }
        }

        double max_total_voltage = valid_cells * max_voltage_per_cell;
        double battery_percentage = (max_total_voltage > 0) ? (total_voltage / max_total_voltage) * 100.0 : 0.0;

        putText(local_frame, "Battery Voltage: " + to_string(total_voltage) + " V", Point(10, 210),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        putText(local_frame, "Battery Percentage: " + to_string(static_cast<int>(battery_percentage)) + " %", Point(10, 240),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
    }

    imshow("GCS Display", local_frame);

    if (waitKey(30) >= 0) {
        running = false;
    }
}

    running = false;
    reader.join();
    cap.release();
    destroyAllWindows();

    return 0;
}
