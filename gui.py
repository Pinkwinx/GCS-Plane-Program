import tkinter as tk
import cv2
from PIL import Image, ImageTk


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

video_label = tk.Label(main_frame, text="Video Camera", font=("Arial", 16), bg="gray")
video_label.pack(side="left", padx=20, fill="both", expand=True)

plane_model = tk.Label(main_frame, text="Plane Model", font=("Arial", 16), width=50, height=20, bg="black")
plane_model.pack(side="right", padx=20)

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

# dumbass stream
def update_video_frame():
    ret, frame = cap.read()
    if ret:
        
        video_width = video_label.winfo_width()
        video_height = video_label.winfo_height()

      
        frame = cv2.resize(frame, (600, 200))

        
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = Image.fromarray(frame_rgb)
        photo = ImageTk.PhotoImage(image)


        video_label.config(image=photo)
        video_label.image = photo


    window.after(10, update_video_frame)


cap = cv2.VideoCapture('rtsp://192.168.144.25:8554/main.264')


update_video_frame()


window.mainloop()


cap.release()
