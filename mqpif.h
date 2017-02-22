#ifndef mqpif_h_included
#define mqpif_h_included


//////////////////////////////////////////////////////////////////////////////
enum MQPIF_MsgType {
	MQPIF_CAMERA			= 'C',
	MQPIF_LIDAR				= 'L',
};


//////////////////////////////////////////////////////////////////////////////
class MQPIf {
private:
	bool		connected;

	int			socket;
	int			ip;

	char		result[256];

public:
	MQPIf();
	~MQPIf();

	bool connect(const char * const host, const int port);
	void disconnect();

	bool sendOne(MQPIF_MsgType type, const void *msg, const int msg_len);

	const char * getResult()			{ return result; }
};


#endif	// mqpif_h_included
