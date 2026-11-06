# Gyro Mouse Base: The Official Build Guide

This guide details the physical assembly, wiring, and software tuning required to construct the Gyro Mouse Base platform. It isolates all tilt mechanics to the desk surface so you can use your preferred optical mouse without any destructive modification.

Note: A mouse pad is optional for this platform, but it is highly recommended to at least use a wrist rest.

---

## 1. Step-by-Step Assembly

The build order ensures clean mechanical alignment and prevents accidental short circuits or misplacement. If you haven't already procured all parts from the bill of materials, go to [the design](DESIGN.md) and return once you have everything you need.

1. Trace and cut two 130mm circles from 0.55mm Galvabond steel sheet, deburr and file the edges smooth, and epoxy them together. Laminating two sheets creates an ultra-rigid 1.1mm plate that is 4x stiffer than 1mm aluminum.
2. Cut a matching 130mm circle of non-slip drawer liner and adhere it to the top metal surface. This high-friction mat prevents the mouse from sliding during steep tilts. If you are not using a mouse pad for this platform then cut a second circle.
3. Cut two matching 130mm circles from a cereal box and glue the gloss sides together. This will be used as an area under the base, holding the pivot in position and preventing the rough ends of the springs from scratching any surface. Glue the M5 metal washers to the dead center of one side for the pivot to rest on. If you are not using a mouse pad for this platform then glue the non-slip liner to the other side.
4. Flip the metal plate over. Mark a circle 20mm from the edge which will be used later. Also mark every 45 degrees around this circle.
5. Glue the M4 Dome Nut dead-center on the underside using epoxy or cyanoacrylate (super glue). This establishes a 7–8mm center pivot. Glue the springs and foam forming a foam-spring-foam-spring circle. The springs will push against your hand to return the plate to a level state. The foam is used so the plate does not wobble or require extra effort to get level. The springs will normally have one end flattened where the rings are close together. This is the end that should be glued to the metal plate.
6. The sensor is the main component of the build to measure the tilt angle, while the touch sensor or clutch is used to enable/disable tilt control.

    1. **Insulation:** Apply a small strip of electrical or Kapton tape to the metal plate where the MPU-6050 will sit to insulate its through-hole solder pins from the steel.
    2. **Orientation:** Align the left edge of the MPU-6050 module toward the **front** of the base on the left side. A mouse is generally oval in footprint, so placing the sensor on the left will leave it away from the mouse. The left edge facing toward the front allows the pins to trail off the front of the base.
    3. **Mounting:** Remove a section of the non-slip liner where the sensor will go. Use **super glue (Cyanoacrylate)** to fix the sensor in place. This will ensure a heavily rigid mount while ensuring there is no damage to the sensor board.
    4. **Wiring:** Connect the 4-wire flexible ribbon cable to the ESP32-S3 (3.3V, GND, SDA $\rightarrow$ GPIO 4, SCL $\rightarrow$ GPIO 5).
    5. **Touch pad preparation**: Take a piece of foil about 20mm wide and fold it in half repeatedly until it is about square. Make 3 of these shapes.
    6. **Touch pad wiring**: Read this instruction fully before performing. Cut a 200mm DuPont connector down to 150mm and strip about 10mm from the end. Splay the bare wire partially open on one side of a foil piece, then glue the wire using super glue and fold the other side over. You should have a 5x10mm square of foil with the wire now glued inside. Repeat to make 3 pads.
    7. **Final connections:** Connect the pads to GPIO pins 1, 2 an 6. Use craft glue to attach:
        * The GPIO1 pad to the end of the left mouse button
        * The GPIO2 pad to the end of the right mouse button
        * The GPIO6 pad to be comfortably under the thumb
7. [Optional]: Remove the onboard blue LED that notes if an attached LiPo battery is charging/charged. This LED will continue to flash otherwise.
8. Flash the device. A build script has been included for Linux to generate the required firmware and flash the chip using `./build.sh flash`.

