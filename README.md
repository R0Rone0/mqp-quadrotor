#Socket Server for On-Board Sensors

##Description
This server was developed to be put on board a 3DR Solo Quadrotor carrying a Raspberry Pi, camera, and LiDAR sensor. This server provides the means of communicating between the Raspberry Pi and the on-board sensors.

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
