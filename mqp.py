import socket
import SocketServer

def doCameraMsg(self):
    while True:
	datastream = self.request.recv(56)
	print datastream.split(',')
        break
        
def doLidarMsg(self):
    while True:
	"""
	theta = self.request.recv(8)
	distance = self.request.recv(8)
	quality = self.request.recv(8)
	print "datastream = ", theta, ",", distance, ",", quality
	print "theta = ", theta
	print "distance = ", distance
	print "quality = ", quality
	"""
	datastream = self.request.recv(1024)
	print datastream.split(',')
	break

class MyTCPHandler(SocketServer.BaseRequestHandler):
    """
    The request handler class for our server.

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """

    def handle(self):
        while True:
            msg_type = self.request.recv(1).strip()
            if not msg_type:
                break

            length = 0
            while True:
                c = self.request.recv(1).strip()
                

                if ((c >= '0') and (c <= '9')):
                    length = (length * 10) + int(c)
                else:
                    break

            #print "msg_type = ", msg_type
            #print "length = ", length # = 56 for camera, 24 for lidar
            if msg_type == 'C':
                doCameraMsg(self)
		continue
            elif msg_type == 'L':
                doLidarMsg(self)
		continue
            #print "c = ", c
            #print length
            #data = self.request.recv(length)
            #print "full msg data: ", data

        print "done"

if __name__ == "__main__":
    HOST, PORT = "localhost", 9999

    # Create the server, binding to localhost on port 9999
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    server.serve_forever()
