#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

using namespace cv;
using namespace std;

int main() {
 setenv("FFMPEG_HTTP_TIMEOUT", "6000000", 1);

 string rtsp_url = "rtsp://192.168.144.25:8554/main.264";
 VideoCapture cap(rtsp_url);

 if (!cap.isOpened()) {
 cerr << "Error: Unable to connect to RTSP stream." << endl;
 return -1;
 }

 // Reduce buffer size
 cap.set(CAP_PROP_BUFFERSIZE, 1);

 Mat frame;

 atomic<bool> running(true);

 thread reader([&]() {
 while (running) {
 cap >> frame;
 if (frame.empty()) {

 cerr << "Error: Frame is empty." << endl;

 running = false;

 break;

 }
 }
 });

 namedWindow("Video", WINDOW_NORMAL);

 while (true) {
 if (!frame.empty()) {
 imshow("Video", frame);
 }

 if (waitKey(60) >= 0) {
 running = false;
 break;
 }

 }

 running = false;
 reader.join();

 cap.release();
 destroyAllWindows();

 return 0;
}