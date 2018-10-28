#include "cmd.h"

int fd = 0;
struct termios settings;

/**
* Initialize the communication
* Send Echo cmd to the device
* Send Auto-Start cmd to the device
* Send Reset to the device
* @Param : withEcho (1 = y, 0 = n), withAS (1 = y, 0 = n)
* (if opening port and config setup fail, program will quit)
*/
void init(int withEcho, int withAS){
  openPort();
  setEcho(withEcho);
  sleep(1);
  setAS(withAS);
  sleep(1);
  doReset();
}

void printTermMainHelp(){
	printf("This is the main help for this terminal\n");
	printf(ANSI_COLOR_WHITE "quit :               " ANSI_COLOR_GREEN "Quit" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "clear :              " ANSI_COLOR_GREEN "Clear the screen" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "send :               " ANSI_COLOR_GREEN "Send program" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "write :              " ANSI_COLOR_GREEN "Free writing commands program" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "searchMax :          " ANSI_COLOR_GREEN "Begin search for the max payload size (please don't do anything else)" ANSI_COLOR_RESET  "\n");
	printf(ANSI_COLOR_WHITE "testPayload #size# : " ANSI_COLOR_GREEN "Baseband test with payload of #size# hexaCharater" ANSI_COLOR_RESET  "\n");

}

void printCmdHelp(){
	printf("This is the help for all programmed commands (1=set, 0=unset, ?=get status)\n");
	printf(ANSI_COLOR_WHITE "quit :           " ANSI_COLOR_GREEN "Back to main menu" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "clear :          " ANSI_COLOR_GREEN "Clear the screen" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "echo 1/0/? :     " ANSI_COLOR_GREEN "Set/Unset echo" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "as 1/0/? :       " ANSI_COLOR_GREEN "Set/Unset auto-start" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "register 1/0/? : " ANSI_COLOR_GREEN "Register/Unregister" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "reset :          " ANSI_COLOR_GREEN "Reset Device" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "sendTest :       " ANSI_COLOR_GREEN "Send test payload to Device" ANSI_COLOR_RESET "\n");		
	printf(ANSI_COLOR_WHITE "send #data# :    " ANSI_COLOR_GREEN "Send #data# (muste be hexa data) to Device" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_WHITE "sendN #n# :      " ANSI_COLOR_GREEN "Send a payload with size=n to Device" ANSI_COLOR_RESET "\n");	

}
void printNewLine(){
printf(ANSI_COLOR_BLUE "=> " ANSI_COLOR_RESET);
}
void printHelpWrite(){
	printf(ANSI_COLOR_WHITE"Write your commands and press ENTER to send\n(enter \"quit\" to exit this mode)\n");
	printf("*******************************************\n\n");
	printNewLine();
}

void clearScreen(){
	printf(ANSI_CLS "\n" ANSI_BEGIN_TERM);
}







/**
* Send a command to the
* device
* @Param : cmd the command to send
* @Require : 1[s] between commands
*/
void writeCmd(char* cmd){
  int n = 0;
  n = write(fd, cmd, strlen(cmd));
  if (n<0) {
    printf(ANSI_COLOR_RED "Error from write: %d, %d\n\n" ANSI_COLOR_RESET, n, errno);
	closePort(-1);
	exit(0);
  }
  printf(ANSI_COLOR_BLUE "%s\n\n" ANSI_COLOR_RESET, cmd);
}

/**
* Close de communication Port
* @Param : case_int 1 if good closing needed -1 if not
*/
void closePort(int case_int){
  char* case_str = "-";
  if(case_int>=0) case_str = "+";
  printf(ANSI_COLOR_YELLOW "[%s]Closing %s\n" ANSI_COLOR_RESET, case_str, DEVICE);
  close(fd);
  printf(ANSI_COLOR_YELLOW "[%s]%s closed\n\n" ANSI_COLOR_RESET, case_str, DEVICE);
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


void doReset(){
  writeCmd("AT+RESET\r");
}
void setEcho(int value){
  if(value==1)
    writeCmd("AT+E=1\r");
  else if(value==0)
		writeCmd("AT+E=0\r");
	else
		writeCmd("AT+E=?\r");
}
void setAS(int value){
  if(value==1)
    writeCmd("AT+AS=1\r");
  else if(value==0)
		writeCmd("AT+AS=0\r");
	else
		writeCmd("AT+AS=?\r");
}
void sendTest(){
	writeCmd("AT+TX=\"ABCDEF\"\r");
}
void sendData(char* data){
	data[strlen(data)-1]= 0;
	char str[2048];
	sprintf(str,"AT+TX=\"%s\"\r",data);
	writeCmd(str);
}
void sendN(int n){
	char data[n];
	for(int i = 0; i<n; ++i){
		data[i]='a';
	}
	data[n]=0;
	char str[2048];
	sprintf(str,"AT+TX=\"%s\"\r",data);
	writeCmd(str);
}
/**
* @Require control when using this method to wait until
* the end of process
*/
void registerDevice(int value){
  if(value==1)
		writeCmd("AT+CREG=1\r");
  else if(value==0)
		writeCmd("AT+CREG=0\r");
	else
		writeCmd("AT+CREG=?\r");
}


