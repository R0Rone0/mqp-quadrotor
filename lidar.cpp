#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include "util.h"
#include "mqpipc.h"
#include "mqpif.h"
#include "rplidar.h"

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

using namespace rp::standalone::rplidar;

////////////////////////////////////////////////////////////////////////////////
RPlidarDriver * drv = 0;

////////////////////////////////////////////////////////////////////////////////
//
static MQPIf mqpif;

////////////////////////////////////////////////////////////////////////////////
bool checkRPLIDARHealth(RPlidarDriver * drv)
{
    u_result     op_result;
    rplidar_response_device_health_t healthinfo;

    op_result = drv->getHealth(healthinfo);
    if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        std::cout << "RPLidar health status : " << healthinfo.status << std::endl;
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
            std::cerr << "Error, rplidar internal error detected. Please reboot the device to retry.\n";
            // enable the following code if you want rplidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        std::cerr << "Error, cannot retrieve the lidar health code: " << op_result << "\n";
        return false;
    }
}

bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}


bool initLidar(const char * const path, int baudrate)
{
	// create the driver instance
	drv = RPlidarDriver::CreateDriver(RPlidarDriver::DRIVER_TYPE_SERIALPORT);

	if (!drv) {
		std::cerr << "insufficent memory, exit\n";
		return false;
	}

	// make connection...
	if (IS_FAIL(drv->connect(path, baudrate))) {
		std::cerr << "Error, cannot bind to the specified serial port " <<
			path << "\n";
		return false;
	}

	rplidar_response_device_info_t devinfo;
	// retrieving the device info
	////////////////////////////////////////
	if (IS_FAIL(drv->getDeviceInfo(devinfo))) {
		std::cerr << "Error, cannot get device info.\n";
		return false;
	}

	// check health...
	if (!checkRPLIDARHealth(drv)) {
		return false;
	}

	return true;
}


void readLidar()
{
	size_t   count = 360*2;
	rplidar_response_measurement_node_t nodes[count];

	u_result op_result = drv->grabScanData(nodes, count);

	if (IS_OK(op_result)) {
		drv->ascendScanData(nodes, count);

		LidarMessage msg;
		for (int pos = 0; pos < (int)count ; ++pos) {
			msg.theta	= (nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/64.0f;
			msg.distance	= nodes[pos].distance_q2/4.0f;
			msg.quality	= nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;

			// Send to server...
			//mqpif.sendOne(MQPIF_LIDAR, &msg, sizeof(msg));
/*
			std::stringstream ss;

			ss << msg.theta << ",";
			ss << msg.distance << ",";
			ss << msg.quality << ",";

			std::string str = ss.str();

			mqpif.sendOne(MQPIF_LIDAR, str.c_str(), str.length() + 1);

			nsent++*/
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
int main(int argc, char *argv[]) {
char		com_path[128];
_u32		com_baudrate = 115200;
extern char	*optarg;
int			err	= 0;
int			c;
char		host[256];
int			port = 9999;
int			debug = 0;
int			verbose = 0;

	::strcpy(host, "localhost");
	::strcpy(com_path, "/dev/ttyUSB0");

	while ((c = getopt(argc, argv, "h:p:c:b:dv")) != -1) {
		switch (c) {
			case 'h' :	::strcpy(host, optarg);			break;
			case 'p' :	port		= atoi(optarg);		break;
			case 'c' :	::strcpy(com_path, optarg);		break;
			case 'b' :	com_baudrate	= atoi(optarg);	break;
			case 'd' :	debug++;						break;
			case 'v' :	verbose++;						break;
			default :	err++;							break;
		}
	}

	if (err) {
		std::cerr << "Usage : " << argv[0] << " -h host -p port -c com_path -b com_baud\n";
		return -1;
	}

	doLog("Starting");

	if (!initLidar(com_path, com_baudrate)) {
		return -2;
	}

	signal(SIGINT, ctrlc);

	try {
		mqpif.connect(host, port);

		drv->startMotor();

		// start scan...
		drv->startScan();

		// fetch result and print it out...
		while (!ctrl_c_pressed) {
			readLidar();
		}
	}
	catch (const char *n_err) {
		doLog(n_err);
		doLog("errno=%d", errno);
	}
	catch (...) {
		doLog("Internal Error");
		doLog("errno=%d", errno);
	}

	mqpif.disconnect();

	drv->stop();
	drv->stopMotor();

	// done!
	RPlidarDriver::DisposeDriver(drv);
	return 0;
}
