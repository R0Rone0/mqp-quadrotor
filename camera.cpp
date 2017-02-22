#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>

#include "util.h"
#include "mqpipc.h"
#include "mqpif.h"

#include <opencv2/opencv.hpp>
#include "apriltag_pose.h"
#include "apriltag_utils/PoseUtil.h"
#include "apriltag_utils/TagDetection.h"

////////////////////////////////////////////////////////////////////////////////
//
static MQPIf mqpif;

// opencv devices
static cv::VideoCapture video_cap_;

// apriltag variables
static apriltag_detector_t* apriltag_detector_;
static apriltag_family_t* apriltag_family_;

////////////////////////////////////////////////////////////////////////////////
// Open the camera.
bool openCamera()
{
	apriltag_family_ = tag36h11_create();
	apriltag_detector_ = apriltag_detector_create();

	apriltag_detector_add_family(apriltag_detector_, apriltag_family_);

	return video_cap_.open(0);
}

AprilTags::TagDetection ConvertDetectionStruct(apriltag_detection_t* det)
{
	AprilTags::TagDetection tag_det;

	tag_det.id = det->id;
	tag_det.hammingDistance = det->hamming;

	tag_det.p[0] = std::make_pair<float,float>(det->p[0][0], det->p[0][1]);
	tag_det.p[1] = std::make_pair<float,float>(det->p[1][0], det->p[1][1]);
	tag_det.p[2] = std::make_pair<float,float>(det->p[2][0], det->p[2][1]);
	tag_det.p[3] = std::make_pair<float,float>(det->p[3][0], det->p[3][1]);

	tag_det.cxy = std::make_pair<float,float>(det->c[0], det->c[1]);
	tag_det.hxy = std::make_pair<float,float>(det->c[0], det->c[1]);

	for(int r = 0; r < det->H->nrows; r++)
		for(int c = 0; c < det->H->ncols; c++)
			tag_det.homography(r,c) = det->H->data[r * det->H->ncols + c];

	return tag_det;
}


////////////////////////////////////////////////////////////////////////////////
//
int getCameraMessage()
{
	cv::Mat frame, gray;

	int nsent = 0;
	if(video_cap_.isOpened())
	{
		video_cap_.read(frame);

		if (frame.empty()) {
			std::cerr << "ERROR! blank frame grabbed\n";
			return 0;
		}

		cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

		// Make an image_u8_t header for the Mat data
		// accessing cv::Mat (C++ data structure) in a C way
		image_u8_t im = { .width = gray.cols,
				.height = gray.rows,
				.stride = gray.cols,
				.buf = gray.data
		};

		// find all tags in the image frame
		zarray_t *detections = apriltag_detector_detect(apriltag_detector_, &im);
		//std::cout << zarray_size(detections) << " tags detected" << std::endl;

		// Draw detection outlines of each tag
		for (int i = 0; i < zarray_size(detections); i++) {
			apriltag_detection_t *det;
			zarray_get(detections, i, &det);

			/*------------------------------------------------------------*/
			AprilTags::TagDetection tag_det = ConvertDetectionStruct(det);

			Eigen::Vector3d translation;
			Eigen::Matrix3d rotation;
			tag_det.getRelativeTranslationRotation(0.064, 595, 595, 330, 340,
					translation, rotation);

			Eigen::Matrix3d F;
			F <<    1, 0,  0,
					0,  -1,  0,
					0,  0,  1;
			Eigen::Matrix3d fixed_rot = F*rotation;
			double yaw, pitch, roll;
			AprilTags::wRo_to_euler(fixed_rot, yaw, pitch, roll);

			CameraMessage msg;

			msg.tag_id = tag_det.id;

			msg.translation.x = translation(0);
			msg.translation.y = translation(1);
			msg.translation.z = translation(2);

			msg.rotation.r = roll;		// degrees
			msg.rotation.p = pitch;		// degrees
			msg.rotation.y = yaw;		// degrees

			//doLog("Sending x=%f", msg.translation.x);

			// Send to server...
			//mqpif.sendOne(MQPIF_CAMERA, &msg, sizeof(msg));
			std::stringstream ss;

			ss << msg.tag_id << ',';
			ss << msg.translation.x << ',';
			ss << msg.translation.y << ',';
			ss << msg.translation.z << ',';
			ss << msg.rotation.r << ',';
			ss << msg.rotation.p << ',';
			ss << msg.rotation.y;

			std::string str = ss.str();

			mqpif.sendOne(MQPIF_CAMERA, str.c_str(), str.length() + 1);

			nsent++;
		}

		// free memory used for detection
		zarray_destroy(detections);
	}

	return nsent;
}

////////////////////////////////////////////////////////////////////////////////
//
int main(int argc, char *argv[]) {
extern char	*optarg;
int			err	= 0;
int			c;
char		host[256];
int			port = 9999;
int			debug = 0;
int			verbose = 0;

	::strcpy(host, "localhost");

	while ((c = getopt(argc, argv, "h:p:dv")) != -1) {
		switch (c) {
			case 'h' :	::strcpy(host, optarg);			break;
			case 'p' :	port		= atoi(optarg);		break;
			case 'd' :	debug++;						break;
			case 'v' :	verbose++;						break;
			default :	err++;							break;
		}
	}

	if (err) {
		std::cerr << "Usage : " << argv[0] << " -h host -p port\n";
		return -1;
	}

	doLog("Starting");

	// open camera.
	if (!openCamera()) {
		doLog("Unable to open default video device");
		return -1;
	}

	try {
		mqpif.connect(host, port);

		int nsent = 0;
		for (int i = 1;; i++) {
			nsent += getCameraMessage();

			if ((i % 100) == 0) {
				doLog("Camera: %d (%d sent)", i, nsent);
			}
		}

		mqpif.disconnect();
	}
	catch (const char *n_err) {
		doLog(n_err);
		doLog("errno=%d", errno);
	}
	catch (...) {
		doLog("Internal Error");
		doLog("errno=%d", errno);
	}

	doLog("Exiting");

	return 0;
}
