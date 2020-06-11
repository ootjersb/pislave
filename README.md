# pislave
Set of tools to communicate with Itho WPU and Autotemp units.
The tools are supposed to run on a Raspberry Pi, and there is some electronics involved to connect
the device to the WPU or Autotemp via UTP cable.

The communication to the Itho devices works via I2C. You need both a master and a slave on the I2C bus.
The Master is used to send requests to the WPU or Autotemp. The Slave is used to receive the answer.
The Itho device is registered at address 0x82. The Raspberry Pi is registered at address 0x80.

The toolset:
- pimaster.cpp: Initial attempt to send commands to the Itho devices. This one has been replaced by pimaster2.c
- pimaster2.c: Tool that can be used to send queries (hardcoded) to the Itho device.
- emu_autotemp.c: Simulator for Autotemp which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- emu_heatpump.c: Simulator for Heat Pump which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- pislave.cpp: Tool to receive the data from the Itho device. Currently some hardcoded actions are possible: Insert raw data into SQLite DB, Parse a single value and upload to Domoticz
- pislave82.cpp: Simulator for WPU/Autotemp on receival side.
Other files are included with the above.

This project started from this forum thread: https://www.circuitsonline.net/forum/view/65868

# Wiring
The component involved is a level shifter, because the Itho units run on 5V instead of 3.3V.
I used the Adafruit BSS138 level shifter: https://www.kiwi-electronics.nl/4-channel-i2c-safe-bi-directional-logic-level-converter-BSS138?search=adafruit%20i2c%20level%20shifter&description=true
Wiring like this:
- I2C-SDA: Raspberry Pi GPIO02 (PIN03) & GPIO18 (PIN12) to Level shifter Low (A1), Level shifter high (B1) to RJ-45 pin 2 (Orange)
- I2C-SCL: Raspberry Pi GPIO19 (PIN35) & GPIO03 (PIN05) to Level shifter Low (A2), Level shifter high (B2) to RJ-45 pin 7 (White/Brown)
- Raspberry Pi - GRND(PIN39) - Level shifter GRND Hi and Low - RJ45 Pin 6 (Green)
- Raspberry Pi - 3.3V (PIN01) to Level shifter Low (LV)
![image](https://github.com/ootjersb/pislave/blob/master/wiring-schema.png?raw=true) 
 
# Dependencies
pislave is dependent upon:
- Curl: sudo apt-get install libcurl4-openssl-dev
- Sqlite3: sudo apt-get install libsqlite3-dev
- Enabled I2C port: sudo raspi-config. Option 5 and enable I2C.
 
# Upload and Compile
Upload to Raspberry Pi and compile with the instructions given in the source file itself.
Easiest way to upload is just to clone this repository into the pi home directory.

# Run
Common operations works by launching first pislave and then pimaster2

# Service installation
The process can be run as system (no syslog logging yet however, and no start/stop control). Stopping is done by killing the pislave proces :-(
Copy file ithowp.service to /lib/systemd/system/ithowp.service.
Reboot your Raspberry Pi.

# Cron job
You can use cron to schedule running the pimaster GetDatalog command. The crontab line for this is:
* * * * * /home/pi/pislave/pimaster2 97

# Todo
- Proper start/stop control for the service
- Proper syslog logging
- Interrupt for receiving data
- Integrating send (pimaster2) and receive (pislave) to single service.

## pislave
sudo ./pislave

This will launch the slave and respond to data being send to I2C address 0x80 (which is 0x40 using 7 bit).
Data being received is printed to the console.

Options for control:  
x = Exit the program and close communication  

## pimaster2
After pislave has been launched you can open a second session and launch pimaster2 to execute commands:
./pimaster2

Options for control:  
r = GetRegelaar. This is a quite generic message that asks the connected device for its information (make, model and such), that is usually the start when you are new.  
a = Retrieve the datalog. This is the most interesting message in the sense that it asks for the sensor read outs. The data differs per device.