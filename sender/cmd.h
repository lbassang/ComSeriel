#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>


/*DEFINE*/
#define UNUSED(x) (void)x
#define DEVICE "/dev/ttyUSB0"
#define FIFO_PATH "fifo"
#define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE 1
#define BAUDRATE B115200
#define TRUE 1
#define FALSE 0
#define ANSI_BEGIN_TERM		 "\033[1;1H"
#define ANSI_CLS					 "\033[2J"
#define ANSI_COLOR_WHITE 	 "\x1b[37m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"




/* Initialize com and device with echo on/off and auto-start on/off */
void init(int, int);

/* Clear the terminal screen */
void clearScreen();
/* Print the main help for the terminal use */
void printTermMainHelp();
/* Print the help for the terminal in programmed cmd mode */
void printCmdHelp();
/* Print the help for the writting terminal */
void printHelpWrite();
/* Print a new line for the writting terminal */
void printNewLine();
/* Send a command to the device */
void writeCmd(char* cmd);


/* Close the com port */
void closePort(int case_int);
/* Open the com port */
void openPort();

/* Test for payload size and return the number of hexa Sring char max */
int searchForMax();
/* Send full payload counter in continue until times end. Return number of payload sent */
int testPayloadCharge(int);

/* Send RESET */
void doReset();
/* Set ECHO value */
void setEcho(int);
/* Set Auto-Start value */
void setAS(int);
/* Launch un/-register process */
void registerDevice(int);
/* Send a test text to the device */
void sendTest();
/* Send the given data to the device */
void sendData(char*);
/* Send a payload of size n to the device */
void sendN(int);


