#include "cmd.h"
#define TIMERMAX_SEC 600
#define LOG_FILE_PATH "log.txt"
struct sigaction act;

int fd_fifo;
char fifoResponse[1024];
int n = 0;
int sizePayloadBasebandTest = 118;
FILE *pFile_log;

/* Shared variables */
int responseReceived = 0;
int errorReceived = 0;
int busyReceived = 0;
int timerShouldGo = 0;
int endTime = 0;

/* Mutex for shared variables */
sem_t mutexError;
sem_t mutexBusy;
sem_t mutexTime;

/* Threads */
pthread_t readFifoThread_t;
pthread_t timerThread_t;

/* Threads methods */
void* timerThread(void *);
void* readFifoThread(void *);

/* Utils fonctions */
void createLogFile();
void hexaCounter(int, char*);
void createSemaphores();
void destroySemaphores();

/* Terminal fonctions */
void quitProgram();
void sendProgram();
void writeProgram();
void mainTerminal();

/**
* Retrieve Ctrl-C input from Users and quit Program
*/
void  intHandler(int sig){
	printf(ANSI_COLOR_RED "Signal %i received !! STOP process %i\n" ANSI_COLOR_RESET, sig, getpid());
  quitProgram();
}

void* timerThread(void *param){
	UNUSED(param);
	int value = TIMERMAX_SEC;
	while(endTime==0){
		sem_wait(&mutexTime);
		if(timerShouldGo == 1){
			timerShouldGo = 0;
			sem_post(&mutexTime);
			while(value > 0){
				sleep(1);
				value--;
			}
			sem_wait(&mutexTime);
			endTime = 1;
			sem_post(&mutexTime);
		}else{
			sem_post(&mutexTime);
		}
	}while(1); // To avoid warning: control reaches end of non-void function [-Wreturn-type]

}

