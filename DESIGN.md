# Gyro Mouse Base: Modular Gyro-Platform Specifications

## 1. Concept Overview

A modular, zero-fatigue, analog flight-stick style platform that upgrades any standard optical mouse into a tilt-to-move device without requiring destructive modding. The optical mouse rests stationary on the platform, while an internal gyroscope calculates pitch and roll to drive cursor velocity. The intention is to reduce wrist fatigue by tilting the mouse to move the pointer rather than sliding the mouse itself.

## 2. Bill of Materials (BOM)

| Component | Specific Item / Location | Project Role |
| :--- | :--- | :--- |
| **Base Plate** | 2 x 0.55mm Galvabond Steel Sheet (Bunnings). Cut to 130mm circles. | Two laminated sheets epoxied together to form an ultra-rigid 1.1mm base disc (4x stiffer than 1mm aluminum). |
| **Pivot Node** | M4 Dome Nut (eBay / Fasteners). Height: 7–8mm. | Center pivot zero-point glued dead-center underneath the base. |
| **Surface Grip** | Non-Slip Drawer Liner (Bunnings). Cut to a 130mm circle. | Textured high-friction mat on top to keep mouse securely in place during tilt. |
| **Suspension Spring** | Springs removed from a ball point pen. Either 4 x 10mm or 2 x 20mm cut in half. | Return-to-center springs inset 20mm from outer edge. |
| **Suspension Dampening** | Moroday Open Cell Weather Seal (Bunnings). 8mm uncompressed thickness. | 4 squares to dampen the springs, also inset 20mm from outer edge. |
| **Support Plate** | 2 x 130mm circles of cardboard from a cereal box. | Circles glued together to keep plate steady and prevent scratching of desk surface. |
| **Pivot Anchor** | 2 x M5 washers (Bunnings). Glued together for height. | Fixed to the centre of the support plate to anchor the pivot. |
| **Microcontroller** | ESP32 SuperMini-S3 (USB-C). | USB mouse and USB serial configuration. Handles sensor readings, calibration and velocity mapping. |
| **Tilt Sensor** | MPU-6050 6-Axis IMU Module. | Fixed to top left of base plate via super glue to detect pitch/roll. |
| **Clutch Sensor** | Kitchen Aluminium Foil folded into shape | Placed on mouse thumb rest and connected to GPIO 6. Enables/disables cursor movement. |
| **Touch Buttons** | Kitchen Aluminium Foil folded into shape | Placed over mouse left/right buttons and connected to GPIO 1 (Btn 4 / Back) and GPIO 2 (Btn 5 / Forward). |
| **Adhesives & Insulation** | Cyanoacrylate (Super Glue) & Kapton / Electrical Tape. | Secures MPU-6050 to the left side and insulates solder pins from the steel plate. |
| **Connectors** | DuPont female-to-female connectors. 4 x 100mm and 3 x 200mm. | Controller to Gyroscope connection and touch connections. |

**Tips**:

* The suspension springs and dampening should be rather soft. Using stiffer materials will place more strain on your wrist.
* The springs I used were about 20mm long and cut in half. You want about 6 coils of spring so that the compressed height is $\approx2 \text{mm}$.
* The initial design called for a foam ring instead of a foam and spring combination. While this will work, the foam will eventually warp regardless of configuration. The spring and foam design should limit the rate of warping but not eliminate it.

## 3. Z-Axis Stack & Mechanical Physics

* **Platform Diameter:** 130mm (optimally sized for ergonomic mice).
* **Pivot Height:** 7–8mm (Standard M4 Dome Nut).
* **Maximum Theoretical Tilt Angle:** $\approx 6.8^\circ - 7.0^\circ$ (Measured physical travel: Pitch $13.87^\circ$ total [$\pm 6.94^\circ$], Roll $13.66^\circ$ total [$\pm 6.83^\circ$]).
* **Total Height Added:** $\approx 10\text{mm}$ (thinner than most standard gel wrist rests).
* **Suspension Foam Preload:** The 8mm uncompressed foam squares are compressed by $\approx 1\text{mm}$ at rest under the 7mm pivot, eliminating slop and establishing a firm neutral center.
* **Suspension Spring Preload:** The 10mm uncompressed springs are compressed by $\approx 3\text{mm}$ at rest under the 7mm pivot, eliminating slop and establishing a firm neutral center.

## 4. Suspension Calculation & Geometry

To ensure the plate reaches full mechanical tilt to the desk without the foam acting as a premature hard stop:

