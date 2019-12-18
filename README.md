# pislave
Set of tools to communicate with Itho WPU and Autotemp unit.
The tools are supposed to run on a Raspberry Pi, and there is some electronics involved to connect
the device to the WPU or Autotemp via UTP cable.

The communication to the Itho devices works via I2C. You need both a master and a slave on the I2C bus.
The Master is used to send requests to the WPU or Autotemp. The Slave is used to receive the answer.

The toolset:
- pimaster.cpp: Initial attempt to send commands to the Itho devices. This one has been replaced by pimaster2.c
- pimaster2.c: Tool that can be used to send queries (hardcoded) to the Itho device.
- emu_autotemp.c: Simulator for Autotemp which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- emu_heatpump.c: Simulator for Heat Pump which is normally at address 0x82, can be used to test the other programs without connecting to the Itho device. Does send a datalog message
- pislave.cpp: Tool to receive the data from the Itho device. Currently some hardcoded actions are possible: Insert raw data into SQLite DB, Parse a single value and upload to Domoticz
- pislave82.cpp: Simulator for WPU/Automtemp on receival side.
 

# Upload and Compile
Upload to Raspberry Pi and compile with the instructions given in the source file itself.

# Run
Common operations works by launching first pislave and then pimaster2

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