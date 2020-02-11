# arduino-433
Arduino/ESP based 433MHz home control transceiver

## Introduction
This Arduino code allows you to create a cheap 433MHz wireless transceiver to control electric switches and other home appliances. It can use USB or WiFi to connect to a computer to receive and send commands. It is mainly used for the [homebridge-433-arduino](https://github.com/normen/homebridge-433-arduino) plugin but can of course be used otherwise as well.

The project uses the PlatformIO IDE and runs on Arduino (Micro/Nano etc.) as well as ESP8266. To decode signals rc-switch or ESPiLight can be used. Simple 433MHz receiver/sender hardware or more advanced CC1101 based transceiver modules are supported to receive and send 433MHz RC data.

## Installation 
### PlatformIO (recommended)
- Download and install the PlatformIO IDE, i.e. VisualStudioCode (see https://platformio.org)
- Clone or download this github repository, e.g. `git clone https://github.com/normen/arduino-433`
- Open the project in PlatformIO
- Configure the platformio.ini build options as needed (see below)
- Configure the code as needed (see below)
- Connect the desired microcontroller and transceiver hardware (see below)
- Build and upload the project to your Arduino or ESP

### Arduino IDE
Alternatively you can copy-paste the code in `src/main.cpp` into the Arduino IDE and install the libraries listed under `lib_deps` in the `platformio.ini` file. Comment out the `#include "Arduino.h"` line in main.cpp when doing that.

### The Hardware
#### Microcontroller options
- ESP8266 (3.3V) - recommended with CC1101, needed for WiFi and ESPiLight
- Arduino Micro (5V) - recommended for the simple receivers/senders as they work best at 5V
- Arduino Nano (3.3V) - for CC1101 without WiFi or ESPiLight
- Others (see below)

The PlatformIO project is prepared for the D1 Mini board (ESP8266) as well as Arduino Micro, change `default_envs` in platformio.ini to `micro` to switch. See the PlatformIO documentation on how to compile for other boards / hardware.

#### Transceiver module options
- Simple 433 receiver/sender pairs
  - Use 5V power
  - For the sender you can use basically any 433MHz module (e.g FS1000A)
  - For the receiver the very cheap modules didn't work properly for me, try to get the "superheterodyne" (NOT superregeneration) receiver as it works MUCH better.
  - Cheap (1-4$)
- CC1101 based transceiver
  - Use 3.3V power
  - Configurable via SPI
  - Often come with a proper antenna
  - These work much better usually but might be more expensive (5-15$)

See below for the pins used to connect the transceiver hardware.

### The Software
#### Operation
Note: If you're using homebridge-433-arduino you can skip to the "Configuration" section unless you're interested in the interna of the transceiver communication.

In its default mode the transceiver will use the USB serial port to send rc-switch data (code, pulse, protocol) in a format like `12345/123/1` when any 433MHz codes are received and decoded. When receiving serial data in the same format the transceiver will send the corresponding 433MHz data and return an `OK` message. The serial data is terminated by `\n`.

In ESPiLight mode the transceiver will send received signals in a JSON based format like `{"type":"switch_type","message":{"id":"A3","unit":"20","state":"off"}}`. When sending data the format changes slightly to `{"type":"switch_type","message":{"id":"A3","unit":"20","off":1}}`. See the ESPiLight documentation for more info.

Accordingly, the transceiver will only ever output 4 types of messages:
- Message `OK` -> after receiving a message and sending the corresponding code
- Message starting with `{` -> JSON data when using ESPiLight
- Message starting with `pilight` -> debug data when using ESPiLight
- Message starting with a number -> code/pulse/protocol message when using rc-switch

Note that when sending RC data the transceiver can not receive any new commands, wait until the `OK` message has been sent back before sending any new commands.

#### Configuration
To configure the transceiver open `src/main.cpp` and change/uncomment the following `#define` parameters.

##### `#define RC_INPUT_PIN D2`/ `#define RC_OUTPUT_PIN D1`
These values are needed and represent the input (receiver) and output (transmitter) pins. The microcontroller has to support interrupts on the input pin for the receiver, almost any output pin can be used for the transmitter. Note that these are actual pin numbers, not interrupt numbers.

Usual Values:
- ESP8266 Receiver on IRQ 4 = `D2`
- ESP8266 Transmit on pin 5 = `D1`
- Arduino Micro Receiver on IRQ 0 = `3`
- Arduino Micro Sender on pin 4 = `4`
- Arduino Nano Receiver on IRQ 0 = `2`
- Arduino Nano Transmit = `6`

##### `#define USE_CC1101`
You can use a more advanced CC1101 based transceiver module instead of simple 433MHz receiver/sender pairs. These modules also have send and receive pins but are additionally connected via SPI for configuration.

See https://github.com/LSatan/RCSwitch-CC1101-Driver-Lib on how to connect the CC1101 module.

Works on Arduino and ESP.

##### `#define USE_WEBSOCKET`
You can use WiFi to connect to the computer instead of USB.

This will create a websocket server to send/receive data in addition to the serial port, default hostname is `arduino-433`, port 80. Specify your WiFi credentials in `#define WIFI_SSID "ssid_here"` and `#define WIFI_PASS "pass_here"`.

Works on ESP.

##### `#define USE_ESPILIGHT`
You can use ESPilight insted of rc-switch to decode switches.

The send/receive data format will change to JSON, with `type` and `message` content. Additionally pilight debug messages, always beginning with `pilight` might be generated.

Works on ESP.

## License
Published under the MIT License.