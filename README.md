# arduino-433
Arduino/ESP based 433MHz home control transceiver

## Introduction
This Arduino code allows you to create a cheap 433MHz wireless transceiver to control electric switches and other home appliances. It can use USB or WiFi to connect to a computer to receive and send commands. It is mainly used for the [homebridge-433-arduino](https://github.com/normen/homebridge-433-arduino) plugin but can of course be used otherwise as well.

The project uses the PlatformIO IDE and runs on Arduino (Micro/Nano etc.) as well as ESP8266. To decode signals rc-switch or ESPiLight can be used. Simple 433MHz receiver/sender hardware or more advanced CC1101 based transceiver modules are supported to receive and send 433MHz RC data.

This is not meant to be an advanced firmware but a hobby software that is simple to use, build and understand.

## Installation 
### PlatformIO
- Download and install the PlatformIO IDE, i.e. VisualStudioCode (see https://platformio.org)
- Clone or download this github repository, e.g. `git clone https://github.com/normen/arduino-433`
- Open the project folder in PlatformIO
- Configure the platformio.ini build options if needed (see below)
- Configure the code as needed (see below)
- Connect the desired microcontroller and transceiver hardware (see below)
- Build and upload the project to your Arduino or ESP

#### Arduino IDE
Alternatively you can copy-paste the code in `src/main.cpp` into the Arduino IDE and install the needed boards as well as the libraries listed under `lib_deps` in the `platformio.ini` file. Comment out the `#include "Arduino.h"` line in main.cpp when using Arduino IDE. *Note that using PlatformIO is recommended!*

## Recommended Setup
The recommended setup is the D1 Mini board with a C1101 transceiver module and ESPiLight mode enabled. This gives you the simplest hardware connection, best radio preformance and the largest amount of supported switches out-of-the-box. For other options see the documentation below.

Simply uncomment the `#define USE_CC1101` and `#define USE_ESPILIGHT` lines in the `src/main.ccp` file and connect the D1 Mini and C1101 boards according to the table below.

![D1 Mini Image](/doc/d1_mini.jpg?raw=true "D1 Mini ESP8266 Board")![CC1101 Image](/doc/cc1101.jpg?raw=true "CC1101 Transceiver")

````
ESP8266		/	CC1101
------------------------------------
3V3	(3,3V)	/	VCC
G	(GND)	/	GND
D5	(SCK)	/	SCK
D6	(MISO)	/	MISO
D7	(MOSI)	/	MOSI
D8	(SS)	/	CSN
D1	(PWM)	/	GDO0 (TX)
D2	(PWM)	/	GDO2 (RX)
````

## Hardware Options
### Microcontrollers
- ESP8266 / D1 Mini Board (3.3V) - recommended with CC1101, needed for WiFi and ESPiLight
- Arduino Micro (5V) - best for the simple receivers/senders as they work at 5V
- Arduino Nano (3.3V) - for CC1101 without WiFi or ESPiLight
- Other Arduinos, ESP32 (see below)

The PlatformIO project is by default set up for the D1 Mini board. Support for Arduino Micro is also prepared, change `default_envs` in platformio.ini to `micro` to switch. See the PlatformIO documentation on how to compile for other boards / hardware. Note that you will have to change the input/output pin values in the software for each type of microcontroller (see below).

### Simple 433MHz sender / receivers
- Use 5V power
- Cheap (0.5-2$)
- For the sender these modules (e.g FS1000A) seem to work fine for me
- For the receiver the very cheap modules didn't receive from very far for me

![Cheap Modules Image](/doc/simple_modules.jpg?raw=true "Simple Modules")

### Superheterodyne 433MHz receiver
- Uses 5V power
- These "superheterodyne" (NOT superregeneration) receivers worked much better for me than the simple receivers
- Still Cheap (2-5$)

![superheterodyne image](/doc/superheterodyne.jpg?raw=true "Superheterodyne Receiver")

### CC1101 based 433MHz transceiver
- Uses 3.3V power
- Can receive and send in one module
- Configurable via SPI
- Often comes with a proper antenna
- These modules work much better in general but might be more expensive (8-15$)

![CC1101 Image](/doc/cc1101.jpg?raw=true "CC1101 Transceiver")

### Connections
#### Simple Modules
For the simple modules and the superheterodyne receiver, connect a 173mm piece of solid wire as antenna to the ANT pin, connect VCC to 5V on the micorcontroller board and connect GND to ground on the microcontroller board. See below for which pin on the microcontroller to use for the receiver/sender DATA pins.

#### CC1101
For the CC1101 module, see [this repository](https://github.com/LSatan/RCSwitch-CC1101-Driver-Lib) for info and images showing how to connect these transceivers to various microcontrollers. See below for enabling support for the CC1101 module in the transceiver code.

## The Software
### Configuration
To configure the transceiver open `src/main.cpp` and change/uncomment the following `#define` parameters.

#### `#define RC_INPUT_PIN D2`/ `#define RC_OUTPUT_PIN D1`
These values are needed and represent the input (receiver) and output (transmitter) pins. The microcontroller has to support interrupts on the input pin for the receiver, almost any output pin can be used for the transmitter. Note that these are actual pin numbers, not interrupt numbers.

Usual Values:
- ESP8266
  - Receiver on pin **`D2`** (IRQ4)
  - Sender on pin **`D1`**
- Arduino Micro 
  - Receiver on pin **`3`** (IRQ 0)
  - Sender on pin **`4`**
- Arduino Nano
  - Receiver on pin **`2`** (IRQ 0)
  - Sender on pin **`6`**

#### `#define USE_CC1101`
You can use a more advanced CC1101 based transceiver module instead of simple 433MHz receiver/sender pairs. These modules also have send and receive pins but are additionally connected via SPI for configuration.

Works on Arduino and ESP.

#### `#define USE_WEBSOCKET`
You can use WiFi to connect to the computer instead of USB.

This will create a websocket server to send/receive data in addition to the serial port, default hostname is `arduino-433`, port 80. Specify your WiFi credentials in `#define WIFI_SSID "ssid_here"` and `#define WIFI_PASS "pass_here"`.

Works on ESP.

#### `#define USE_ESPILIGHT`
You can use ESPilight insted of rc-switch to decode switches.

The send/receive data format will change to JSON, with `type` and `message` content. Additionally pilight debug messages, always beginning with `pilight` might be generated.

Works on ESP.

### Adding support for unsupported switches when using rc-switch
If you have a 433 device that doesn't work you can try and download a different version of the rcswitch library and run a "discovery application" that suggests how to extend the rcswitch.cpp file to add support for the unknown signal:

https://github.com/Martin-Laclaustra/rc-switch/tree/protocollessreceiver

Use the `protocollessreceiver` branch in that repository, it includes the Arduino discovery application example.

### Transceiver transmit protocol
*Note: If you're using homebridge-433-arduino you can skip this section unless you're interested in the interna of the transceiver communication.*

In its default mode the transceiver will use the USB serial port to send rc-switch data (code, pulse, protocol) in a format like `12345/123/1` when any 433MHz codes are received and decoded. When receiving serial data in the same format the transceiver will send the corresponding 433MHz data and return an `OK` message. The serial data is terminated by `\n`.

In ESPiLight mode the transceiver will send received signals in a JSON based format like `{"type":"switch_type","message":{"id":"A3","unit":"20","state":"off"}}`. When sending data the format changes slightly to `{"type":"switch_type","message":{"id":"A3","unit":"20","off":1}}`. See the ESPiLight documentation for more info.

Accordingly, the transceiver will only ever output 4 types of messages:
- Message `OK` -> after receiving a message and sending the corresponding code
- Message starting with `{` -> JSON data when using ESPiLight
- Message starting with `pilight` -> debug data when using ESPiLight
- Message starting with a number -> code/pulse/protocol message when using rc-switch

Note that when sending RC data the transceiver can not receive any new commands, wait until the `OK` message has been sent back before sending any new commands.

## License
Published under the MIT License.