* **Uncompressed Foam:** 8mm height.
* **Uncompressed Spring:** 10mm height.
* **Linear Slope Equation:** $\text{Gap}(r) = 7 - (7 / 65) \cdot r$
* **Foam Solid Height Limit:** $\approx 2\text{mm}$ (75% compression limit of open-cell foam).
* **Foam Compression Threshold:** At $r = 46.4\text{mm}$, the gap equals the 2mm solid height limit.
* **Spring Compression Threshold:** With $\approx 6\text{ coils}$ of $\approx 0.35\text{mm}$ wire ($\approx 2.1\text{mm}$ solid height), the gap reaches solid height at $r = 45.5\text{mm}$, preventing coil-binding before full tilt.
* **Placement Conclusion:** Insetting the foam and springs by **20mm from the outer edge** (outer radius = 45mm) guarantees full tilt range with zero binding.

## 5. Software Architecture & Motion Engine

* **Deadzone Filter:** Defaults to $0.35^\circ$, filtering out micro-tremors and pulse vibrations.
* **Linear Motion Mapping:** Natural wrist tilting spans $\approx 1.2^\circ$. Active motion is directly and linearly normalized: $\text{Speed (px/sec)} = \frac{|\theta| - \text{deadzone}}{\text{maxTilt} - \text{deadzone}} \times \text{maxSpeed}$. Output per 10ms frame (100 Hz fixed loop rate) is $\frac{\text{Speed}}{100}$.
* **Capacitive Touch Clutch:** Single wire connected to **GPIO 6** on the ESP32-S3. Lifting the thumb instantly freezes the cursor for jitter-free clicking.
* **Settling Delay:** A 50ms delay prevents cursor jerk from foam rebound when re-engaging the touch clutch.
* **Touch Clutch Gestures:** Single tap and hold engages normal 1x velocity mapping. Double-tap and hold engages momentary 2x sprint speed ($2000\text{ px/sec}$) for traversing wide or multi-monitor screens. Releasing the thumb instantly disengages sprint and freezes the cursor.
* **On-demand Tare:** The resting angle baseline is calibrated on boot and on demand via the serial `z` command or a capacitive touch triple-tap.
* **Interactive 2-Character Serial CLI:** Built-in command interpreter over native USB CDC (`./build.sh monitor`) for live tuning (`d+`/`d-`, `t+`/`t-`, `s+`/`s-`, `u+`/`u-`, `++`, `--`, `i`, `j`, `z`, `c`, `b`, `x`, `y`, `P`, `q`).

## 6. Wiring & Sensor Orientation

```plain
                       +-----------------------+
                       |    HOST COMPUTER      |
                       +-----------+-----------+
                                   | USB Cable
                                   v
                       +-----------------------+
                       |  ESP32 SuperMini-S3   |
                       +---+---+---+---+---+---+
          GPIO 4 (SDA) |   |   |   |   |   | GPIO 1 (Touch Btn 4 - Back)
          GPIO 5 (SCL) |   |   |   |   |   | GPIO 2 (Touch Btn 5 - Forward)
                       |  3.3V |  GND  |   | GPIO 6 (Touch Clutch)
                       v   v   v   v   v   v
         +---------------------------+   +---------------------------------+
         |    MPU-6050 (Top Left)    |   | Foil Touch Sensors:             |
         | Left Edge Aligned to Front|   | • GPIO 1: Back Touch (Btn 4)    |
         |   Glued with Super Glue   |   | • GPIO 2: Forward Touch (Btn 5) |
         +---------------------------+   | • GPIO 6: Thumb Clutch          |
                                         +---------------------------------+
```

* **Sensor Position:** Top surface of the plate, left side.
* **Sensor Orientation:** Left edge of the MPU-6050 PCB points toward the **front** of the base, mounted on the left side.
* **Insulation:** Header pins insulated from the metal base with electrical/Kapton tape before super gluing.
* **Clutch Wire:** Flexible wire from thumb rest aluminium foil pad to **GPIO 6**.
* **Button Wires:** Flexible wires from left/right button aluminium foil pads to **GPIO 1** (Button 4 / Back) and **GPIO 2** (Button 5 / Forward).

## 7. Gyroscope Sensor Location

The gyroscope has been mounted on the left side of the base with the left edge facing to the front:

* Pins can trail off the front of the board easily
* A mouse usually has an oval footprint, so the left side will place the sensor outside the mouse footprint
