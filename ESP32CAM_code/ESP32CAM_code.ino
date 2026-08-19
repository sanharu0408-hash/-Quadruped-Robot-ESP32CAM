#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

#include "board_config.h"

// ============================================================
// Wi-Fi
// ============================================================
const char *ssid = "******";
const char *password = "*******";
// ============================================================
// Camera Web Server
// ============================================================

void startCameraServer();
void setupLedFlash();

// ============================================================
// UDP
// ============================================================

WiFiUDP udp;

const int UDP_PORT = "5000";

char udpBuffer[128];

// ============================================================
// PCA9685
// ============================================================

#define SDA_PIN 47
#define SCL_PIN 21

Adafruit_PWMServoDriver pwm =
  Adafruit_PWMServoDriver(0x40);

// ============================================================
// Servo Assignment
// ============================================================

enum ServoIndex {

  FL_KNEE, // 0
  FL_HIP,  // 1

  FR_KNEE, // 2
  FR_HIP,  // 3

  RL_KNEE, // 4
  RL_HIP,  // 5

  RR_KNEE, // 6
  RR_HIP   // 7
};

// ============================================================
// Servo Configuration
// ============================================================

struct ServoConfig {

  String name;

  int ch;

  int minPulse;
  int centerPulse;
  int maxPulse;

  int direction;
};

// ============================================================
// Servo Configurations
// ============================================================

ServoConfig servos[] = {

  // Name        Ch   Min   Center   Max   Dir

  {"FL_KNEE",     0,   145,   330,   520,   1},
  {"FL_HIP",      2,   140,   325,   450,   1},

  {"FR_KNEE",     4,   140,   320,   520,  -1},
  {"FR_HIP",      6,   275,   300,   485,  -1},

  {"RL_KNEE",     8,   200,   310,   500,  -1},
  {"RL_HIP",     10,   280,   305,   495,  -1},

  {"RR_KNEE",    12,   100,   300,   400,   1},
  {"RR_HIP",     14,   120,   300,   325,   1}
};

const int SERVO_COUNT = 8;

// ============================================================
// Robot State
// ============================================================

int mode = 0;
// 0 = Idle
// 2 = Walking

float waveAngle = 0;

float stick_Y = 0.0;
float stick_X = 0.0;

// ============================================================
// Servo Control
// ============================================================

void moveServo(
  int index,
  float ratio
) {

  ServoConfig s = servos[index];

  // Apply servo direction
  float final_ratio =
    ratio * s.direction;

  float p_range =
    (final_ratio >= 0)
      ? (s.maxPulse - s.centerPulse)
      : (s.centerPulse - s.minPulse);

  float targetPulse =
    s.centerPulse +
    (final_ratio * p_range);

  int safePulse =
    constrain(
      (int)targetPulse,
      s.minPulse,
      s.maxPulse
    );

  pwm.setPWM(
    s.ch,
    0,
    safePulse
  );
}

// ============================================================
// Reset servos to center
// ============================================================

void resetToCenter() {

  for (
    int i = 0;
    i < SERVO_COUNT;
    i++
  ) {

    pwm.setPWM(
      servos[i].ch,
      0,
      servos[i].centerPulse
    );
  }
}

// ============================================================
// Squat Pose
// ============================================================

void setSquatPose() {

  moveServo(
    FL_KNEE,
    -1.4
  );

  moveServo(
    FR_KNEE,
    -1.4
  );

  moveServo(
    RL_KNEE,
    -1.4
  );

  moveServo(
    RR_KNEE,
    -1.4
  );

  moveServo(
    FL_HIP,
    0.0
  );

  moveServo(
    FR_HIP,
    0.0
  );

  moveServo(
    RL_HIP,
    0.0
  );

  moveServo(
    RR_HIP,
    0.0
  );
}

// ============================================================
// Jump
// ============================================================

void executeJump() {

  setSquatPose();

  delay(150);

  moveServo(
    FL_KNEE,
    1.0
  );

  moveServo(
    FR_KNEE,
    1.0
  );

  delay(35);

  moveServo(
    RL_KNEE,
    1.0
  );

  moveServo(
    RR_KNEE,
    1.0
  );

  delay(150);

  moveServo(FL_KNEE, 0.0);
  moveServo(FR_KNEE, 0.0);

  moveServo(RL_KNEE, 0.0);
  moveServo(RR_KNEE, 0.0);

  delay(80);

  setSquatPose();

  delay(200);
}

// ============================================================
// Kick
// ============================================================

