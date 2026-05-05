#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include "stm32f4xx_hal.h"

int _write(int file, char *ptr, int len) {
	(void)file;
	  int DataIdx;

	  for (DataIdx = 0; DataIdx < len; DataIdx++)
	  {
	    ITM_SendChar(*ptr++);
	  }
	  return len;
}
