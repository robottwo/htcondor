
#include "condor_common.h"
#include "dc_socket_manager.h"
#include "generic_stats.h"
#include "reli_sock.h"
#include "condor_daemon_core.h"
#include "daemon_command.h"
#include "condor_threads.h"

extern void **curr_dataptr;
extern void **curr_regdataptr;


static void
registerFD(SockEnt &ent, Selector &selector)
{
	if (!ent.iosock) {return;}
	int fd = ent.iosock->get_file_desc();
	switch (ent.handler_type)
	{
		case HANDLE_READ:
			selector.add_fd(fd, Selector::IO_READ);
			break;
		case HANDLE_READ_WRITE:
			selector.add_fd(fd, Selector::IO_READ);
			// Fall through!
		case HANDLE_WRITE:
			selector.add_fd(fd, Selector::IO_WRITE);
			// Fall through!
		case HANDLE_NONE:
			break;
	}
}


static void
removeFD(SockEnt &ent, Selector &selector)
{
	if (!ent.iosock) {return;}
	int fd = ent.iosock->get_file_desc();
	switch (ent.handler_type)
	{
		case HANDLE_READ:
			selector.delete_fd(fd, Selector::IO_READ);
			break;
		case HANDLE_READ_WRITE:
			selector.delete_fd(fd, Selector::IO_READ);
			// Fall through!
		case HANDLE_WRITE:
			selector.delete_fd(fd, Selector::IO_WRITE);
			// Fall through!
		case HANDLE_NONE:
			break;
	}
}


int
SockManager::registerCommandSocket(Stream *iosock, const char* iosock_descrip,
	const char *handler_descrip, Service* s, DCpermission perm, HandlerType handler_type, int is_cpp, void **prev_entry)
{

	// In sockTable, unlike the others handler tables, we allow for a NULL
	// handler and a NULL handlercpp - this means a command socket, so use
	// the default daemon core socket handler which strips off the command.
	// SO, a blank table entry is defined as a NULL iosock field.
	int result = registerSocket(iosock, iosock_descrip, NULL, NULL, handler_descrip, s, perm, handler_type, is_cpp, prev_entry);

	if (result < 0) {return result;}

	m_command_socks.push_back(result);
	registerFD(m_socks[result], m_sel_comm);

	return result;
}


int
SockManager::registerSocket(Stream *iosock, const char* iosock_descrip,
	SocketHandler handler, SocketHandlercpp handlercpp, const char *handler_descrip,
	Service* service, DCpermission perm, HandlerType handler_type, int is_cpp, void **prev_entry)
{
	if ( prev_entry ) {
		*prev_entry = NULL;
	}

	if ( !iosock ) {
		dprintf(D_DAEMONCORE, "Can't register NULL socket \n");
		return -1;
	}

	// Find empty slot, set to be idx.
	unsigned idx=0;
	bool found_entry = false;
	for (std::vector<SockEnt>::iterator it=m_socks.begin(); it!=m_socks.end(); it++, idx++)
	{
		if ( it->iosock == NULL )
		{
			found_entry = true;
			break;
		}
		if ( it->remove_asap && (it->servicing_tid == 0) )
		{
			found_entry = true;
			it->iosock = NULL;
			break;
		}
	}
	if (!found_entry)
	{
		idx = m_socks.size();
		m_socks.push_back(SockEnt());
	}
	// NOTE: There previously was a check that m_socks[idx].iosock == NULL;
	// I have eliminated this until we seriously reconsider doing condor threads.

	if (handler_descrip == NULL) {handler_descrip = "UnnamedHandler";}
	daemonCore->dc_stats.NewProbe("Socket", handler_descrip, AS_COUNT | IS_RCT | IF_NONZERO | IF_VERBOSEPUB);

	// Verify that this socket has not already been registered
	// Since we are scanning the entire table to do this (change this someday to a hash!),
	// at the same time update our nRegisteredSocks count by initializing it
	// to the number of slots (nSock) and then subtracting out the number of slots
	// not in use.
	m_registered = m_socks.size();
	int fd_to_register = ((Sock *)iosock)->get_file_desc();
	unsigned idx2 = 0;
	bool duplicate_found = false;
	Stream *stream_to_register = m_socks[idx].iosock;
	for (std::vector<SockEnt>::iterator it=m_socks.begin(); it!=m_socks.end(); it++, idx2++)
	{       
		if ( it->iosock == stream_to_register )
		{
			idx = idx2;
			duplicate_found = true;
		}

		// fd may be -1 if doing a "fake" registration: reverse_connect_pending
		// so do not require uniqueness of fd in that case
		if ( it->iosock && fd_to_register != -1 )
		{
			if (static_cast<Sock *>(it->iosock)->get_file_desc() == fd_to_register)
			{
				idx = idx2;
				duplicate_found = true;
			}
		}

			// check if slot empty or available
		if ( (it->iosock == NULL) ||  // slot is empty
			(it->remove_asap && it->servicing_tid==0 ) )  // Slot is available.
		{
			m_registered--; // decrement count of active sockets
		}
	}
	if (duplicate_found) {
		if (prev_entry) {
			*prev_entry = new SockEnt();
			*static_cast<SockEnt*>(*prev_entry) = m_socks[idx];
		}
		else
		{
			dprintf(D_ALWAYS, "DaemonCore: Attempt to register socket twice\n");
			return -2;
		}
	}

			// Check that we are within the file descriptor safety limit
			// We currently only do this for non-blocking connection attempts because
			// in most other places, the code does not check the return value
			// from Register_Socket().  Plus, it really does not make sense to 
			// enforce a limit for other cases --- if the socket already exists,
			// DaemonCore should be able to manage it for you.

	if (iosock->type() == Stream::reli_sock && static_cast<ReliSock*>(iosock)->is_connect_pending())
	{
		MyString overload_msg;
		bool overload_danger = daemonCore->TooManyRegisteredSockets( static_cast<Sock *>(iosock)->get_file_desc(), &overload_msg);

		if (overload_danger)
		{
			dprintf(D_ALWAYS,
				"Aborting registration of socket %s %s: %s\n",
				iosock_descrip ? iosock_descrip : "(unknown)",
				handler_descrip ? handler_descrip : static_cast<Sock*>(iosock)->get_sinful_peer(),
				overload_msg.Value());
			return -3;
		}
	}

	SockEnt &ent = m_socks[idx];
	ent.servicing_tid = 0;
	ent.remove_asap = false;
	ent.call_handler = false;
	ent.iosock = static_cast<Sock *>(iosock);
	switch (iosock->type())
	{
		case Stream::reli_sock:
		{
			ReliSock &rsock = *static_cast<ReliSock*>(iosock);
			ent.is_connect_pending = rsock.is_connect_pending() && !rsock.is_reverse_connect_pending();
			ent.is_reverse_connect_pending = rsock.is_reverse_connect_pending();
			break;
		}
		case Stream::safe_sock:
			ent.is_connect_pending = false;
			ent.is_reverse_connect_pending = false;
			break;
		default:
			EXCEPT("Adding CEDAR socket of unknown type");
			break;
	}
	ent.handler = handler;
	ent.handlercpp = handlercpp;
	ent.is_cpp = is_cpp;
	ent.perm = perm;
	ent.handler_type = handler_type;
	ent.service = service;
	ent.data_ptr = NULL;
	ent.waiting_for_data = false;
	ent.iosock_descrip = iosock_descrip ? iosock_descrip : static_cast<Sock*>(iosock)->get_sinful_peer();
	ent.handler_descrip = handler_descrip ? handler_descrip : "Unknown Handler";
	if (ent.handler_descrip == DaemonCommandProtocol::WaitForSocketDataString)
	{
		ent.waiting_for_data = true;
	}

	// Update curr_regdataptr for SetDataPtr()
	curr_regdataptr = &(ent.data_ptr);

	// Conditionally dump what our table looks like
	dump(D_FULLDEBUG | D_DAEMONCORE, "");

	// If we are a worker thread, wake up select in the main thread
	// so the main thread re-computes the fd_sets.
	daemonCore->Wake_up_select();

	registerFD(ent, m_sel_all);

	return static_cast<int>(idx);
}


