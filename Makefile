##################################################################
BIN_DIR = bin
OBJ_DIR = obj

APRILTAG_DIR = ../apriltag-pose/apriltag-pose

RPLIDAR_DIR = ../rplidar_sdk/sdk

CFLAGS +=	-g3
LDFLAGS +=	-g3

CFLAGS +=	-I$(APRILTAG_DIR)/src/pose \
				-I$(APRILTAG_DIR)/src/eigen \
				-I$(APRILTAG_DIR)/src/apriltag \
				-I$(APRILTAG_DIR)/src
			
CFLAGS += 	-I$(RPLIDAR_DIR)/sdk/include

OBJS1 =	$(OBJ_DIR)/mqp.o \
		$(OBJ_DIR)/util.o

OBJS2 =	$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/util.o \
		$(OBJ_DIR)/mqpif.o

OBJS3 =	$(OBJ_DIR)/lidar.o \
		$(OBJ_DIR)/util.o \
		$(OBJ_DIR)/mqpif.o

OPENCV_LIBS = 	-lopencv_core \
					-lopencv_calib3d \
					-lopencv_highgui \
					-lopencv_imgproc
				
APRILTAG_LIBS =	-L$(APRILTAG_DIR)/build/lib \
					-lapriltag_pose \
					-lapriltags \
					-lapriltag_utils

RPLIDAR_LIBS =	-L$(RPLIDAR_DIR)/output/Linux/Release \
					-lrplidar_sdk

LDLIBS1 =	-lpthread

LDLIBS2 =	$(APRILTAG_LIBS) \
			$(OPENCV_LIBS) \
			-lpthread

LDLIBS3 =	$(RPLIDAR_LIBS) \
			-lpthread

##################################################################
all : $(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/mqp $(BIN_DIR)/camera $(BIN_DIR)/lidar

clean :
	$(RM) -rf $(OBJ_DIR) $(BIN_DIR)
	$(RM) -f core

.PHONY obj ::
	mkdir -p obj
	
.PHONY bin ::
	mkdir -p bin

$(BIN_DIR)/mqp : $(OBJS1)
	$(LINK.cc) $(LDFLAGS) -o $@ $(OBJS1) $(LDLIBS1)
	
$(BIN_DIR)/camera : $(OBJS2)
	$(LINK.cc) $(LDFLAGS) -o $@ $(OBJS2) $(LDLIBS2)
	
$(BIN_DIR)/lidar : $(OBJS3)
	$(LINK.cc) $(LDFLAGS) -o $@ $(OBJS3) $(LDLIBS3)

##################################################################
$(OBJ_DIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< 
