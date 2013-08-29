#! /usr/bin/env perl
##**************************************************************
##
## Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
## University of Wisconsin-Madison, WI.
## 
## Licensed under the Apache License, Version 2.0 (the "License"); you
## may not use this file except in compliance with the License.  You may
## obtain a copy of the License at
## 
##    http://www.apache.org/licenses/LICENSE-2.0
## 
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
##**************************************************************

use CondorTest;

$cmd      = 'job_schedd_restart-check_reconnect.cmd';
$testdesc =  'Verify held jobs are preserved on schedd restarts';
$testname = "job_schedd_restart-check_reconnect";

$beforequeue = "job_schedd_restart-holdjobs.before";
$afterqueue = "job_schedd_restart-holdjobs.after";

# truly const variables in perl
sub IDLE{1};
sub HELD{5};
sub RUNNING{2};

my $alreadydone=0;
my $disconnectedcount = 0;
my $reconnectedcount = 0;
my $reconnectfailedcount = 0;

my $submithandled = "no";

$aborted = sub
{
	CondorTest::debug("Aborting expected\n",1);
};

$disconnected = sub
{
	$disconnectedcount += 1;
	CondorTest::debug("Disconnect expected count now $disconnectedcount\n",1);
};

$reconnected = sub
{
	$reconnectedcount += 1;
	CondorTest::debug("Reconnect expected count now $reconnectedcount\n",1);
};

$reconnectfailed = sub
{
	$reconnectfailedcount += 1;
	CondorTest::debug("Reconnect Failure Not expected count now $reconnectfailedcount\n",1);
};


$executed = sub
{
	my %args = @_;
	my $cluster = $args{"cluster"};
	my $doneyet = "no";

	if($submithandled eq "no") {
		$submithandled = "yes";

		CondorTest::debug("Exectuting\n",1);
		CondorTest::debug("Collecting queue details on $cluster\n",1);
		my @adarray;
		my $status = 1;
		my $cmd = "condor_q $cluster";
		$status = CondorTest::runCondorTool($cmd,\@adarray,2);
		if(!$status)
		{
			CondorTest::debug("Test failure due to Condor Tool Failure<$cmd>\n",1);
			exit(1)
		}

		#open(BEFORE,">$beforequeue") || die "Could not ope file for before stats $!\n";
		#foreach my $line (@adarray)
		#{
			#CondorTest::debug("$line\n",1);
			#print BEFORE "$line\n";
		#}
		#close(BEFORE);

		$status = CondorTest::changeDaemonState( "schedd", "off", 9 );
		if(!$status)
		{
			CondorTest::debug("Test failure: could not turn scheduler off!\n",1);
			exit(1)
		}

		$status = CondorTest::changeDaemonState( "schedd", "on", 9 );
		if(!$status)
		{
			CondorTest::debug("Test failure: could not turn scheduler on!\n",1);
			exit(1)
		}

		CondorTest::debug("Collecting queue details on $cluster\n",1);
		my @fdarray;
		my $status = 1;
		my $cmd = "condor_q $cluster";
		$status = CondorTest::runCondorTool($cmd,\@fdarray,2);
		if(!$status)
		{
			CondorTest::debug("Test failure due to Condor Tool Failure<$cmd>\n",1);
			exit(1)
		}

		$cmd = "condor_rm $cluster";
		CondorTest::debug("removing reconnected jobs <$cmd>\n",1);
		$status = CondorTest::runCondorTool($cmd,\@fdarray,2);
		if(!$status)
		{
			CondorTest::debug("Test failure due to Condor Tool Failure<$cmd>\n",1);
			exit(1)
		}

		if($reconnectedcount != $disconnectedcount) {
			die "Not all jobs reconnnected\n";
		}
	}
};


my $on_evictedwithoutcheckpoint = sub {
	CondorTest::debug("Evicted Without Checkpoint from removing jobs.\n",1);
};

CondorTest::RegisterEvictedWithoutCheckpoint($testname, $on_evictedwithoutcheckpoint);

CondorTest::RegisterExecute( $testname, $executed );
CondorTest::RegisterAbort( $testname, $aborted );
CondorTest::RegisterDisconnected( $testname, $disconnected );
CondorTest::RegisterReconnected( $testname, $reconnected );
CondorTest::RegisterReconnectFailed( $testname, $reconnectfailed );

if( CondorTest::RunTest($testname, $cmd, 0) ) {
	CondorTest::debug("$testname: SUCCESS\n",1);
	exit(0);
} else {
	die "$testname: CondorTest::RunTest() failed\n";
}