void kick() {

  setSquatPose();

  delay(150);

  moveServo(
    FL_HIP,
    -1.0
  );

  delay(150);

  moveServo(
    FL_KNEE,
    0.0
  );

  delay(200);

  setSquatPose();

  delay(200);
}

// ============================================================
// Wave Hand
// ============================================================

void executeWaveHand() {

  moveServo(
    FR_KNEE,
    -1.4
  );

  moveServo(
    RL_KNEE,
    -1.4
  );

  moveServo(
    RR_HIP,
    -0.6
  );

  moveServo(
    RR_KNEE,
    -2.5
  );

  moveServo(
    FR_HIP,
    -0.3
  );

  moveServo(
    RL_HIP,
    0.2
  );

  delay(250);

  moveServo(
    FL_KNEE,
    0.8
  );

  moveServo(
    RR_KNEE,
    -0.4
  );

  moveServo(
    FL_HIP,
    -1.0
  );

  delay(200);

  for (
    int i = 0;
    i < 3;
    i++
  ) {

    moveServo(
      FL_HIP,
      -0.5
    );

    delay(150);

    moveServo(
      FL_HIP,
      -1.3
    );

    delay(150);
  }

  setSquatPose();

  delay(200);
}

// ============================================================
// UDP Command Handler
// ============================================================

void handleUDP() {

  int packetSize =
    udp.parsePacket();

  if (packetSize <= 0) {
    return;
  }

  int len =
    udp.read(
      udpBuffer,
      sizeof(udpBuffer) - 1
    );

  if (len <= 0) {
    return;
  }

  udpBuffer[len] = '\0';

  String input =
    String(udpBuffer);

  input.trim();

  Serial.print("[UDP] ");
  Serial.println(input);

  // ==========================================================
  // Walking
  // ==========================================================

  if (input.equalsIgnoreCase("w")) {

    mode =
      (mode == 2)
        ? 0
        : 2;

    stick_Y = 0.0;
    stick_X = 0.0;

    if (mode == 0) {

      setSquatPose();
    }

    Serial.print(
      "[MODE] "
    );

    if (mode == 2) {
      Serial.println("WALKING");
    } else {
      Serial.println("IDLE");
    }
  }

  // ==========================================================
  // Jump
  // ==========================================================

  else if (
    input.equalsIgnoreCase("j")
  ) {

    Serial.println(
      "[ACTION] JUMP"
    );

    executeJump();
  }

  // ==========================================================
  // Wave Hand
  // ==========================================================

  else if (
    input.equalsIgnoreCase("h")
  ) {

    Serial.println(
      "[ACTION] WAVE"
    );

    executeWaveHand();
  }

  // ==========================================================
  // Kick
  // ==========================================================

  else if (
    input.equalsIgnoreCase("b")
  ) {

    Serial.println(
      "[ACTION] KICK"
    );

    kick();
  }

  // ==========================================================
  // Stick
  //
  // Python:
  //
  // f2.50,r-1.20
  // ==========================================================

  else if (
    input.startsWith("f") &&
    input.indexOf(",r") != -1
  ) {

    int rIndex =
      input.indexOf(",r");

    String fStr =
      input.substring(
        1,
        rIndex
      );

    String rStr =
      input.substring(
        rIndex + 2
      );

    stick_Y =
      fStr.toFloat();

    stick_X =
      rStr.toFloat();

    Serial.printf(
      "[STICK] Forward: %+0.2f | Turn: %+0.2f\n",
      stick_Y,
      stick_X
    );
  }
}

// ============================================================
// Setup
// ============================================================

