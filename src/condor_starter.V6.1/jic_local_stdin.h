/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
 * CONDOR Copyright Notice
 *
 * See LICENSE.TXT for additional notices and disclaimers.
 *
 * Copyright (c)1990-2002 CONDOR Team, Computer Sciences Department, 
 * University of Wisconsin-Madison, Madison, WI.  All Rights Reserved.  
 * No use of the CONDOR Software Program Source Code is authorized 
 * without the express consent of the CONDOR Team.  For more information 
 * contact: CONDOR Team, Attention: Professor Miron Livny, 
 * 7367 Computer Sciences, 1210 W. Dayton St., Madison, WI 53706-1685, 
 * (608) 262-0856 or miron@cs.wisc.edu.
 *
 * U.S. Government Rights Restrictions: Use, duplication, or disclosure 
 * by the U.S. Government is subject to restrictions as set forth in 
 * subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer 
 * Software clause at DFARS 252.227-7013 or subparagraphs (c)(1) and 
 * (2) of Commercial Computer Software-Restricted Rights at 48 CFR 
 * 52.227-19, as applicable, CONDOR Team, Attention: Professor Miron 
 * Livny, 7367 Computer Sciences, 1210 W. Dayton St., Madison, 
 * WI 53706-1685, (608) 262-0856 or miron@cs.wisc.edu.
****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/

#if !defined(_CONDOR_JIC_LOCAL_STDIN_H)
#define _CONDOR_JIC_LOCAL_STDIN_H

#include "jic_local_config.h"

/** 
	This is the child class of JICLocal (and therefore
	JobInfoCommunicator) that deals with running "local" jobs and
	getting the job ClassAd info from both STDIN and out of the config
	file.  There are two constructors, one where we take a string, one
	where we don't.  The string is the optional keyword we got passed
	on the command line to prepend to all the attributes we want to
	look up which aren't in the classad that's written to our STDIN.
	If there's no keyword, either the job ClassAd on STDIN should have
	all the attributes we need, or it should define the keyword
	itself.
*/

class JICLocalStdin : public JICLocalConfig {
public:

		/// Constructor with a keyword on the command-line
	JICLocalStdin( const char* keyword, int cluster, int proc, 
				   int subproc );

		/// Constructor without a keyword on the command-line
	JICLocalStdin( int cluster, int proc, int subproc );

		/// Destructor (does nothing interesting)
	virtual ~JICLocalStdin();

		/** This is the version of getLocalJobAd that knows to try to
			read from STDIN to get the job ClassAd.  If there's a config
			keyword (either on the command-line or in the ClassAd on
			STDIN, we also try calling JICLocalConfig::getLocalJobAd()
			to read in any attributes we might need from the config
			file.  This allows users to define most of the job in the
			config file, and only specify some of the attributes in
			the ClassAd which is passed dynamically.  The attributes
			in the dynamic ClassAd always take precedence over ones in
			the config file.
		*/
	bool getLocalJobAd( void );

private:

		/** This version first checks the ClassAd we got from STDIN
			before looking in the config file
		*/
	bool getUniverse( void );

		/** Private helper to actually read STDIN and try to insert it
			into our job ClassAd.
		*/
	bool readStdinClassAd( void );

};


#endif /* _CONDOR_JIC_LOCAL_STDIN_H */
