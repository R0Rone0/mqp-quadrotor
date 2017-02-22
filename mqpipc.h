#ifndef mqpipc_h_included
#define mqpipc_h_included

#include <string.h>			// For memset()


//////////////////////////////////////////////////////////////////////////////
// Camera Message
struct Translation {
	double			x;
	double			y;
	double			z;
};

struct Rotation {
	double			r;
	double			p;
	double			y;
};

struct CameraMessage {
	int				tag_id;
	Translation		translation;
	Rotation		rotation;
};


//////////////////////////////////////////////////////////////////////////////
// LiDAR Message
struct LidarMessage {
	double			theta;
	double			distance;
	double			quality;
};

#endif	// mqpipc_h_included