void setup() {

  // ==========================================================
  // Serial
  // ==========================================================

  Serial.begin(115200);

  Serial.setDebugOutput(true);

  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    " ESP32-S3 Camera + Robot Controller "
  );

  Serial.println(
    "======================================"
  );

  // ==========================================================
  // I2C / PCA9685
  // ==========================================================

  Wire.end();

  pinMode(
    SDA_PIN,
    INPUT
  );

  pinMode(
    SCL_PIN,
    INPUT
  );

  delay(50);

  Wire.begin(
    SDA_PIN,
    SCL_PIN,
    100000
  );

  Wire.setTimeOut(25);

  // ==========================================================
  // PCA9685
  // ==========================================================

  pwm.begin();

  pwm.setOscillatorFrequency(
    25000000
  );

  pwm.setPWMFreq(50);

  // ==========================================================
  // Initial Servo Position
  // ==========================================================

  resetToCenter();

  // ==========================================================
  // Camera Configuration
  // ==========================================================

  camera_config_t config;

  config.ledc_channel =
    LEDC_CHANNEL_0;

  config.ledc_timer =
    LEDC_TIMER_0;

  config.pin_d0 =
    Y2_GPIO_NUM;

  config.pin_d1 =
    Y3_GPIO_NUM;

  config.pin_d2 =
    Y4_GPIO_NUM;

  config.pin_d3 =
    Y5_GPIO_NUM;

  config.pin_d4 =
    Y6_GPIO_NUM;

  config.pin_d5 =
    Y7_GPIO_NUM;

  config.pin_d6 =
    Y8_GPIO_NUM;

  config.pin_d7 =
    Y9_GPIO_NUM;

  config.pin_xclk =
    XCLK_GPIO_NUM;

  config.pin_pclk =
    PCLK_GPIO_NUM;

  config.pin_vsync =
    VSYNC_GPIO_NUM;

  config.pin_href =
    HREF_GPIO_NUM;

  config.pin_sccb_sda =
    SIOD_GPIO_NUM;

  config.pin_sccb_scl =
    SIOC_GPIO_NUM;

  config.pin_pwdn =
    PWDN_GPIO_NUM;

  config.pin_reset =
    RESET_GPIO_NUM;

  config.xclk_freq_hz =
    20000000;

  config.frame_size =
    FRAMESIZE_UXGA;

  config.pixel_format =
    PIXFORMAT_JPEG;

  config.grab_mode =
    CAMERA_GRAB_WHEN_EMPTY;

  config.fb_location =
    CAMERA_FB_IN_PSRAM;

  config.jpeg_quality =
    12;

  config.fb_count =
    1;

  // ==========================================================
  // PSRAM
  // ==========================================================

  if (
    config.pixel_format ==
    PIXFORMAT_JPEG
  ) {

    if (psramFound()) {

      config.jpeg_quality =
        10;

      config.fb_count =
        2;

      config.grab_mode =
        CAMERA_GRAB_LATEST;

    } else {

      config.frame_size =
        FRAMESIZE_SVGA;

      config.fb_location =
        CAMERA_FB_IN_DRAM;
    }

  } else {

    config.frame_size =
      FRAMESIZE_240X240;

#if CONFIG_IDF_TARGET_ESP32S3

    config.fb_count = 2;

#endif
  }

  // ==========================================================
  // Camera Init
  // ==========================================================

  esp_err_t err =
    esp_camera_init(
      &config
    );

  if (err != ESP_OK) {

    Serial.printf(
      "Camera init failed with error 0x%x\n",
      err
    );

    return;
  }

  Serial.println(
    "Camera initialized."
  );

  // ==========================================================
  // Camera Sensor
  // ==========================================================

  sensor_t *s =
    esp_camera_sensor_get();

  if (
    s->id.PID ==
    OV3660_PID
  ) {

    s->set_vflip(
      s,
      1
    );

    s->set_brightness(
      s,
      1
    );

    s->set_saturation(
      s,
      -2
    );
  }

  // ==========================================================
  // Initial Frame Size
  // ==========================================================

  if (
    config.pixel_format ==
    PIXFORMAT_JPEG
  ) {

    s->set_framesize(
      s,
      FRAMESIZE_QVGA
    );
  }

  // ==========================================================
  // M5Stack
  // ==========================================================

#if defined(CAMERA_MODEL_M5STACK_WIDE) || \
    defined(CAMERA_MODEL_M5STACK_ESP32CAM)

  s->set_vflip(
    s,
    1
  );

  s->set_hmirror(
    s,
    1
  );

#endif

  // ==========================================================
  // ESP32-S3-EYE
  // ==========================================================

#if defined(CAMERA_MODEL_ESP32S3_EYE)

  s->set_vflip(
    s,
    1
  );

#endif

  // ==========================================================
  // LED
  // ==========================================================

#if defined(LED_GPIO_NUM)

  setupLedFlash();

#endif

  // ==========================================================
  // Wi-Fi
  // ==========================================================

  WiFi.begin(
    ssid,
    password
  );

  WiFi.setSleep(false);

  Serial.print(
    "WiFi connecting"
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "WiFi connected."
  );

  // ==========================================================
  // UDP Server
  // ==========================================================

  udp.begin(
    UDP_PORT
  );

  Serial.printf(
    "UDP server started on port %d\n",
    UDP_PORT
  );

  // ==========================================================
  // Camera Web Server
  // ==========================================================

  startCameraServer();

  // ==========================================================
  // Information
  // ==========================================================

  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    "Camera + Robot Ready!"
  );

  Serial.print(
    "Web:    http://"
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "Stream: http://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    ":81/stream"
  );

  Serial.print(
    "UDP:    "
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.print(
    ":"
  );

  Serial.println(
    UDP_PORT
  );

  Serial.println(
    "======================================"
  );

  Serial.println();

  // 起動時はしゃがみ姿勢
  setSquatPose();
}

