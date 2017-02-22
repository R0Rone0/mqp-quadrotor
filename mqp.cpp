#include <unistd.h>
#include <iostream>
#include <map>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <fstream>
#include <string>

#include "util.h"
#include "mqpipc.h"
#include "mqpif.h"

////////////////////////////////////////////////////////////////////////////////
void startListening(int port);
void run();
void shutdown();
void buildPollArray();
void doConnect();
bool readOneMsg(const int socket);
void doOneMsg(const int socket);
void doCameraMsg(const int socket);
void doLidarMsg(const int socket);

////////////////////////////////////////////////////////////////////////////////
static int		listen_socket_inet	= -1;

typedef std::map<int, time_t>		ConnMap;
typedef ConnMap::iterator			ConnIterator;

static ConnMap			conn_map;
static int				npoll			= 0;
static struct pollfd	*poll_fds		= 0;
static int				maxpoll			= 128;

static time_t	t_now			= 0;

////////////////////////////////////////////////////////////////////////////////
static char				msg_type;
static char				*msg			= 0;
static int				msg_len			= 0;
static int				max_msg_len		= 0;

////////////////////////////////////////////////////////////////////////////////
//
int main(int argc, char *argv[]) {
extern char	*optarg;
int			err	= 0;
int			c;
int			port = 9999;
int			debug = 0;
int			verbose = 0;

	while ((c = getopt(argc, argv, "p:dv")) != -1) {
		switch (c) {
			case 'p' :	port		= atoi(optarg);		break;
			case 'd' :	debug++;						break;
			case 'v' :	verbose++;						break;
			default :	err++;							break;
		}
	}

	if (err) {
		std::cerr << "Usage : " << argv[0] << " -p port\n";
		return -1;
	}

	doLog("Starting");

	try {
		// Start listening for connections
		startListening(port);

		run();
	}
	catch (const char *n_err) {
		doLog(n_err);
		doLog("errno=%d", errno);
	}
	catch (...) {
		doLog("Internal Error");
		doLog("errno=%d", errno);
	}

	shutdown();

	doLog("Exiting");

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
void startListening(int port)
{
		// Create a INET type socket.
	if ((listen_socket_inet = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
		throw "Unable to create INET listen socket";

		// Build addr info
	struct sockaddr_in	in_addr;
	in_addr.sin_family		= AF_INET;
	in_addr.sin_addr.s_addr	= INADDR_ANY;
	in_addr.sin_port		= htons(port);

		// Bind to port
	if (::bind(listen_socket_inet, (struct sockaddr *)&in_addr,
										sizeof(struct sockaddr_in)) == -1)
		throw "Unable to bind INET";

		// Start listening
	if (::listen(listen_socket_inet, 8) == -1)
		throw "Unable to listen INET";

	doLog("Listening on Port %d", port);

	// Add listen socket to map
	conn_map[listen_socket_inet]	= time(0);

	// Build poll array
	buildPollArray();

	return;
}	// startListening()


//////////////////////////////////////////////////////////////////////////////
//
void run()
{
	for (;;) {
		// Wait
		int	nfd	= ::poll(poll_fds, npoll, 2 * 60 * 1000);	// 2 minutes
		if (nfd == -1) {
			if (errno == EINTR)
				return;
			throw "poll(2) failed";
		}
		else if (nfd == 0) {		// Timeout
			continue;
		}

		t_now	= time(0);

		bool			rebuild	= false;
		int				i;
		struct pollfd	*pf;

		for (i = npoll, pf = poll_fds; (i > 0) && (nfd > 0); i--, pf++) {
			// Nothing to do.
			if (pf->revents == 0)
				continue;

			if (pf->revents & (POLLIN|POLLPRI)) {
				nfd--;

				if (pf->fd == listen_socket_inet) {
					doConnect();
					rebuild	= true;
				}

				else {
					if (readOneMsg(pf->fd) == false) {			// Closed
						doLog("%d|Read Close", pf->fd);
						::close(pf->fd);
						conn_map.erase(pf->fd);
						rebuild	= true;
					}
					else
						doOneMsg(pf->fd);
				}
			}

				// Anything else - close it.
			else {
				doLog("%d|Close", pf->fd);

				::close(pf->fd);
				conn_map.erase(pf->fd);
				rebuild	= true;
			}
		}

		if (rebuild) {
			buildPollArray();
		}
	}

	return;
}	// run()


//////////////////////////////////////////////////////////////////////////////
//
void buildPollArray()
{
ConnIterator		it;
struct pollfd		*pf;

	if (poll_fds == 0) {
		poll_fds	= new struct pollfd[maxpoll];
	}

	for (npoll = 0, it = conn_map.begin(), pf = poll_fds;
					(npoll < maxpoll) && (it != conn_map.end());
					npoll++, it++, pf++) {
		pf->fd		= it->first;
		pf->events	= POLLIN;
	}

	doLog("Rebuild [npoll:%d] [Conn:%d]", npoll, conn_map.size());
	if (npoll == maxpoll)
		doLog("Max Poll Reached! [Max:%d] [Conn:%d]", npoll, conn_map.size());

	return;
}	// buildPollArray()


//////////////////////////////////////////////////////////////////////////////
//
void doConnect()
{
int					socket;
struct sockaddr_in	addr;
socklen_t			len;

		// Accept connection
	len	= sizeof(struct sockaddr_in);
	if ((socket = ::accept(listen_socket_inet, (struct sockaddr *)&addr, &len)) == -1) {
		doLog("Unable to accept [%d]", errno);
		return;
	}

		// Got it, now add to map
	conn_map[socket]	= t_now;

		// Get info
	len	= sizeof(struct sockaddr_in);
	if (::getpeername(socket, (struct sockaddr *)&addr, &len) == 0)
		doLog("%d|Connection %s", socket, inet_ntoa(addr.sin_addr));
	else
		doLog("%d|Connection", socket);

	return;
}	// doConnect()

//////////////////////////////////////////////////////////////////////////////
//
void shutdown()
{
ConnIterator		it;

	for (it = conn_map.begin(); it != conn_map.end(); it++)
		::close(it->first);

	return;
}	// shutdown()


//////////////////////////////////////////////////////////////////////////////
// Read data from a connection
//
bool readSocket(const double f_timeout, int i_conn, char *n_chunk, int i_size, int *i_rsize)
{
char				*ptr;
int					i_bytes;
bool				b_closed;
int					i_cnt;

		// Now loop waiting for data.
	ptr			= n_chunk;
	b_closed	= false;
	*i_rsize	= 0;

	int		i_timeout;

	if (f_timeout > 0)
		i_timeout	= (int)(f_timeout * 1000);
	else
		i_timeout	= -1;

		// Loop waiting for the data.
	while (!b_closed && (i_size > 0)) {
		if (f_timeout > 0) {
			struct pollfd	poll_fds[1];
			poll_fds[0].fd		= i_conn;
			poll_fds[0].events	= POLLIN;

			i_cnt	= ::poll(poll_fds, 1, i_timeout);

			if (i_cnt == 0) {					// Timeout
				doLog("%d|Timeout", i_conn);
				return false;
			}
			else if (i_cnt == 1)				// Ok, data waiting.
				i_bytes = ::recv(i_conn, ptr, i_size, 0);
			else
				i_bytes	= -1;					// Force error processing.
		}
		else
			i_bytes = ::recv(i_conn, ptr, i_size, 0);

		if (i_bytes == 0) {				// Nothing received, connection closed.
			b_closed	= true;
		}

		else if (i_bytes > 0) {			// Got something, update pointers.
			ptr		+= i_bytes;
			i_size	-= i_bytes;

				// Up bytes read.
			*i_rsize	= *i_rsize + i_bytes;
		}

		else {							// Error
			switch (errno) {
#if defined(ENOTCONN)
			case ENOTCONN:
#endif
#if defined(EPROTO)
			case EPROTO:
#endif
#if defined(ECONNRESET)
			case ECONNRESET:
#endif
				b_closed	= true;
				break;

			default:
				doLog("%d|recv(2) failed [%d]", i_conn, errno);
				return false;
			}
		}
	}

		// if closed, set error and return.
	if (b_closed) {
		return false;
	}

	return true;
}	// readSocket()


//////////////////////////////////////////////////////////////////////////////
//
bool readOneMsg(const int socket)
{
int				bytes;
int				i;
unsigned char	c;

		// Get msg type
	if ((readSocket(10, socket, (char *)&msg_type, 1, &bytes) == false) ||
				(bytes != 1)) {
		return false;
	}

		// Get message length
	msg_len	= 0;
	for (;;) {
		if (readSocket(10, socket, (char *)&c, 1, &bytes) == false)
			return false;
		if (bytes != 1)
			return false;
		if (c == '\r')
			continue;
		if (c == '\n')
			break;
		if (!isdigit(c)) {
			doLog("%d|Invalid size", socket);
			return false;
		}
		
		msg_len	= (msg_len * 10) + (c - '0');
		if (msg_len > 1000000) {
			doLog("%d|Invalid size", socket);
			return false;
		}
	}

	// Resize buffer.
	if (msg_len > max_msg_len) {
		int	n	= ((msg_len / 1024) + 1) * 1024;

		delete [] msg;
		msg	= new char[n + 1];
		if (msg == 0)
			throw "Unable to allocate memory";
		
		max_msg_len	= n;
	}

	// Read message.
	if (readSocket(10, socket, msg, msg_len, &bytes) == false)
		return false;
	if (bytes != msg_len)
		return false;
	msg[msg_len]	= 0;

	return true;
}	// readOneMsg()


//////////////////////////////////////////////////////////////////////////////
//
void doOneMsg(const int socket)
{
	switch (msg_type) {
		case MQPIF_CAMERA:
			doCameraMsg(socket);
			break;
		case MQPIF_LIDAR:
			doLidarMsg(socket);
			break;
	}

	return;
}	// doOneMsg()


//////////////////////////////////////////////////////////////////////////////
//
void doCameraMsg(const int socket)
{

	//std::cout << msg << std::endl;

	return;
}	// doCameraMsg()


//////////////////////////////////////////////////////////////////////////////
//
void doLidarMsg(const int socket)
{
	LidarMessage *lidar_msg = (LidarMessage *)msg;

	if ((lidar_msg->theta < 10) || (lidar_msg->theta > 350)) {
		std::cout << "theta: " << lidar_msg->theta
				<< " dist: " <<	lidar_msg->distance
				<< " qual: " << lidar_msg->quality
				<< "\n";


	}

	return;
}	// doLidarMsg()

