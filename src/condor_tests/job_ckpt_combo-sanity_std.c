/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
  *
  * Condor Software Copyright Notice
  * Copyright (C) 1990-2004, Condor Team, Computer Sciences Department,
  * University of Wisconsin-Madison, WI.
  *
  * This source code is covered by the Condor Public License, which can
  * be found in the accompanying LICENSE.TXT file, or online at
  * www.condorproject.org.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * AND THE UNIVERSITY OF WISCONSIN-MADISON "AS IS" AND ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  * WARRANTIES OF MERCHANTABILITY, OF SATISFACTORY QUALITY, AND FITNESS
  * FOR A PARTICULAR PURPOSE OR USE ARE DISCLAIMED. THE COPYRIGHT
  * HOLDERS AND CONTRIBUTORS AND THE UNIVERSITY OF WISCONSIN-MADISON
  * MAKE NO MAKE NO REPRESENTATION THAT THE SOFTWARE, MODIFICATIONS,
  * ENHANCEMENTS OR DERIVATIVE WORKS THEREOF, WILL NOT INFRINGE ANY
  * PATENT, COPYRIGHT, TRADEMARK, TRADE SECRET OR OTHER PROPRIETARY
  * RIGHT.
  *
  ****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/

 


#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define NUMTESTS 500

#ifdef __cplusplus
extern "C" {
#endif

extern void ckpt_and_exit();
void loop(){};

int main( int argc, char **argv )
{
	int i, j;
	register int k;
	char buf[ 1024 ];
	FILE	*fp;
	void *curbrk, *oldbrk;

	/*
	DebugFlags = D_CKPT;
	*/

	if( (fp=fopen( "job_ckpt_combo-sanity_std.results", "w" )) == NULL ) {
		perror( "job_ckpt_combo-sanity_std.results" );
		exit( 1 );
	}

	oldbrk = sbrk(0);

	for( i = 0; i < NUMTESTS; i++ ) {
		sprintf(buf, "set i = %d\n", i);
		if (write(2, buf, strlen(buf)) == -1)
		{
			printf("WHAT?!? Write failed on fd[%d]\n", fileno(fp));
			exit(EXIT_FAILURE);
		}

		rewind( fp );
		if (fprintf( fp, "set i = %d\n", i ) == -1)
		{
				printf("WHAT?!? fprintf failed on fd[%d]\n", fileno(fp));
				exit(EXIT_FAILURE);
		}
		fflush( fp );

		curbrk = sbrk(0);
		if( oldbrk != curbrk ) {
			printf("Current break is now 0x%x, old was 0x%x (set i = %d)\n",
				curbrk, oldbrk, i);
		}

		oldbrk = curbrk;

		for(k=j=0; j < 100000; k++,j++ );
		if( j != k ) {
			sprintf(buf, "j = %d, k (register) = %d\n", j, k);
			if (write(2, buf, strlen(buf)) == -1)
			{
				printf("WHAT?!? Write failed on fd[%d]\n", fileno(fp));
				exit(EXIT_FAILURE);
			}
		}

		ckpt_and_exit();
	}
	fclose(fp);
	exit( 0 );
}

#ifdef __cplusplus
}
#endif