// ============================================================
// Main Loop
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // UDP command
  // ----------------------------------------------------------

  handleUDP();

  // ----------------------------------------------------------
  // MODE 2: WALKING
  // ----------------------------------------------------------

  if (mode == 2) {

    if (
      abs(stick_Y) > 0.05 ||
      abs(stick_X) > 0.05
    ) {

      if (
        abs(stick_Y) > 0.05
      ) {

        if (stick_Y > 0) {

          waveAngle +=
            (
              0.04 +
              stick_Y * 0.04
            );

        } else {

          waveAngle -=
            (
              0.04 +
              abs(stick_Y) * 0.04
            );
        }

      } else {

        waveAngle +=
          0.04 +
          (2.5 * 0.04);
      }
    }

    // ========================================================
    // Phase
    // ========================================================

    float phaseFL =
      waveAngle;

    float phaseFR =
      waveAngle + PI;

    float phaseRL =
      waveAngle + PI;

    float phaseRR =
      waveAngle;

    float hip_phaseFL =
      phaseFL;

    float hip_phaseFR =
      phaseFR;

    float hip_phaseRL =
      phaseRL + PI;

    float hip_phaseRR =
      phaseRR + PI;

    // ========================================================
    // Knee
    // ========================================================

    float fl_knee;
    float fr_knee;
    float rl_knee;
    float rr_knee;

    if (
      abs(stick_Y) > 0.05 ||
      abs(stick_X) > 0.05
    ) {

      fl_knee =
        (sin(phaseFL) - 3)
        / 2.0 * 0.7;

      fr_knee =
        (sin(phaseFR) - 3)
        / 2.0 * 0.7;

      rl_knee =
        (sin(phaseRL) - 3)
        / 2.0 * 0.7;

      rr_knee =
        (sin(phaseRR) - 3)
        / 2.0 * 0.7;

    } else {

      fl_knee = -1.4;
      fr_knee = -1.4;
      rl_knee = -1.4;
      rr_knee = -1.4;
    }

    // ========================================================
    // Walking multiplier
    // ========================================================

    float base_Y =
      abs(stick_Y);

    float left_multiplier;
    float right_multiplier;

    if (stick_Y >= 0) {

      left_multiplier =
        base_Y +
        (stick_X * 1);

      right_multiplier =
        base_Y -
        (stick_X * 1);

    } else {

      left_multiplier =
        base_Y -
        (stick_X * 1);

      right_multiplier =
        base_Y +
        (stick_X * 1);
    }

    left_multiplier =
      constrain(
        left_multiplier,
        0.0,
        5.0
      );

    right_multiplier =
      constrain(
        right_multiplier,
        0.0,
        5.0
      );

    // ========================================================
    // Hip
    // ========================================================

    float fl_hip =
      cos(hip_phaseFL) *
      0.6 *
      (left_multiplier / 5.0);

    float fr_hip =
      cos(hip_phaseFR) *
      0.6 *
      (right_multiplier / 5.0);

    float rl_hip =
      cos(hip_phaseRL) *
      0.6 *
      (left_multiplier / 5.0);

    float rr_hip =
      cos(hip_phaseRR) *
      0.6 *
      (right_multiplier / 5.0);

    // ========================================================
    // Move all servos
    // ========================================================

    moveServo(
      FL_KNEE,
      fl_knee
    );

    moveServo(
      FL_HIP,
      fl_hip
    );

    moveServo(
      FR_KNEE,
      fr_knee
    );

    moveServo(
      FR_HIP,
      fr_hip
    );

    moveServo(
      RL_KNEE,
      rl_knee
    );

    moveServo(
      RL_HIP,
      rl_hip
    );

    moveServo(
      RR_KNEE,
      rr_knee
    );

    moveServo(
      RR_HIP,
      rr_hip
    );

    delay(20);
  }

  // ==========================================================
  // Idle
  // ==========================================================

  else {

    // UDPの受信処理だけ行う
    // サーボは現在位置を維持
    delay(1);
  }
}
