#ifndef MYTIME_H
#define MYTIME_H

#include <stdio.h>
#include <stdlib.h>
 
int mytime(int left, int right) {
	return (left + rand() % (right - left));	
	// printf("random time is %d sec\n", time);
}

#endif