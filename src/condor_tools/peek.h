/***************************************************************
 *
 * Copyright (C) 2017, Condor Team, Computer Sciences Department,
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


#include "dc_starter.h"
#include "proc.h"

#include <cstdio>
#include <string>
#include <vector>

class DCTransferQueue;

namespace htcondor {

class HTCondorPeek : public PeekGetFD
{
public:
	HTCondorPeek() :
		m_transfer_stdout(true),
		m_transfer_stderr(false),
		m_auto_retry(false),
		m_retry_sensible(true),
		m_follow(false),
		m_success(false),
		m_max_bytes(1024),
		m_stdout_offset(-1),
		m_stderr_offset(-1),
		m_xfer_q(NULL)
	{
		m_id.cluster = -1;
		m_id.proc = -1;
	}

	virtual ~HTCondorPeek();

	/**
	 * Parse command-line arguments.
	 *
	 * This will configure the object appropriately as if it was
	 * invoked from the command line tool; this *may* call exit() directly,
	 * making it inappropriate for invoking from a library.
	 */
	bool parse_args(int argc, char *argv[]);
	void usage(const char *);

	/**
 	* Execute the `peek` request.
 	*/
	bool execute();

	/**
	 * Returns the file descriptor where the contents of the remote file
	 * should be written.
	 *
	 * This class always writes to stdout - but that can be overridden by sub-classes.
	 */
	virtual int getNextFD(const std::string &) {return 1;}

	/**
	 * Various setters for the object's attributes; use these when
	 * invoking from a library context.
	 */
	void setPool(const std::string &pool) {m_pool = pool;}
	void setName(const std::string &name) {m_name = name;}
	void setAddr(const std::string &addr) {m_addr = addr;}
	void setJobId(PROC_ID id) {m_id = id;}
	void addFiles(const std::vector<std::string> &filenames);
	void setTransferStdout(bool doXfer) {m_transfer_stdout = doXfer;}
	void setTransferStderr(bool doXfer) {m_transfer_stderr = doXfer;}

private:
	bool execute_peek();
	bool execute_peek_with_session();
	bool create_session();
	bool get_transfer_queue_slot();
	void release_transfer_queue_slot();

	std::string m_pool;
	std::string m_name;
	std::string m_addr;
	PROC_ID m_id;
	std::vector<std::string> m_filenames;
	std::vector<ssize_t> m_offsets;
	bool m_transfer_stdout;
	bool m_transfer_stderr;
	bool m_auto_retry;
	bool m_retry_sensible;
	bool m_follow;
	bool m_success;
	size_t m_max_bytes;
	ssize_t m_stdout_offset;
	ssize_t m_stderr_offset;
	std::string m_sec_session_id;
	std::string m_starter_addr;
	std::string m_starter_version;
	DCTransferQueue *m_xfer_q;
};

}
