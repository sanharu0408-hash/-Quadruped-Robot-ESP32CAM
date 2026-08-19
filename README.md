# 🤖 Quadruped Robot (Freenove ESP32-S3 CAM)

A quadruped walking robot powered by the **Freenove ESP32-S3 CAM**. Features real-time video streaming and wireless remote control over Wi-Fi.

---

## 🛠️ Parts List

| Part Name | Model / Specs | Qty | Notes |
| :--- | :--- | :---: | :--- |
| **Main Controller** | Freenove ESP32-S3 WROOM CAM | 1 | Wi-Fi / Bluetooth / Camera onboard |
| **Servo Motor** | SG90 / MG90S |8 | 2 DOF per leg × 4 legs |
| **Servo Driver** | PCA9685 |  1  | 16-Channel 12-bit PWM I2C Driver |
| **Power Supply** | AA nickel-metal hydride battery | 4 | |
---

## 🔌 Pinout & Wiring

### Freenove ESP32-S3 ↔ PCA9685 Servo Driver

| Freenove ESP32-S3 Pin | PCA9685 Pin | Description |
| :--- | :--- | :--- |
| **GPIO 21**  | **SCL** | I2C Clock |
| **GPIO 47**  | **SDA** | I2C Data |
| **3.3V** | **VCC** | Logic Power (3.3V) |
| **5V** | **+5V** | Main Board Power | 
| **GND** | **GND** | Common Ground |


---

## 🚀 Features & Quick Start

* **Live Video Streaming**: Real-time camera feed accessible via web browser.
* **Wireless Control**: Remote gait control over Wi-Fi network.
## 🚀 Quick Start

1. Open **Arduino IDE** and select the board: `ESP32S3 Dev Module`.
2. Open the built-in camera example:  
   `File` > `Examples` > `ESP32` > `Camera` > `CameraWebServer`
3. Replace the `.ino` file in the `CameraWebServer` sketch with the `ESP32CAM_code` provided in this repository.
4. Open the `board_config.h` tab and select/uncomment your specific board model.
5. Configure your Wi-Fi credentials (SSID & Password) inside the code.
6. Flash the code to the ESP32-S3 module, open the Serial Monitor, and check the assigned IP address and port for remote control and live streaming.
7. Enter the IP address and Port displayed in the Serial Monitor (`ESP_IP` and `ESP_PORT`) into your Python script to connect to the video stream:

[!NOTE]

The instructions and code provided here may not be perfect. If you run into any issues or have questions, please feel free to leave a comment on my YouTube video!

🔗https://www.youtube.com/watch?v=QVtJsp4elgw
