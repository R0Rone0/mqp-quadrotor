#Socket Server for On-Board Sensors

##Description
This server was developed to be put on board a 3DR Solo Quadrotor carrying a Raspberry Pi, camera, and LiDAR sensor. This server provides the means of communicating between the Raspberry Pi and the on-board sensors.

##Setup Environment
1. Make /dev directory for environment
```
mkdir dev
cd dev
```
2. Clone and build Apriltag library into /dev (courtesy of Ruixiang Du)
```
git clone https://github.com/rxdu/apriltag-pose.git
```
3. Download RPLiDAR A2 SDK from [Slamtec](http://www.slamtec.com/en/lidar) website
4. Unzip SDK into /dev
```
unzip rplidar\__sdk_v1.5.7.zip
```
5. Build SDK in /dev
```
cd rplidar_sdk/sdk
make all
```

##How to Compile
1. Clone repository into local environment
2. Navigate to repository
3. Execute: 
```
make all
```

##How to Execute
1. Compile code
2. Run python server
```python
python mqp.py
```
3. Run either camera or lidar in bin folder
4. Data should now print into terminal executing server
