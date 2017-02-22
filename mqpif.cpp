#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>

#include "mqpif.h"
#include "mqpipc.h"


//////////////////////////////////////////////////////////////////////////////
MQPIf::MQPIf() : connected(false), socket(-1), ip(0)
{
	::memset(result, 0, sizeof(result));
}


//////////////////////////////////////////////////////////////////////////////
MQPIf::~MQPIf()
{
	if (connected) {
		disconnect();
	}
}


//////////////////////////////////////////////////////////////////////////////
bool MQPIf::connect(const char * const host, const int port)
{
		// Look up the host info
	if (ip == 0) {
		struct hostent	*hp;
		if ((hp = gethostbyname(host)) == 0) {
			::sprintf(result, "Unknown host '%s'", host);
			throw result;
		}

		ip = ntohl(((struct in_addr *)(hp->h_addr_list[0]))->s_addr);
	}

		// Build socket address
	struct sockaddr_in	addr;
	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= htonl(ip);
	addr.sin_port			= htons(port);

		// Create a INET type socket.
	if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		::strcpy(result, "Unable to create socket");
		throw result;
	}

		// Connect to the new socket.
	if (::connect(socket, (struct sockaddr *)&addr,
										sizeof(struct sockaddr)) == -1) {
		disconnect();
		::sprintf(result, "Unable to connect H:%s P:%d", host, port);
		throw result;
	}

	// Set flag
	connected	= true;

	return true;
}	// connect()


//////////////////////////////////////////////////////////////////////////////
void MQPIf::disconnect()
{
	if (connected) {
		::close(socket);
		connected	= false;
	}

	return;
}	// disconnect()


//////////////////////////////////////////////////////////////////////////////
bool MQPIf::sendOne(MQPIF_MsgType type, const void *msg, const int msg_len)
{
	if (!connected) {
		::strcpy(result, "Not Connected");
		throw result;
	}

	bool	ok	= true;

	// Catch SIGPIPE
	struct sigaction	act, oact;
	act.sa_handler	= SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags	= 0;
	sigaction(SIGPIPE, &act, &oact);

	try {
		// Send msg type
		if (::send(socket, &type, 1, 0) != 1)
			throw "Unable to send msg type";

		// Send msg length
		char	length[16];
		::sprintf(length, "%d\n", msg_len);
		int		len	= ::strlen(length);
		if (::send(socket, length, len, 0) != len)
			throw "Unable to send msg length";

			// Send msg
		if (::send(socket, msg, msg_len, 0) != msg_len)
			throw "Unable to send msg";
	}
	catch (const char *err) {
		::sprintf(result, "%s [%d]", err, errno);
		ok	= false;

		disconnect();
	}

	// Restore SIGPIPE
	sigaction(SIGPIPE, &oact, 0);

	// Fail!
	if (!ok) {
		throw result;
	}

	return true;
}	// sendOne()
