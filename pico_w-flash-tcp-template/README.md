# Raspberry Pi Pico template for Flash / TCP server
## Prerequirement
Install "pico-setup-windows-x64-standalone.exe"  
https://github.com/raspberrypi/pico-setup-windows/releases/tag/v1.5.1
## Get started
1. Modify "CMakeLists.txt" to your environmet.
    ```CMakeLists.txt
    # Set your SSID and Password
    set(WIFI_SSID "hogehoge")
    set(WIFI_PASSWORD "hogehoge")
    ```
1. Open this repository by "Pico - Visual Studio Code".
1. Modify PICO_SDK_PATH in CMakeLists.txt to suit your environment.
1. Setup VScode
    1. "Command Palate" > "CMake: Select a Kit" > "GCC 10.3.1 arm-none-eabi"
    1. "Command Palate" > "CMake: Set Build Target" > "pico_w-freertos-helloworld"
    1. "Command Palate" > "CMake: Set Debug Target" > "pico_w-freertos-helloworld"
1. Build and Debug by F5!
## Hardware
### Component
- Raspberry Pi Pico
- Raspberry Pi Pico W
    - Pinout    
    https://datasheets.raspberrypi.com/picow/PicoW-A4-Pinout.pdf
- Raspberry Pi Debug Probe 
    - https://www.raspberrypi.com/products/debug-probe/
    - https://datasheets.raspberrypi.com/debug/debug-connector-specification.pdf
### Connection
- Debug by probe connection
    | Pico W | Probe(DEBUG) |
    | ---- | ---- |
    | SWCLK | SWCLK |
    | SWDIO | SWDIO |
    | GND | GND |
- Output Printf() by default serial connection
    | Pico W | Probe(UART) |  
    | ---- | ---- |
    | TX(1) | RX |  
    | RX(2) | TX |
    | GND | GND |  

