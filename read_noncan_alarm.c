// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>


// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
unsigned const int TIMETORUN = 12;



void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = TIMETORUN; // read with inter-byte timer used
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Loop for input
    	unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
	unsigned char  ua[5] = {0x7E,0x01,0x07,0x00,0x7E};
	// to check if the SET frame is correct
	unsigned char set[5];
	// xor to calculate the bcc1 value
	ua[3] = ua[1]^ua[2];
	set[3] = set[1]^set[2];
	unsigned char bcc_set;


	(void)signal(SIGALRM, alarmHandler);
	
    while(alarmCount < TIMETORUN){
	int bytes = read(fd,set,5);
	//printf("data received: %s bytes:%d",set,bytes);
	// check bcc
	bcc_set = set[1]^set[2];

	if(set[4]==ua[4] && set[0] == ua[0] && bcc_set == set[3] ){
		printf("array is correct");
		set[0] = 0;
		set[1] = 0;
		set[2] = 0;
		set[3] = 0;
		set[4] = 0;
		//int bytes1 = write(fd,ua,sizeof(ua));
	}

	//alarm
	if (alarmEnabled == FALSE)
        {

            alarm(TIMETORUN); // Set alarm to be triggered $
            alarmEnabled = TRUE;
        }
    }

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
