
#ifndef _DC_SOCKET_MANAGER_H_
#define _DC_SOCKET_MANAGER_H_

#include <string>
#include "dc_typedefs.h"
#include "sock.h"
#include "selector.h"

struct SockEnt
{
	Sock*                   iosock;
	SocketHandler           handler;
	SocketHandlercpp        handlercpp;
	Service*                service;
	std::string		iosock_descrip;
	std::string		handler_descrip;
	void*                   data_ptr;
	DCpermission            perm;
	bool                    is_cpp;
	bool                    is_connect_pending;
	bool                    is_reverse_connect_pending;
	bool                    call_handler;
	bool                    waiting_for_data;
	bool                    remove_asap;    // remove when being_serviced==false
	HandlerType             handler_type;
	int                             servicing_tid;  // tid servicing this socket
	bool            is_command_sock;

	SockEnt()
	: iosock(NULL),
	handler(NULL),
	handlercpp(NULL),
	service(NULL),
	iosock_descrip(NULL),
	handler_descrip(NULL),
	data_ptr(NULL),
	perm(FIRST_PERM),
	is_cpp(false),
	is_connect_pending(false),
	is_reverse_connect_pending(false),
	call_handler(false),
	waiting_for_data(false),
	remove_asap(false),
	handler_type(HANDLE_NONE),
	servicing_tid(0),
	is_command_sock(false)
	{}

	~SockEnt()
	{}
};

class SockManager
{
public:
	SockManager() :
	m_registered(0),
	m_pending(0)
	{
	}

	unsigned getSockCount() const {return m_command_socks.size() + m_socks.size();}
	unsigned getRegisteredSockCount() const {return m_registered;}
	unsigned getPendingSockCount() const {return m_pending;}

	void initialize(unsigned new_size)
	{
		m_socks.resize(new_size);
		m_registered = 0;
		m_pending = 0;
	}

	std::vector<SockEnt> &getSockTable();

	void cleanup()
	{
		for (std::vector<SockEnt>::iterator it = m_command_socks.begin(); it!= m_command_socks.end(); it++)
		{
			if (it->remove_asap && it->servicing_tid==0)
			{
				m_command_socks.erase(it);
			}
		}
		cleanup(m_socks);
	}

	Sock *getInitialCommandSocket()
	{
		for(std::vector<SockEnt>::const_iterator it=m_command_socks.begin(); it != m_socks.end(); it++)
		{
			if ( (it->iosock != NULL) && (it->is_command_sock) )
			{
			return it->iosock;
			}
		}
		return NULL;
	}

	const Selector &getCommandSelector()
	{
		return m_sel_comm;
	}

	const Selector &getSelector()
	{
		return m_sel_all;
	}

	void executeCommandSelector()
	{
		m_sel_comm.execute();
	}

	void execute()
	{
		m_sel_all.execute();
	}

	int getReadyCommandSocket() // TODO: Find the next ready socket in the selector, cleaning up as necessary!
	{
                                        // If the slot in sockTable just got removed, make sure we exit the loop
/*                                if ( ((*sockTable)[i].iosock == NULL) ||  // slot is empty
                                         ((*sockTable)[i].remove_asap &&           // slot available
                                          (*sockTable)[i].servicing_tid==0 ) ) {
                                        break;
                                }
*/	
		return 0;
	}

		// Return the socket table entry # of a socket which is either ready or
		// has a deadline, and that deadline has passed.
	int getReadySocket()
	{
		return 0;
	}

	SockEnt &getSocket(int idx)
	{
		return m_socks[idx];
	}

	void set_timeout(int timeout)
	{
		m_sel_all.set_timeout(timeout);
	}

	void set_command_timeout(int timeout)
	{
		m_sel_comm.set_timeout(timeout);
	}

	void dump(int flag, const char *indent);

	// Iterate through the socket table and remove them from the 
	void updateSelector();

	void watchFDs(std::vector<std::pair<int, HandlerType> >);
	void removeFDs(std::vector<std::pair<int, HandlerType> >);

	void incPending() {m_pending++;}
	void decPending() {m_pending--;}
	unsigned registeredSocketCount() {return m_pending + m_registered;}

	void registerCommandSocket(Stream *iosock, const char* iosock_descrip, const char *handler_descrip, Service* s, DCpermission perm, HandlerType handler_type, int is_cpp, void **prev_entry);
	void registerSocket(Stream *iosock, const char* iosock_descrip, SocketHandler handler, SocketHandlercpp handlercpp, const char *handler_descrip, Service* s, DCpermission perm, HandlerType handler_type, int is_cpp, void **prev_entry);
	int cancelSocket(Stream* insock, void *prev_entry);
	bool isRegistered(Stream* stream);

	std::vector<int>::iterator begin();
	std::vector<int>::iterator end();

private:
	void cleanup(std::vector<SockEnt> &table)
	{
		for (std::vector<SockEnt>::iterator it = table.begin(); it != table.end(); it++)
		{
			if (it->remove_asap && (0 == it->servicing_tid)) {table.erase(it);}
		}
	}

	unsigned m_registered; // number of sockets registered, always < getSockCount()
	unsigned m_pending; // number of sockets waiting on timers or any other callbacks
		// Note that command sockets have a concept of the "initial" socket; hence,
		// this is the only ordered table we keep.
	std::vector<SockEnt> m_command_socks;
	std::vector<SockEnt> m_socks;

	Selector m_sel_comm;
	Selector m_sel_all;
};

#endif

