# Automated Trash Can Cleaning System - Arduino Code

## Overview

This Arduino code controls an automated system designed to clean a trash can (or a similar container) using a predefined sequence of operations: initial water spray, soap spray, rinsing water spray, sanitizer spray, and finally, drying with fans. The system uses an I2C LCD display for user feedback and float switches to monitor liquid levels.

## Features

* **Automated Cleaning Cycle:** Executes a multi-stage cleaning process with configurable timings.
* **User Interface:** Uses a 16x2 I2C LCD to display status messages, progress, and alerts.
* **Liquid Level Sensing:** Employs float switches to detect low levels of water, soap, and sanitizer, preventing operation with empty tanks.
* **State Machine:** Manages the cleaning process through distinct states (Idle, Spraying, Drying, etc.).
* **Simple Control:** Starts the cleaning cycle with a single button press.
* **Component Control:** Manages pumps for liquids and fans for drying.

## Hardware Requirements

* Arduino Board (e.g., Uno, Nano, Mega)
* 16x2 I2C LCD Display
* Push Button
* 3 x Float Switches (for Water, Soap, Sanitizer levels)
* 3 x Liquid Pumps (for Water, Soap, Sanitizer)
* 2 x Fans
* Relay Module(s) (to safely control the pumps and fans, as Arduino pins cannot directly handle their power requirements)
* Connecting Wires
* Power Supply (appropriate for Arduino, pumps, and fans)
* Tubing and Nozzles (for liquid delivery)
* Containers for Water, Soap, and Sanitizer

## Software Requirements

* Arduino IDE
* `Wire.h` library (usually included with Arduino IDE)
* `LiquidCrystal_I2C.h` library (needs to be installed in the Arduino IDE)

## Pinout Configuration

The code defines the following pin connections:

| Component         | Arduino Pin | Type         | Notes                               |
| :---------------- | :---------- | :----------- | :---------------------------------- |
| Start Button      | 2           | INPUT_PULLUP | Connect button between Pin 2 and GND |
| Water Float Sw.   | 3           | INPUT_PULLUP | Connect switch between Pin 3 and GND |
| Soap Float Sw.    | 4           | INPUT_PULLUP | Connect switch between Pin 4 and GND |
| Sanitizer Float Sw| 5           | INPUT_PULLUP | Connect switch between Pin 5 and GND |
| Water Pump Relay  | 8           | OUTPUT       | Controls the water pump             |
| Soap Pump Relay   | 9           | OUTPUT       | Controls the soap pump              |
| Sanitizer Pump R. | 10          | OUTPUT       | Controls the sanitizer pump         |
| Fan 1 Relay       | 11          | OUTPUT       | Controls Fan 1                      |
| Fan 2 Relay       | 12          | OUTPUT       | Controls Fan 2                      |
| I2C LCD SDA       | A4 (Uno/Nano) | I2C          | Connect to LCD SDA pin              |
| I2C LCD SCL       | A5 (Uno/Nano) | I2C          | Connect to LCD SCL pin              |

**Note:** The code assumes **active-LOW relays**, meaning setting the Arduino pin `LOW` turns the relay (and thus the pump/fan) **ON**, and setting the pin `HIGH` turns it **OFF**. This is common for many relay modules.

## Code Explanation

### 1. Includes and Initialization

* `#include <Wire.h>`: Includes the library for I2C communication, necessary for the LCD.
* `#include <LiquidCrystal_I2C.h>`: Includes the library for controlling the I2C LCD.
* `LiquidCrystal_I2C lcd(0x27, 16, 2);`: Creates an LCD object.
    * `0x27`: The I2C address of the LCD. **This might need adjustment** (common addresses are 0x27 or 0x3F). Check your specific LCD module.
    * `16`: Number of columns on the LCD.
    * `2`: Number of rows on the LCD.

### 2. Constants

* **Pin Definitions (`const int ...Pin`)**: Assigns meaningful names to the Arduino pins connected to hardware components.
* **Time Durations (`const unsigned long ...Time`)**: Defines the duration (in milliseconds) for each step of the cleaning cycle (spraying, drying) and the delay between sprays. These values can be adjusted to change the cycle timing.
    * `1000` milliseconds = 1 second.
    * `600000` milliseconds = 10 minutes.

### 3. Variables

* **`enum CleaningState {...}`**: Defines a set of named states that represent the different stages of the cleaning process (e.g., `IDLE`, `WATER1`, `SOAP`, `DRYING`, `LOW_LIQUID`). Using an enum makes the code more readable than using arbitrary numbers for states.
* **`CleaningState currentCleaningState = IDLE;`**: A variable that holds the current state of the system. It starts in the `IDLE` state.
* **`unsigned long startTime = 0;`**: Stores the time (using `millis()`) when the current state began. This is used to calculate how long the system has been in the current state.

### 4. `setup()` Function

This function runs once when the Arduino starts up:

* `Serial.begin(9600);`: Initializes serial communication (useful for debugging).
* `lcd.init(); lcd.backlight();`: Initializes the LCD and turns on its backlight.
* `displayLoadingAnimation();`: Shows a simple "..." animation on the LCD.
* `displayReadyMessage();`: Displays the initial "Ready to Use" message.
* `pinMode(...)`: Configures each pin as either an `INPUT_PULLUP` (for buttons and float switches, using the internal pull-up resistor) or an `OUTPUT` (for controlling relays).
* `digitalWrite(..., HIGH);`: Sets all output pins (relays) to `HIGH` initially. As per the active-LOW assumption, this ensures all pumps and fans are **OFF** at the start.

### 5. `loop()` Function

This function runs repeatedly after `setup()`:

