import sys
import pygame
import socket
import webbrowser

# ============================================================
# Configuration
# ============================================================

ESP_IP = "********"

ESP_PORT = 5000

# ESP32 Camera WebServer
CAMERA_WEB_URL = f"http://{ESP_IP}/"

DEADZONE = 0.2
LOOP_HZ = 15

# ============================================================
# Button Mapping
# ============================================================

BTN_A = 0
BTN_B = 1
BTN_X = 2
BTN_Y = 3

# ============================================================
# Initialize Pygame
# ============================================================

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:

    print("[Error] No controller connected.")

    pygame.quit()
    sys.exit()

controller = pygame.joystick.Joystick(0)
controller.init()

print(f"Controller: {controller.get_name()}")

# ============================================================
# Initialize UDP
# ============================================================

print(
    f"Connecting to ESP32 at "
    f"{ESP_IP}:{ESP_PORT}..."
)

sock = socket.socket(
    socket.AF_INET,
    socket.SOCK_DGRAM
)

print("Wi-Fi communication ready!")

# ============================================================
# Open ESP32 Camera WebServer
# ============================================================

print()
print("Opening ESP32 Camera WebServer:")
print(CAMERA_WEB_URL)

webbrowser.open(CAMERA_WEB_URL)

# ============================================================
# Status
# ============================================================

status_msg = "Idle"

print()
print("--- Operation Guide ---")
print(" B Button: Toggle Walking Mode (w)")
print(" A Button: Kick (b)")
print(" X Button: Jump (j)")
print(" Y Button: Wave Hand (h)")
print(" ESC / Ctrl+C: Exit")
print("------------------------")
print()

clock = pygame.time.Clock()

# ============================================================
# Main Loop
# ============================================================

try:

    running = True

    while running:

        clock.tick(LOOP_HZ)

        # ----------------------------------------------------
        # Pygame Events
        # ----------------------------------------------------

        for event in pygame.event.get():

            if event.type == pygame.QUIT:

                running = False

            elif event.type == pygame.JOYBUTTONDOWN:

                # --------------------------------------------
                # B = Walking
                # --------------------------------------------

                if event.button == BTN_B:

                    sock.sendto(
                        b"w\n",
                        (ESP_IP, ESP_PORT)
                    )

                    status_msg = \
                        "Sent: Walking Mode Toggle"

                # --------------------------------------------
                # A = Kick
                # --------------------------------------------

                elif event.button == BTN_A:

                    sock.sendto(
                        b"b\n",
                        (ESP_IP, ESP_PORT)
                    )

                    status_msg = \
                        "Sent: Kick"

                # --------------------------------------------
                # X = Jump
                # --------------------------------------------

                elif event.button == BTN_X:

                    sock.sendto(
                        b"j\n",
                        (ESP_IP, ESP_PORT)
                    )

                    status_msg = \
                        "Sent: Jump"

                # --------------------------------------------
                # Y = Wave
                # --------------------------------------------

                elif event.button == BTN_Y:

                    sock.sendto(
                        b"h\n",
                        (ESP_IP, ESP_PORT)
                    )

                    status_msg = \
                        "Sent: Wave"

        # ----------------------------------------------------
        # Stick
        # ----------------------------------------------------

        raw_Y = -controller.get_axis(1)
        raw_X = controller.get_axis(2)

        # Deadzone

        if abs(raw_Y) > DEADZONE:

            stick_Y = round(
                raw_Y * 5.0,
                2
            )

        else:

            stick_Y = 0.0

        if abs(raw_X) > DEADZONE:

            stick_X = round(
                raw_X * 5.0,
                2
            )

        else:

            stick_X = 0.0

        # ----------------------------------------------------
        # Send Stick Data
        # ----------------------------------------------------

        msg = (
            f"f{stick_Y:.2f},"
            f"r{stick_X:.2f}\n"
        )

        sock.sendto(
            msg.encode(),
            (ESP_IP, ESP_PORT)
        )

        # ----------------------------------------------------
        # Status
        # ----------------------------------------------------

        print(
            f"\r[Status] "
            f"{status_msg:<30} | "
            f"[Stick] "
            f"Forward:{stick_Y:+.2f} "
            f"Turn:{stick_X:+.2f}",
            end=""
        )

except KeyboardInterrupt:

    print("\n\nShutting down...")

finally:

    sock.close()

    pygame.quit()

    print(
        "\nCleaned up resources. Goodbye."
    )