---

## 2. Tuning

After assembly comes tuning the pad. While the defaults may be suitable, it is likely that the touch threshold may need calibrating, the speed need adjusting or the dead zone be too narrow. Use the CLI reference below to adjust the settings to your preference.

### 2.1 Touch Pad Calibration

The touch sensors work by applying a small charge to the pad and measuring how long this pad takes to reach a threshold. When tuning the touch sensitivity, the diagnostic output may stop short of the full range of the pad. For example, if the threshold is set to 40,000 which is a very gentle touch, the diagnostic may show 41,342. Adjusting the range up again to 50,000 and touching again with the same pressure may yield 52,007. Continue to adjust the threshold up/down until you feel comfortable.

### 2.2 Cursor Speed

The cursor speed should be set based on fine movement. If you can drag over a line of text comfortably then the speed is reasonable. The device supports a double tap action to double the speed for moving between windows or across screens, so calibrate for fine movement and use the double tap for coarse and fast moves.

---

## 3. Interactive Serial CLI & Tuning Reference

Connect to the live CLI using `./build.sh monitor`. All settings can be adjusted on the fly:

### CLI Commands Reference

| Command | Action | Range / Step | Description |
| :---: | :--- | :---: | :--- |
| `d+` / `d-` | **Deadzone** | $0.05^\circ - 1.50^\circ$ ($\pm 0.05^\circ$) | Adjust resting deadzone threshold ($0.35^\circ$ default). |
| `t+` / `t-` | **Max Tilt Angle** | $0.80^\circ - 6.50^\circ$ ($\pm 0.05^\circ$) | Adjust max tilt angle required for top speed ($3.50^\circ$ default). |
| `s+` / `s-` | **Cursor Speed** | $100 - 6000$ ($\pm 50$) | Adjust maximum cursor velocity in pixels/second (1000 px/sec default). |
| `u+` / `u-` | **Touch Threshold** | $25000 - 300000$ ($\pm 5000$) | Adjust capacitive touch thresholds (75% linked hysteresis, 150000 default). |
| `++` | **Save to Flash** | — | Save current tuning configuration to NVS memory. |
| `--` | **Factory Reset** | — | Reset all configuration settings back to defaults in flash. |
| `i` | **Motion Telemetry** | — | Toggle real-time angle and cursor velocity stream ON/OFF. |
| `j` | **Touch Events** | — | Toggle live touch event logging on button/clutch press/release ON/OFF. |
| `z` | **Zero / Tare** | — | Calibrate current resting angles as $0^\circ$ baseline in dynamic RAM (or triple-tap clutch). |
| `c` | **Clutch Override** | — | Toggle clutch mode (Always Move vs Capacitive Touch Required). |
| `b` | **Touch Buttons 4/5** | — | Toggle Touch Buttons 4 and 5 active (Disable when GPIO 1/2 are floating). |
| `x` | **Invert Roll** | — | Toggle Roll (X-Axis) cursor movement direction inversion. |
| `y` | **Invert Pitch** | — | Toggle Pitch (Y-Axis) cursor movement direction inversion. |
| `P` | **Programming Mode** | — | Software trigger to reboot ESP32-S3 into ROM bootloader for flashing. |
| `h` / `?` | **Help Menu** | — | Display interactive help menu and current NVS configuration summary. |
| `q` / `Ctrl+C` | **Quit Session** | — | Close serial session and return cleanly to terminal shell. |

### Touch Clutch Gestures

* **Touch & Hold:** Normal 1x cursor velocity control (Status LED: Red).
* **Double-Tap & Hold:** Momentary 2x sprint speed ($2000\text{ px/sec}$) for fast navigation across multi-monitor setups (Status LED: Blue). Releasing thumb drops back to 1x.
* **Triple-Tap:** Calibrates current resting angles as $0^\circ$ baseline in dynamic RAM (Status LED: Green flash).
