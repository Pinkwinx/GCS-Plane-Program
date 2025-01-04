import tkinter as tk
import cv2
from PIL import Image, ImageTk
import threading
import time

window = tk.Tk()
window.title("AscendOS")
window.geometry("1920x1080")

# For Battery updates
def update_battery_icon(canvas, percentage):
    canvas.create_rectangle(2, 2, 58, 24, outline="black", width=2)
    canvas.create_rectangle(58, 8, 62, 18, fill="black")
    fill_width = int(56 * (percentage / 100))
    canvas.create_rectangle(2, 2, 2 + fill_width, 24, fill="green")
    canvas.create_rectangle(2 + fill_width, 2, 58, 24, fill="gray")

top_frame = tk.Frame(window)
top_frame.pack(side="top", fill="x", padx=10, pady=10)

battery_frame = tk.Frame(top_frame)
battery_frame.pack(side="left")

battery_percentage = 25
voltage = 12.5

battery_icon = tk.Canvas(battery_frame, width=64, height=26, bg="white", highlightthickness=0)
battery_icon.pack(side="left", padx=5)
update_battery_icon(battery_icon, battery_percentage)

battery_label = tk.Label(battery_frame, text=f"{battery_percentage}% | {voltage}V", font=("Arial", 14))
battery_label.pack(side="left")

title_label = tk.Label(top_frame, text="AscendOS", font=("Arial", 30))
title_label.pack(side="right")

# For longitude latitude and altitude
position_frame = tk.Frame(window)
position_frame.pack(pady=10)

longitude = 0.0
latitude = 0.0
altitude = 0.0

longitude_label = tk.Label(position_frame, text=f"Longitude: {longitude}°", font=("Arial", 14))
longitude_label.grid(row=0, column=0, padx=20)

latitude_label = tk.Label(position_frame, text=f"Latitude: {latitude}°", font=("Arial", 14))
latitude_label.grid(row=0, column=1, padx=20)

altitude_label = tk.Label(position_frame, text=f"Altitude: {altitude}m", font=("Arial", 14))
altitude_label.grid(row=0, column=2, padx=20)

# For both Camera and plane model
main_frame = tk.Frame(window)
main_frame.pack(pady=10, padx=10, fill="both", expand=True)

main_frame.pack_propagate(False)

video_label = tk.Label(main_frame, text="Video Camera", font=("Arial", 16), bg="gray")
video_label.pack(side="left", padx=20, fill="both", expand=True)

plane_model = tk.Label(main_frame, text="Plane Model", font=("Arial", 16), bg="black")
plane_model.pack(side="right", padx=20, fill="both", expand=True)

# Function to adjust the sizes of video_label and plane_model
def resize_labels(event=None):
    frame_width = main_frame.winfo_width()
    frame_height = main_frame.winfo_height()
    label_width = frame_width // 2 - 40
    label_height = frame_height
    video_label.config(width=label_width, height=label_height)
    plane_model.config(width=label_width, height=label_height)

window.bind("<Configure>", resize_labels)

# For roll pitch and yaw
control_frame = tk.Frame(window)
control_frame.pack(pady=20)

roll = 0
pitch = 0
yaw = 0

roll_label = tk.Label(control_frame, text=f"Roll: {roll}°", font=("Arial", 14))
roll_label.grid(row=0, column=0, padx=20)

pitch_label = tk.Label(control_frame, text=f"Pitch: {pitch}°", font=("Arial", 14))
pitch_label.grid(row=0, column=1, padx=20)

yaw_label = tk.Label(control_frame, text=f"Yaw: {yaw}°", font=("Arial", 14))
yaw_label.grid(row=0, column=2, padx=20)


cap = None
frame_buffer = None
last_frame_time = time.time()

# updated video capture
def capture_video():
    global cap, frame_buffer, last_frame_time
    while True:
        try:
            if cap is None or not cap.isOpened():
                print("Attempting to reconnect to video stream...")
                cap = cv2.VideoCapture('rtsp://192.168.144.25:8554/main.264', cv2.CAP_FFMPEG)
                cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduce buffer size to minimize latency 
                cap.set(cv2.CAP_PROP_FPS, 30)  # Set a fixed frame rate
                time.sleep(2) 

            ret, frame = cap.read()
            if ret:
                last_frame_time = time.time()
                frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                frame_buffer = frame_rgb  
            else:
                print("Error: No frame received.")
                time.sleep(1)
        except Exception as e:
            print(f"Error during video capture: {e}")
            time.sleep(2)

# Function to update the video frame in Tkinter
def update_video_frame():
    global frame_buffer
    if frame_buffer is not None:
        image = Image.fromarray(frame_buffer)
        photo = ImageTk.PhotoImage(image)
        video_label.config(image=photo)
        video_label.image = photo
    window.after(10, update_video_frame)

# Start the capture thread
capture_thread = threading.Thread(target=capture_video, daemon=True)
capture_thread.start()

resize_labels()
update_video_frame()

window.mainloop()

if cap:
    cap.release()
