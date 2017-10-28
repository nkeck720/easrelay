#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define CALLSIGN "WABC/TV "
int numRecievedCodes = 0;

/*
 * easrelay - a simple EAS relay program using minimodem to encode/decode
 */

int main(int argc, char **argv) {
  printf("easrelay v1.0\n");
  fflush(stdout);
  /* Get the FIPS code(s) that the program needs to monitor from the command line */
  if(argc == 1) {
    fprintf(stderr, "Incorrect number of args, must specify FIPS codes to monitor.\n");
    exit(EXIT_FAILURE);
  }
  /* make an array for the FIPS codes that need to be monitored for */
  char *fipsCodes[argc];
  for(int i = argc; i > 0; i--) {
    fipsCodes[i] = argv[i];
  }
  int numFipsCodes = argc - 1;
  printf("Now monitoring FIPS codes: ");
  for(int i = numFipsCodes; i >= 0; i--) {
    printf("%s ", fipsCodes[i]);
  }
  printf("\n");
  fflush(stdout);
  /* Set up a pipe and get minimodem running */
  FILE *monitorInput = popen("minimodem --rx same 2> /dev/null", "r");
  /* Loop getting input waiting for ZCZC */
  char charIn;
  char originatorCode[4];
  originatorCode[3] = '\0';
  char alertCode[4];
  alertCode[3] = '\0';
  char timeCode[5];
  timeCode[4] = '\0';
  char dayIssued[4], hourIssued[3], minuteIssued[3];
  dayIssued[3] = '\0';
  hourIssued[2] = '\0';
  minuteIssued[2] = '\0';
  char **recievedFipsCodes = (char **)malloc(31 * sizeof(char *));
  if(!recievedFipsCodes) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(EXIT_FAILURE);
  }
  for(int i = 0; i < 31; i++) {
    recievedFipsCodes[i] = (char *)malloc(7 * sizeof(char));
    if(!recievedFipsCodes[i]) {
      fprintf(stderr, "Unable to allocate memory\n");
    }
  }
  for(int i = 0; i < 31; i++) {
    recievedFipsCodes[i][6] = '\0';
  }
  bool gettingFipsCodes = false;
  while(true) {
    fscanf(monitorInput, "%c", &charIn);
    if(charIn == 'Z') {
      fscanf(monitorInput, "%c", &charIn);
      if(charIn != 'C')
	continue;
      fscanf(monitorInput, "%c", &charIn);
      if(charIn != 'Z')
	continue;
      fscanf(monitorInput, "%c", &charIn);
      if(charIn != 'C')
	continue;
      /* 
       * If here then we have an alert message. Check to see the type of alert 
       * (NWS, PEP, etc.)
       */
      printf("Alert incoming:\n");
      /* Get rid of the - */
      fscanf(monitorInput, "%c", &charIn);
      for(int i = 0; i < 3; i++) {
	fscanf(monitorInput, "%c", &originatorCode[i]);
      }
      if((strcmp(originatorCode, "PEP")) == 0) {
	printf("Initiated by Primary Entry Point\n");
      }
      else if((strcmp(originatorCode, "CIV")) == 0) {
	printf("Initiated by Civil Authorities\n");
      }
      else if((strcmp(originatorCode, "WXR")) == 0) {
	printf("Initiated by National Weather Service\n");
      }
      else if((strcmp(originatorCode, "EAS")) == 0) {
	printf("Initaited by EAS Participant\n");
      }
      else if((strcmp(originatorCode, "EAN")) == 0) {
	printf("\x1b[31m" "EMERGENCY ACTION NOTIFICATION NETWORK\n" "\x1b[0m");
      }
      else {
	printf("Invalid originator code (not relaying)\n");
	continue;
      }
      /* Next - to get rid of */
      fscanf(monitorInput, "%c", &charIn);
      /* Get the alert code */
      for(int i = 0; i < 3; i++) {
	fscanf(monitorInput, "%c", &alertCode[i]);
      }
      /* If EAN then needs immediate retransmission */
      bool relayingMessage = false;
      if(strcmp("EAN", alertCode) == 0) {
	printf("\x1b[31m" "EAN RECIEVED, NEEDS IMMEDIATE RETRANSMISSON!\n" "\x1b[0m");
	relayingMessage = true;
      }
      printf("Alert code: %s\n", alertCode);
      /* Next dash */
      fscanf(monitorInput, "%c", &charIn);
      /* Get the fips codes in a loop */
      if(strcmp(alertCode, "NPT") == 0) {
	relayingMessage == true;
	printf("National EAS test in progress, relaying\n");
	fflush(stdout);
      }
      printf("FIPS codes issued to:\n");
      int fipsIndex = 0;
      int charIndex = 1;
      fscanf(monitorInput, "%c", &recievedFipsCodes[0][0]);
      do {
	switch(recievedFipsCodes[0][0]) {
	case '1':
	  gettingFipsCodes = true;
	  if(relayingMessage == false) {
	    printf("Got full-county code, relaying\n");
	    relayingMessage = true;
	  }
	case '0':
	  /* Get the code in a for loop */
	  gettingFipsCodes = true;
	  for(int i = charIndex; i < 6; i++) {
	    fscanf(monitorInput, "%c", &recievedFipsCodes[fipsIndex][i]);
	  }
	  break;
	default:
	  gettingFipsCodes = false;
	  /* Get the first char of the next field back out of there */
	  
	  break;
	}
	recievedFipsCodes[fipsIndex][6] = '\0';
	  /* Check to see if the FIPS code is one that we need to relay */
	  for(int i = 1; i < argc; i++) {
	    if(strcmp(&recievedFipsCodes[fipsIndex][0], argv[i]) == 0) {
	      printf("(will be relayed) ");
	      relayingMessage = true;
	    }
	  }
	  printf("%s, ", recievedFipsCodes[fipsIndex]);
	  fipsIndex++;
	  charIndex = 0;
	  /* Get the dash in and see if the next one is a FIPS code */
	  fscanf(monitorInput, "%c", &charIn);
	  if(charIn == '+') {
	    gettingFipsCodes = false;
	    numRecievedCodes = fipsIndex - 1;
	  }
	  else {
	    fscanf(monitorInput, "%c", &charIn);
	    recievedFipsCodes[fipsIndex][charIndex] = charIn;
	    charIndex++;
	  }
      } while(gettingFipsCodes == true);
      /* Get the time code */
      for(int i = 0; i < 4; i++) {
	fscanf(monitorInput, "%c", &timeCode[i]);
      }
      printf("Time to purge: +%s\n", timeCode);
      fflush(stdout);
      /* Get time of issue */
      fscanf(monitorInput, "%c", &charIn);
      for(int i = 0; i < 3; i++) {
	fscanf(monitorInput, "%c", &dayIssued[i]);
      }
      for(int i = 0; i < 2; i++) {
	fscanf(monitorInput, "%c", &hourIssued[i]);
      }
      for(int i = 0; i < 2; i++) {
	fscanf(monitorInput, "%c", &minuteIssued[i]);
      }
      printf("Time issued: Day %s, %s:%s UTC\n", dayIssued, hourIssued, minuteIssued);
      fflush(stdout);
      fscanf(monitorInput, "%c", &charIn);
      /* Wait until the last dash is reached */
      do {
	fscanf(monitorInput, "%c", &charIn);
      } while(charIn != '-');
      /* See if the message needs to be relayed */
      if(strcmp(alertCode, "RWT") == 0) {
	relayingMessage == false;
	printf("Not relaying a weekly test.\n");
	fflush(stdout);
      }
      if(relayingMessage == false) {
	continue;
      }
      /* Open a new modem to relay the message with */
      FILE *monitorOut = popen("minimodem --tx same", "w");
      /* Construct the message we need to send */
      for(int i = 0; i < 3; i++) {
	fprintf(monitorOut, "ZCZC-%s-%s", originatorCode, alertCode);
	for(int i = 0; i < numRecievedCodes; i++) {
	  fprintf(monitorOut, "-%s", recievedFipsCodes[i]);
	}
	/* Transmit the time code now, and then date and callsign */
	fprintf(monitorOut, "+%s-%s%s%s-%s-", timeCode, dayIssued, hourIssued, minuteIssued, CALLSIGN);
	fflush(monitorOut);
	sleep(2);
      }
      /* Enable the audio now */
      pid_t pid = fork();
      if(pid == 0) { 
        static char *argv[] = {"dd","if=/dev/dsp","of=/dev/dsp",NULL};
        execvp("dd",argv);
        exit(127); /* only if execv fails */
      }
      /* Wait for the NNNN footer to come in */
    wait_for_footer:
      do {
	fscanf(monitorInput, "%c", &charIn);
      } while(charIn != 'N');
      /* See if the next one is also an N, if not keep waiting */
      fscanf(monitorInput, "%c", &charIn);
      if(charIn != 'N') {
	goto wait_for_footer;
      }
      /* kill the audio and then send the footer */
      kill(pid, SIGKILL);
      for(int i = 0; i < 3; i++) {
	fprintf(monitorOut, "NNNN");
	fflush(monitorOut);
	sleep(1);
      }
      pclose(monitorOut);
      printf("Alert Ended\n");
      fflush(stdout);
    }
  }
}