* **Button Check**: It constantly checks if the `buttonPin` is pressed (`LOW`).
    * **Debounce**: A small `delay(50)` is used to prevent multiple triggers from a single button press (debouncing).
    * **Start Condition**: If the button is pressed *and* the system is currently `IDLE`, it shows a "Preparing" animation and calls `startCleaningCycle()`.
* **State Machine Execution**: It continuously calls `runCleaningCycle()` to manage the active cleaning process based on the `currentCleaningState`.

### 6. Display Functions

* `displayLoadingAnimation()`: Shows "..." on the LCD during startup.
* `displayPrepairingAnimation()`: Shows "Preparing..." when the button is pressed.
* `displayReadyMessage()`: Shows the system is ready.
* `displayLowLiquidMessage()`: Displays a warning indicating which liquid(s) are low.
* `displayCleaningComplete()`: Shows a completion message after the cycle finishes.

### 7. `startCleaningCycle()` Function

* **Liquid Level Check**: Reads the state of the three float switches (`waterFloatPin`, `soapFloatPin`, `sanitizerFloatPin`). A `LOW` reading typically means the liquid level is low (float is down).
* **Low Liquid Handling**: If any float switch reads `LOW`, it sets the state to `LOW_LIQUID`, calls `displayLowLiquidMessage()` to show the warning, and exits the function, preventing the cycle from starting.
* **Start Cycle**: If all liquid levels are sufficient (`HIGH`), it sets the `currentCleaningState` to `WATER1` (the first step), records the `startTime` using `millis()`, and clears the LCD to prepare for status updates.

### 8. `runCleaningCycle()` Function (State Machine Core)

This is the heart of the operation. It uses a `switch` statement to execute different code blocks depending on the value of `currentCleaningState`.

* **Time Calculation**: It calculates `elapsedTime` since the current state began (`currentTime - startTime`).
* **State Actions**: Inside each `case` (state):
    * **LCD Update**: Displays the current action (e.g., "Spraying Water...", "Drying...").
    * **Activate Output**: Turns the relevant pump(s) or fan(s) `ON` by setting their relay pin `LOW`.
    * **Check Duration**: Compares `elapsedTime` with the required duration for that state (e.g., `waterSprayTime1`).
    * **Transition**: If the time is up:
        * Deactivates the output(s) by setting the pin(s) `HIGH`.
        * Changes `currentCleaningState` to the next state in the sequence.
        * Resets `startTime = millis()` to time the *new* state.
* **Specific State Logic**:
    * `WATER1`, `SOAP`, `WATER2`, `SANITIZER`: Control respective pumps.
    * `SOAP_DELAY`, `WATER2_DELAY`, `SANITIZER_DELAY`: Insert pauses between sprays without activating any pump.
    * `DRYING`: Turns fans `ON` (`LOW`), calculates and displays the remaining drying time. When time is up, turns fans `OFF` (`HIGH`), transitions to `COMPLETE`, and displays the completion message.
    * `COMPLETE`: Does nothing active here; the state transition happens in `DRYING` or `displayCleaningComplete`.
    * `LOW_LIQUID`: Calls the function to display the warning message and resets to `IDLE` after a delay.
    * `IDLE`: Does nothing, waiting for the button press in `loop()`.

### 9. `displayCleaningComplete()` Function

* Called when the `DRYING` state finishes.
* Displays "CleaningTrashcan Complete!" on the LCD for 3 seconds.
* Resets the LCD to the `displayReadyMessage()`.
* Sets the `currentCleaningState` back to `IDLE`, ready for the next cycle.

## How It Works (Workflow)

1.  **Power On**: The Arduino initializes, checks the LCD, shows a loading animation, sets pins, ensures outputs are off, and displays the "Ready" message. The system enters the `IDLE` state.
2.  **Button Press**: The user presses the start button.
3.  **Preparation & Check**: A "Preparing..." animation shows. The `startCleaningCycle` function checks the water, soap, and sanitizer float switches.
    * **If Low Liquid**: A warning message appears indicating which liquid is low. The system returns to `IDLE` after a few seconds.
    * **If Liquids OK**: The system proceeds.
4.  **Cleaning Cycle**: The `runCleaningCycle` function takes over, moving through the states based on timers:
    * `WATER1`: Water pump runs for `waterSprayTime1`.
    * `SOAP_DELAY`: Pause for `sprayDelay`.
    * `SOAP`: Soap pump runs for `soapSprayTime`.
    * `WATER2_DELAY`: Pause for `sprayDelay`.
    * `WATER2`: Water pump runs for `waterSprayTime2` (rinse).
    * `SANITIZER_DELAY`: Pause for `sprayDelay`.
    * `SANITIZER`: Sanitizer pump runs for `sanitizerSprayTime`.
    * `DRYING`: Fans run for `fanRunTime`. The LCD shows a countdown.
5.  **Completion**: Once drying is finished:
    * Fans turn off.
    * A "Complete!" message is shown for 3 seconds.
    * The system displays the "Ready" message again.
    * The state returns to `IDLE`.

## Configuration / Customization

* **LCD Address**: Modify `LiquidCrystal_I2C lcd(0x27, 16, 2);` if your LCD uses a different I2C address (like `0x3F`).
* **Timings**: Adjust the `const unsigned long` duration values (e.g., `waterSprayTime1`, `fanRunTime`) to fine-tune the cleaning cycle length.
* **Pins**: If you use different Arduino pins, update the `const int ...Pin` definitions at the beginning of the code.
* **Relay Type**: If using active-HIGH relays, invert the logic for `digitalWrite` calls controlling pumps/fans (`LOW` becomes OFF, `HIGH` becomes ON).