/**
* Read coniniously the response fifo
* and should save the output into
* the corresponding variable
*/
void* readFifoThread(void *param){
	UNUSED(param);
	fd_fifo = open(FIFO_PATH, O_RDONLY);
	if(fd_fifo < 0) printf(ANSI_COLOR_RED "Error FIFO %i: %s\n\n" ANSI_COLOR_RESET, fd_fifo, strerror(errno));
	while(1){
		n = read(fd_fifo, fifoResponse, sizeof(fifoResponse));
  	printf(ANSI_COLOR_MAGENTA "[FIFO_READ]Response from device : %s" ANSI_COLOR_RESET "\n", fifoResponse);
  	if(strncmp("ERROR",fifoResponse,5)==0){
  		sem_wait(&mutexError);
  		errorReceived = 1;
  		sem_post(&mutexError);
  	}else if(strncmp("BUSY",fifoResponse,4)==0){
  		sem_wait(&mutexBusy);
  		busyReceived = 1;
  		sem_post(&mutexBusy);
  	}else if(strncmp("+TX-ACK",fifoResponse,7)==0){
  		time_t t = time(NULL);
  		struct tm tm;
  		tm = *localtime(&t);
  		fprintf(pFile_log,"** ");
  		fprintf(pFile_log, "%d-%d-%d %d:%d:%d--> %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,fifoResponse);		
  	}
  }
}
void createLogFile(){
	time_t t = time(NULL);
	struct tm tm;
	tm = *localtime(&t);
	pFile_log = fopen(LOG_FILE_PATH, "a");
	fprintf(pFile_log,"-------------------------------------------\n");
	fprintf(pFile_log,"BEGIN SESSION : ");
	fprintf(pFile_log,"%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}
int main(){
	createLogFile();
	act.sa_handler = intHandler;
  sigaction(SIGINT, &act, NULL); 
  createSemaphores();
	if(pthread_create(&timerThread_t, NULL, timerThread, NULL)){
  	printf(ANSI_COLOR_RED "Error Creating Timer Thread : %s\n\n" ANSI_COLOR_RESET, strerror(errno));
  	quitProgram();
  }
  if(pthread_create(&readFifoThread_t, NULL, readFifoThread, NULL)){
  	printf(ANSI_COLOR_RED "Error Creating Thread fifoReaderProcess : %s\n\n" ANSI_COLOR_RESET, strerror(errno));
  	quitProgram();
  }
  init(1,1);
  sleep(4);
  mainTerminal();
  quitProgram();
  return 0;
}
void mainTerminal(){
	clearScreen();
	printTermMainHelp();
  while(1){
  	char userInput[1024];
  	fgets(userInput,1024,stdin);
  	if(strncmp("quit",userInput,4)==0){
  		/* quit */
  		quitProgram();
  	}else if(strncmp("clear",userInput,5)==0){
  		/* clear terminal */
			clearScreen();
			printTermMainHelp();
  	}else if(strncmp("send",userInput,4)==0){
  		/* access to send program */
  		clearScreen();
  		sendProgram();
  	}else if(strncmp("write",userInput,5)==0){
  		/* access to write program */
  		clearScreen();
  		writeProgram();
  	}else if(strncmp("searchMax",userInput,9)==0){
  		/* search for the max payload size */
  		int max = 0;
  		max = searchForMax();
  		printf(ANSI_COLOR_WHITE " MAX PAYLOAD SIZE : " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n",max);
  	}else if(strncmp("testPayload",userInput,11)==0){
  		/* Baseband Test with given payload size */
  		char* token;
			char* delim = " ";
			int payloadSent = 0;
			token = strtok(userInput, delim);
			token = strtok(NULL, delim);
			if(token!=NULL){
				int a = atoi(token);
				payloadSent = testPayloadCharge(a);
				printf(ANSI_COLOR_WHITE "PACKET GIVEN IN QUEUE : " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n",payloadSent);
			}else{
				printf(ANSI_COLOR_WHITE " USAGE : testPayload #size# :    " ANSI_COLOR_GREEN "Baseband test with payload of #size# hexaCharater" ANSI_COLOR_RESET  "\n");
			}
  		
  		  		
  	}else {
  		clearScreen();
			printTermMainHelp();
		}
	}
}
/**
* Quit sender main program
*/
void quitProgram(){
	time_t t = time(NULL);
	struct tm tm;
	tm = *localtime(&t);
	fprintf(pFile_log,"END SESSION : ");
	fprintf(pFile_log,"%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(pFile_log,"-------------------------------------------\n");
  fclose(pFile_log);
  close(fd_fifo);
  closePort(1);
  destroySemaphores();
  
  if(pthread_kill(readFifoThread_t,SIGKILL)==0){	
  	printf(ANSI_COLOR_RED "[+]Killed ReadFifo thread\n" ANSI_COLOR_RESET);
  }else{
  	printf(ANSI_COLOR_RED "[-]ReadFifo thread NOT Killed\n" ANSI_COLOR_RESET);  
  }
  if(pthread_kill(timerThread_t,SIGKILL)==0){	
  	printf(ANSI_COLOR_RED "[+]Killed Time thread\n" ANSI_COLOR_RESET);
  }else{
  	printf(ANSI_COLOR_RED "[-]Time thread NOT Killed\n" ANSI_COLOR_RESET);  
  } 
  exit(0);
}


void sendProgram(){
	printCmdHelp();
	while(1){
  	char userInput[1024];
  	fgets(userInput,1024,stdin);
  	printf("INPUT : %s\n",userInput);
		if(strncmp("quit",userInput,4)==0){
			/* quit */
			clearScreen();
			printTermMainHelp();
			break;
		}else if(strncmp("clear",userInput,5)==0){
			/* clear terminal */
		  clearScreen();
			printCmdHelp();
  	}else if(strncmp("echo",userInput,4)==0){
  		/* Send echo cmd */
  		if(strncmp("echo 1",userInput,6)==0){
  			setEcho(1);
  		}else if(strncmp("echo 0",userInput,6)==0){
  			setEcho(0);
  		}else if(strncmp("echo ?",userInput,6)==0){
  			setEcho(-1);
  		}else{
					printf(ANSI_COLOR_WHITE " USAGE : echo 1/0/? :  " ANSI_COLOR_GREEN "Set/Unset echo" ANSI_COLOR_RESET "\n");
			}
  	}else if(strncmp("as",userInput,2)==0){
  		/* Send auto-start cmd */
  		if(strncmp("as 1",userInput,4)==0){
  			setAS(1);
  		}else if(strncmp("as 0",userInput,4)==0){
  			setAS(0);
  		}else if(strncmp("as ?",userInput,4)==0){
  			setAS(-1);
  		}else{
					printf(ANSI_COLOR_WHITE " USAGE : as 1/0/? :  " ANSI_COLOR_GREEN "Set/Unset auto-start" ANSI_COLOR_RESET "\n");
			}
  	}else if(strncmp("register",userInput,8)==0){
  		/* Send register cmd */
			if(strncmp("register 1",userInput,10)==0){
  			registerDevice(1);
  		}else if(strncmp("register 0",userInput,10)==0){
  			registerDevice(0);
  		}else if(strncmp("register ?",userInput,10)==0){
  			registerDevice(-1);
  		}else{
					printf(ANSI_COLOR_WHITE " USAGE : as 1/0/? :  " ANSI_COLOR_GREEN "Register/Unregister" ANSI_COLOR_RESET "\n");
			}
  	}else if(strncmp("reset",userInput,5)==0){
  		/* Send reset cmd */
			doReset();
  	}else if(strncmp("sendTest",userInput,8)==0){
  		/* Send test payload to Device */
			sendTest();
  	}else if(strncmp("sendN",userInput,5)==0){
  		/* Send with given payload size */
  		char* token;
			char* delim = " ";
			token = strtok(userInput, delim);
			token = strtok(NULL, delim);
			if(token!=NULL){
				int a = atoi(token);
				sendN(a);
			}else{
				printf(ANSI_COLOR_WHITE "USAGE : sendN #n# :    " ANSI_COLOR_GREEN "Send a payload with size=n to Device" ANSI_COLOR_RESET "\n");
			}
  	}else if(strncmp("send",userInput,4)==0){
  		/* Send reset cmd */
			char* token;
			char* delim = " ";
			token = strtok(userInput, delim);
			token = strtok(NULL, delim);
			if(token!=NULL){
				sendData(token);
			}else{
				printf(ANSI_COLOR_WHITE "USAGE : send #data# :    " ANSI_COLOR_GREEN "Send #data# (muste be hexa data) to Device" ANSI_COLOR_RESET "\n");
			}
  	}else {
  		clearScreen();
			printCmdHelp();
		}
  }
}

void writeProgram(){
	clearScreen();
	printHelpWrite();
	while(1){
		char inputUser[2046];
		fgets(inputUser,2046,stdin);
		if(strncmp("quit",inputUser,4)==0){
			/* quit write program */
			clearScreen();
			printTermMainHelp();
			break;
		}else{
			/* Send Command to device */
			int index = 0;
			for(;inputUser[index] != '\0';++index)
				if((inputUser[index] >= 'a')&&(inputUser[index]<= 'z'))
					inputUser[index]=inputUser[index]-32;
					
			inputUser[strlen(inputUser)-1]='\r';
			/*for(int i = 0;i<strlen(inputUser);++i){
				printf("%d : %d\n",i,inputUser[i]);
			}*/
			writeCmd(inputUser);
		}
	
	
	}
}

int searchForMax(){
	int ret = 0;
	for(int i = 2;;i+=2){
		sem_wait(&mutexError);
		if(errorReceived==1){
			printf("Error parsed in searchForMax Method\n");
			ret = i;
			errorReceived = 0;
			sem_post(&mutexError);
			return ret-4;
		}
		sem_post(&mutexError);
		char temp[2048];
		for(int y = 0; y<i;++y){
			temp[y]='a';
		}
		temp[i]='\n';
		temp[i+1]='\0';
		sendData(temp);
		sleep(1);

	}
	return ret;

}
int testPayloadCharge(int sizePayload){
	sizePayloadBasebandTest = sizePayload;
	int payloadQueued = 0;
	sem_wait(&mutexTime);
	timerShouldGo = 1;
	sem_post(&mutexTime);
	time_t t = time(NULL);
  struct tm tm;
  tm = *localtime(&t);
  fprintf(pFile_log,"* ");
  fprintf(pFile_log, "%d-%d-%d %d:%d:%d--> BEGIN BASEBAND TEST WITH SIZE=%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,sizePayload);		
	while(1){
		char buffer[119];
		hexaCounter(payloadQueued, buffer);
		char str[2048];
		sprintf(str,"AT+TX=\"%s\"\r",buffer);
		writeCmd(str);
		printf(ANSI_COLOR_WHITE "Trying to enqueue packet number :  " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n", payloadQueued);
		sleep(1);
		sem_wait(&mutexBusy);
		if(busyReceived == 1){
			printf(ANSI_COLOR_RED "[-] Packet " ANSI_COLOR_WHITE "%d" ANSI_COLOR_RED " NOT enqueued ! " ANSI_COLOR_RESET "\n", payloadQueued); 
		  sleep(5);
			busyReceived = 0;
			payloadQueued--;
		}else{
			printf(ANSI_COLOR_GREEN "[+] Packet " ANSI_COLOR_WHITE "%d" ANSI_COLOR_GREEN " Successfully enqueued ! " ANSI_COLOR_RESET "\n", payloadQueued);
			fprintf(pFile_log,"[+] ");
			t = time(NULL);
			tm = *localtime(&t);
  		fprintf(pFile_log, "%d-%d-%d %d:%d:%d--> Packet %d enqueued\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,payloadQueued);	
		}
		sem_post(&mutexBusy);
		payloadQueued++;
		if(payloadQueued > pow((double)sizePayloadBasebandTest,16.0))
			payloadQueued = 0;
		sem_wait(&mutexTime);
		if(endTime){
			endTime=0;
			sem_post(&mutexTime);
			break;
		}else{
			sem_post(&mutexTime);
		}
	}
	fprintf(pFile_log,"* ");
	t = time(NULL);
	tm = *localtime(&t);
  fprintf(pFile_log, "%d-%d-%d %d:%d:%d--> END BASEBAND TEST\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return payloadQueued-1;

}

void hexaCounter(int decimalNumber, char* finalBuffer){
	int quotient;
  int i=1,j,temp;
  char hexadecimalNumber[100];
  quotient = decimalNumber;
  while(quotient!=0) {
  	temp = quotient % 16;
    //To convert integer into character
    if( temp < 10){
    	temp =temp + 48;
    }else{
    	temp = temp + 55;
    }
    hexadecimalNumber[i++]= temp;
    quotient = quotient / 16;
  }

  for(int z = 0; z<=sizePayloadBasebandTest;++z){
  	finalBuffer[z]='0';
  }
 	for (j = i -1 ;j> 0;j--){
		//printf("buffer[%d] = %c\n",j,hexadecimalNumber[j]);
		finalBuffer[sizePayloadBasebandTest-j]=hexadecimalNumber[j];
	}
	finalBuffer[sizePayloadBasebandTest]='\0';


}

void createSemaphores(){
	sem_init(&mutexError, 0, 1);
	sem_init(&mutexBusy, 0, 1);
	sem_init(&mutexTime, 0, 1);
}

void destroySemaphores(){
  sem_destroy(&mutexTime);
  sem_destroy(&mutexError);
  sem_destroy(&mutexBusy);
}














