#include <pthread.h>
#include <pigpio.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using namespace std;

/*
    g++ -l pthread -l pigpio -o pislave82 pislave82.cpp
*/

void closeSlave();
int getControlBits(int, bool);
void *work(void *);

const int slaveAddress = 0x41; // <-- 0x41 is 7 bit address of 0x82
bsc_xfer_t xfer; // Struct to control data flow
int command = 0; // -1=exit, 0=read mode

int main() 
{
    pthread_t cThread;

    gpioInitialise();
    cout << "Initialized GPIOs\n";

    cout << "Starting a new thread to receive data" << endl;
    if(pthread_create(&cThread, NULL, work, NULL))
    {
        perror("ERROR creating thread.");
    }

    string strinput;
    string exit_command = "x";

    cout << "Enter a command (x=exit): ";
    while (strinput.compare(exit_command))
    {
        getline(cin, strinput);
        cout << "You entered: " << strinput << endl;
    }
    cout << "stopping" << endl;
    command = -1;
    sleep(1);
    closeSlave();

    return 0;
}

void *work(void * parm)
{
    //  TODO: Add the pthread_kill to signal this thread instead of just shutting down

    // Close old device (if any)
    xfer.control = getControlBits(slaveAddress, false); // To avoid conflicts when restarting
    bscXfer(&xfer);
    // Set I2C slave address
    xfer.control = getControlBits(slaveAddress, true);
    int status = bscXfer(&xfer); // Should now be visible in I2C-Scanners

    if (status >= 0)
    {
        cout << "Opened slave\n";
        xfer.rxCnt = 0;
        int loopcount = 0;
        int receivedcount = 0;
        char *buffer = new char[1000];
        while(command!=-1)
        {
            bscXfer(&xfer);
            if(xfer.rxCnt > 0) 
            {
                for(int i = 0; i < xfer.rxCnt; i++)
                {
                    buffer[receivedcount] = xfer.rxBuf[i];
                    receivedcount++;
                    // printf("0x%x", );
                    // cout << std::hex << " 0x" << xfer.rxBuf[i];
                }
            }
            else
            {
                loopcount++;
                if (loopcount>10000)
                {
                    if (receivedcount>0)
                    {
                        // write buffer and clear all
                        cout << "[0x82";
                        for (int i = 0; i < receivedcount; i++)
                            printf(" 0x%02X", buffer[i]);
                        cout << "]\n";

                        receivedcount = 0;
                    }
                    loopcount = 0;
                }
                else
                {
                    usleep(10); // 10 us
                }
            }
        }
        cout << "ended loop" << endl;
        delete[] buffer;
    }
    else
        cout << "Failed to open slave!!!\n";
}


void closeSlave() {
    xfer.control = getControlBits(slaveAddress, false);
    bscXfer(&xfer);
    cout << "Closed slave.\n";

    gpioTerminate();
    cout << "Terminated GPIOs.\n";
}


int getControlBits(int address /* max 127 */, bool open) {
    /*
    Excerpt from http://abyz.me.uk/rpi/pigpio/cif.html#bscXfer regarding the control bits:

    22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    a  a  a  a  a  a  a  -  -  IT HC TF IR RE TE BK EC ES PL PH I2 SP EN

    Bits 0-13 are copied unchanged to the BSC CR register. See pages 163-165 of the Broadcom 
    peripherals document for full details. 

    aaaaaaa defines the I2C slave address (only relevant in I2C mode)
    IT  invert transmit status flags
    HC  enable host control
    TF  enable test FIFO
    IR  invert receive status flags
    RE  enable receive
    TE  enable transmit
    BK  abort operation and clear FIFOs
    EC  send control register as first I2C byte
    ES  send status register as first I2C byte
    PL  set SPI polarity high
    PH  set SPI phase high
    I2  enable I2C mode
    SP  enable SPI mode
    EN  enable BSC peripheral
    */

    // Flags like this: 0b/*IT:*/0/*HC:*/0/*TF:*/0/*IR:*/0/*RE:*/0/*TE:*/0/*BK:*/0/*EC:*/0/*ES:*/0/*PL:*/0/*PH:*/0/*I2:*/0/*SP:*/0/*EN:*/0;

    int flags;
    if(open)
        flags = /*RE:*/ (1 << 9) | /*TE:*/ (1 << 8) | /*I2:*/ (1 << 2) | /*EN:*/ (1 << 0);
    else // Close/Abort
        flags = /*BK:*/ (1 << 7) | /*I2:*/ (0 << 2) | /*EN:*/ (0 << 0);

    return (address << 16 /*= to the start of significant bits*/) | flags;
}