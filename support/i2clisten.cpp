// compile: g++ -l pigpio -o i2clisten i2clisten.c

#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DATASZ 2048

#define SELFADDR 0x40

void simple_bsc_event_callback(int event, uint32_t tick) {
  bsc_xfer_t xfer;
  int i = 0, status;

  xfer.control = (SELFADDR << 16) | 0x305;
  status = bscXfer(&xfer);
  if (!status) {
    fprintf(stderr, "process register failed.\n");
  }
  if (xfer.rxCnt) {
    for (i = 0; i < xfer.rxCnt; i++) fprintf(stdout, " %02x", xfer.rxBuf[i]);
    fprintf(stdout, "\n");
  }
}

int main(int argc, char* argv[]) {
  bsc_xfer_t xfer;
  int status;
  if (gpioInitialise() < 0) {
    fprintf(stderr, "pigpio initialisation failed.\n");
    return 1;
  }
  if (!eventSetFunc(PI_EVENT_BSC, simple_bsc_event_callback)) {
    xfer.control = (SELFADDR << 16) | 0x305;
    status = bscXfer(&xfer);
    if (!status) {
      fprintf(stderr, "process register failed.\n");
    }
  }

  sleep(20);
  gpioTerminate();
}

/*
pi@raspberrypi:~/venv/sandbox $ sudo ./prog
 0f 45 65 01 00 00 83 02 00 01 00 ec d0 00 00 75  00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 a2
 4a 2f 00 51 4c 6f 67 69 63 20 46 61 73 74 4c 69  51 4c 34 31 31 33 34 48 4c 52 4a 20 31 30 47 f1
 [b]^^^^ (some bytes missing as seen below) -[/b]

 0f 45 65 01 00 00 13 62 45 20 41 64 61 70 74 65  72 20 2d 20 4c 41 4e 34 00 51 4c 6f 67 69 63 20
 69 6e 51 20 51 4c 34 31 31 33 34 48 4c 52 4a 20  62 45 20 41 64 61 70 74 65 72 20 2d 20 4c 41 42
 12 65 01 00 00 63 4e 2b 52 44 4d 41 00 00 00 ff

below observed on i2csniffer

radix: hexadecimal
0F 45 65 01 00 00 83 02 00 01 00 EC D0 00 00 75 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 A2
[b]02 02 01 00 00[/b] 4A 2F 00 51 4C 6F 67 69 63 20 46  61 73 74 4C 69 6E 51 20 51 4C 34 31 31 33 34
48 4C 52 4A 20 31 30 47 F1
^^^^^^^^^^^^^^^^ (missing of 1st transaction)                                         ^^^^^^^^^

*/
