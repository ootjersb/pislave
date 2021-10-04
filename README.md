# pislave
Set of tools to communicate with Itho WPU and Autotemp units.
The tools are supposed to run on a Raspberry Pi, and there is some electronics involved to connect
the device to the WPU or Autotemp via UTP cable.

The communication to the Itho devices works via I2C. You need both a master and a slave on the I2C bus.
The Master is used to send requests to the WPU or Autotemp. The Slave is used to receive the answer.
The Itho device is registered at address `0x82`. The Raspberry Pi is registered at address `0x80`.
This project has been developed and tested with a Raspberry Pi 3B+. The Raspberry Pi Model 4 has a
little different API, which has not been tested.

The toolset:
- `pi_master/pimaster.cpp`: Initial attempt to send commands to the Itho devices. This one has been replaced by pimaster2.c
- `pi_master/pimaster2.c`: Tool that can be used to send queries (hardcoded) to the Itho device.
- `support/emu_autotemp.c`: Simulator for Autotemp which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- `support/emu_heatpump.c`: Simulator for Heat Pump which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- `pi_slave/pislave.cpp`: Tool to receive the data from the Itho device. Currently some hardcoded actions are possible: Insert raw data into SQLite DB, Parse a single value and upload to Domoticz
- `pi_slave/pislave82.cpp`: Simulator for WPU/Autotemp on receival side.
Other files are included with the above.

This project started from this forum thread: https://www.circuitsonline.net/forum/view/65868

# Wiring
The component involved is a level shifter, because the Itho units run on 5V instead of 3.3V.
I used the Adafruit BSS138 level shifter: https://www.kiwi-electronics.nl/4-channel-i2c-safe-bi-directional-logic-level-converter-BSS138?search=adafruit%20i2c%20level%20shifter&description=true
Wiring like this:
- I2C-SDA: Raspberry Pi GPIO02 (PIN03) & GPIO18 (PIN12) to Level shifter Low (A1), Level shifter high (B1) to RJ-45 pin 7 (White/Brown)
- I2C-SCL: Raspberry Pi GPIO19 (PIN35) & GPIO03 (PIN05) to Level shifter Low (A2), Level shifter high (B2) to RJ-45 pin 2 (Orange)
- Raspberry Pi - GRND(PIN39) - Level shifter GRND Hi and Low - RJ45 Pin 3 (White/Green)
- Raspberry Pi - 3.3V (PIN01) to Level shifter Low (LV), Level shifter High (HV) is unattached. The reason for this is that the RJ-45 pin 5 (White/Blue) measures around 15 volt, and a level shifter cannot handle that usually. 
There are pull up resistors in place at WPU/Autotemp side, so they provide the pull up to the correct high level.
![image](https://github.com/ootjersb/pislave/blob/master/wiring-schema.png?raw=true) 
 
# Dependencies
pislave is dependent upon:
- libcurl
- pigpio
- libsqlite3
- CMake

```bash
sudo apt-get install \
    ibcurl4-openssl-dev \
    pigpio \
    libsqlite3-dev \
    cmake
```

Enabled I2C port: `sudo raspi-config`. Option 5 and enable I2C.
 
# Upload and Compile
Upload to Raspberry Pi and compile with the instructions given in the source file itself.
Easiest way to upload is just to clone this repository into the pi home directory.

## Cmake
Compile the whole project:
```bash
cmake .
cmake --build .
```

# Configuration
At the moment all of the settings and options are hardcoded. Most difficult configuration is probably bound to the model of the HeatPump itself. The parsing of the datalog message is model specific.

# Run
Common operations works by launching first `pislave` and then `pimaster`

# Service installation
The process can be run as system. The process reports output to console which is redirected to syslog by systemd.
Copy the service and timer files to `/lib/systemd/system/`.
```bash
cp support/*.service /lib/systemd/system/
cp support/*.timer /lib/systemd/system/
```
Afterwards enable and start the service and timer.
```bash
sudo systemctl enable ithowp.service
sudo systemctl start ithowp.service

sudo systemctl enable ithowp_request.timer
sudo systemctl start ithowp_request.timer

sudo systemctl enable ithowp_request_counter.timer
sudo systemctl start ithowp_request_counter.timer
```

# Todo
- [ ] Configuration options in configuration file
- [ ] Integrating send (`pimaster`) and receive (`pislave`) to single service.

## pislave
```bash
sudo ./pislave --debug
```

This will launch the receiver in debug mode and respond to data being send to I2C address `0x80` (which is `0x40` using 7 bit).
Data being received is handled accoring to the setting in `config.cpp`. This can be logged to console, domoticz or file

Options for control:  
- x = Exit the program and close communication

## pimaster2
After pislave has been launched you can open a second session and launch pimaster2 to execute commands:
```bash
./pimaster2
```

Options for control:  
- r = GetRegelaar. This is a quite generic message that asks the connected device for its information (make, model and such), that is usually the start when you are new.  
- a = Retrieve the datalog. This is the most interesting message in the sense that it asks for the sensor read outs. The data differs per device.

Another way of starting is by adding -f and a filename. This content of this file will be translated to a byte array and send. Example content:
```
[0x82 0x80 0xC2 0x10 0x04 0x00 0x28]
```
