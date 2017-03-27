/***************************************************************
 *
 * Copyright (C) 2013, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/


#include "peek.h"
#include "condor_version.h"

#include <stdio.h>

void
usage( char *cmd )
{
	fprintf(stderr,"Usage: %s [options] <job-id> [filename1[, filename2[, ...]]]\n",cmd);
	fprintf(stderr,"Tail a file in a running job's sandbox.\n");
	fprintf(stderr,"The options are:\n");
	fprintf(stderr,"    -help             Display options\n");
	fprintf(stderr,"    -version          Display HTCondor version\n");
	fprintf(stderr,"    -pool <hostname>  Use this central manager\n");
	fprintf(stderr,"    -name <name>      Query this schedd\n");
	fprintf(stderr,"    -debug            Show extra debugging info\n");
	fprintf(stderr,"    -maxbytes         The maximum number of bytes to transfer.  Defaults to 1024\n");
	fprintf(stderr,"    -auto-retry       Auto retry if job is not running yet.\n");
	fprintf(stderr,"    -f,-follow        Follow the contents of a file.\n");
	fprintf(stderr,"By default, only stdout is returned.\n");
	fprintf(stderr,"The filename may be any file in the job's transfer_output_files.\n");
	fprintf(stderr,"You may also include the following options.\n");
	fprintf(stderr,"    -no-stdout        Do not tail the job's stdout\n");
	fprintf(stderr,"    -stderr           Tail at the job's stderr\n\n");
}

int
main(int argc, char *argv[])
{
	htcondor::HTCondorPeek peek;
	if (!peek.parse_args(argc, argv))
	{
		return 1;
	}
	if (!peek.execute())
	{
		return 1;
	}
	return 0;
}