int
SockManager::cancelSocket(Stream* insock, void *prev_entry)
{
	if (!insock) {return false;}

	int idx=0, idx2 = -1;
	for (std::vector<SockEnt>::const_iterator it=m_socks.begin(); it!=m_socks.end(); it++, idx++)
	{
		if (it->iosock == insock)
		{
			idx2 = idx;
			break;
		}
	}
	if (idx2 == -1)
	{
		dprintf( D_ALWAYS,"Cancel_Socket: called on non-registered socket!\n");
		if (insock)
		{
			dprintf(D_ALWAYS,"Offending socket number %d to %s\n",
				static_cast<Sock*>(insock)->get_file_desc(),
				insock->peer_description());
		}
		dump(D_DAEMONCORE, "");
		return false;
	}
	SockEnt &ent = m_socks[idx];

		// Clear any data_ptr which go to this entry we just removed
	if (curr_regdataptr == &(ent.data_ptr)) {curr_regdataptr = NULL;}
	if (curr_dataptr == &(ent.data_ptr)) {curr_dataptr = NULL;}

	if ((ent.servicing_tid == 0) || ent.servicing_tid == CondorThreads::get_handle()->get_tid() || prev_entry)
	{
		dprintf(D_DAEMONCORE,"Cancel_Socket: cancelled socket %d <%s> %p\n",
			idx, ent.iosock_descrip.c_str(), ent.iosock);
		// Remove entry; mark it is available for next add via iosock=NULL
		if (prev_entry)
		{
			SockEnt &prev_entry_ref = *static_cast<SockEnt *>(prev_entry);
			prev_entry_ref.servicing_tid = ent.servicing_tid;
			ent = prev_entry_ref;
		}
		else
		{
			removeFD(ent, m_sel_all);
			if (ent.is_command_sock) {removeFD(ent, m_sel_comm);}
			ent.iosock = NULL;
		}
	}
	else
	{
		dprintf(D_DAEMONCORE,"Cancel_Socket: deferred cancel socket %d <%s> %p\n",
			idx, ent.iosock_descrip.c_str(), ent.iosock);
		ent.remove_asap = true;
	}

	if (!prev_entry)
	{
		m_registered--;
	}

	dump(D_FULLDEBUG | D_DAEMONCORE, "");

	// If we are a worker thread, wake up select in the main thread
	// so the main thread re-computes the fd_sets.
	daemonCore->Wake_up_select();

	return true;
}


bool
SockManager::isRegistered(Stream *stream)
{
	for (std::vector<SockEnt>::const_iterator it=m_socks.begin(); it!=m_socks.end(); it++)
	{
		if (stream == it->iosock) {return true;}
	}
	return false;
}

