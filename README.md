# pislave
I2C Slave that responds to Itho devices

# Upload and Compile
Upload to Raspberry Pi and compile with: g++ -l pthread -l pigpio -o pislave pislave.cpp

# Run
sudo ./pislave

This will launch the slave and respond to data being send to I2C address 0x80 (which is 0x40 using 7 bit).
Data being received is printed to the console.

Options for control:
*x = Exit the program and close communication
*r = Send the GetRegelaar package which should retrieve the node information from connect node 0x82 (Warmtepomp, Autotemp)
