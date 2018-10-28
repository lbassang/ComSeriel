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
#include <string.h>

#define FIFO_PATH "fifo"
#define DEVICE "/dev/ttyUSB0"
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

int fd = 0;
int fd_fifo = 0;

void openPort();
void closePort(int);
void readResponses();
void parseResponse(char[]);
void printTocken(char[]);
struct sigaction act;
struct termios settings;

void  intHandler(int sig){
	printf(ANSI_COLOR_RED "Signal %i received !! STOP\n" ANSI_COLOR_RESET, sig);
	closePort(1);
	exit(0);
}

int main(){
  act.sa_handler = intHandler;
  sigaction(SIGINT, &act, NULL); 
  openPort();
  fd_fifo = open(FIFO_PATH, O_WRONLY);
  if(fd_fifo < 0) {
  	printf(ANSI_COLOR_RED "Error FIFO %i: %s\n\n" ANSI_COLOR_RESET, fd_fifo, strerror(errno));
  	exit(0);
  }
	readResponses();
	closePort(1);
	return 0;
}

/**
* Configure the communication
* for the ttyUSB0 port
*/
void openPort(){
	fd = open(DEVICE, O_RDWR); /*Connect to port*/
	if (fd < 0) {
  	printf(ANSI_COLOR_RED "Error opening %s: %s\n\n" ANSI_COLOR_RESET, DEVICE, strerror(errno));
    exit(0);
  }
  printf(ANSI_COLOR_YELLOW "[+]Port %s open\n" ANSI_COLOR_RESET, DEVICE);
	tcgetattr(fd, &settings);
	
	/*setting com options*/
	cfsetospeed(&settings, BAUDRATE); /* baud rate */
	settings.c_cflag &= ~PARENB; /* no parity */
	settings.c_cflag |= CSTOPB; /* 2 stop bit  (not sure)*/
	settings.c_cflag &= ~CSIZE;
	settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
	settings.c_lflag = ICANON; /* canonical mode */
	//settings.c_cflag &= ~CRTSCTS; /* No Hardware flow Control*/
	settings.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);

	/* apply the settings */
	if (tcsetattr(fd, TCSANOW, &settings) != 0) {
  	printf(ANSI_COLOR_RED "Error from tcsetattr: %s\n" ANSI_COLOR_RESET, strerror(errno));
    close(fd);
    exit(0);
  }
  printf(ANSI_COLOR_YELLOW "[+]Port %s configured\n\n" ANSI_COLOR_RESET, DEVICE);
	tcflush(fd, TCOFLUSH);

}

void closePort(int case_int){
	char* case_str = "-";
	if(case_int>=0) case_str = "+";
	printf(ANSI_COLOR_YELLOW "[%s]Closing %s\n" ANSI_COLOR_RESET, case_str, DEVICE);
	close(fd);
	printf(ANSI_COLOR_YELLOW "[%s]%s closed\n\n" ANSI_COLOR_RESET, case_str, DEVICE);
}


void readResponses(){
int index=0;
	do {
		char buf[1024];
		int rdlen;
		rdlen = read(fd, buf, sizeof(char)*1024-1);
		if(rdlen > 0){
			printf(ANSI_COLOR_YELLOW "***************" ANSI_COLOR_RESET "\n");
			printf(ANSI_COLOR_CYAN "Index de lecture : %d" ANSI_COLOR_RESET "\n", index);
			parseResponse(buf);
			printf(ANSI_COLOR_YELLOW "***************\n" ANSI_COLOR_RESET);
			index++;
		}else if(rdlen < 0){
			printf(ANSI_COLOR_RED "Error from read: %d: %s" ANSI_COLOR_RESET "\n", rdlen, strerror(errno));
		}
	}while(1);
}
void printTocken(char token[]){
	if (strncmp("AT+",token,3)==0){
		/* Echo from cmd */
		printf(ANSI_COLOR_WHITE "Echo of cmd : " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", token);
	}else if(strncmp("+",token,1)==0){
		/* Status info */
		printf(ANSI_COLOR_WHITE "Status Info : " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", token);
		if(strncmp("+TX-ACK",token,7)==0){
			/* ACK when pkt is sent to the network */
			write(fd_fifo,token,strlen(token)+1);
		}
	}else if(strstr(token,"1.0.")!=NULL){
		/* EVB version */
		printf(ANSI_COLOR_WHITE "Board version : " ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "\n", token);
	}else if(strncmp("OK",token,2)==0){
		printf(ANSI_COLOR_WHITE "Response : " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", token);
		write(fd_fifo, "OK",3);
	}else if(strncmp("ERROR",token,5)==0){
		printf(ANSI_COLOR_WHITE "Response : " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", token);
		write(fd_fifo, "ERROR",6);
	}else if(strncmp("BUSY",token,4)==0){
		printf(ANSI_COLOR_WHITE "Response : " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", token);
		write(fd_fifo, "BUSY", 5);
	}else{
		printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", token);
	}
}

void parseResponse(char buf[]){
	printf("Enter parsing method\n");
	char * pch;
	int index = 0;
	//printf("Splitting string \"%s\" into tokens :\n", buf);
	pch = strtok(buf,"\r\n");
	while(pch != NULL){
		printTocken(pch);
		//printf("TOKEN %i : %s\n",index, pch);
		pch = strtok(NULL,"\r\n");
		++index;
	}
	printf("End parsing method\n");
}








