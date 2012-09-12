import XBee_ext
import cherrypy
import threading
import json
import urllib
import urllib2
import time
import sys
import string
import base64
from threading import Thread

# Global Variables
USER = "khol"
PASSWORD = "action12"
#HOST = "http://142.103.25.37/"
HOST ="http://demo.sensetecnic.com/"
serialPort = "/dev/ttyUSB1"
baud = 115200 
timeout = 5000 #10 secs = 10000ms
BroadcastMSB =0x00000000
BroadcastLSB=0x0000FFFF

XBee=XBee_ext.XBeeSystem(baud, serialPort)
Nodes = XBee_ext.Nodes
Queue = XBee_ext.Nodes

XBeeLock = threading.RLock()

exitCode=0;


def threadfunc(b):
    global XBeeLock
    global XBee
    global exitCode
    while exitCode==0:
		#XBeeLock.acquire()
        #XBee.read()
		Nodes = XBee.getSensors()
		time.sleep(1)
		#XBeeLock.release()
		time.sleep(1)
		print '%d'%Nodes.size()
		for i in range (0,Nodes.size()):
	#print '%d'%Nodes.nodes.size();
			print "%s"%Nodes.nodes[i].data.Name
	#print "%X"%Nodes.nodes[0].data.SN_High
	#print "%X"%Nodes.nodes[0].data.SN_Low
	#print "%d"%Nodes.nodes[0].data.analog_data[0];
	#print "%d"%Nodes.nodes[0].data.maximum[0];
	#print "%d"%Nodes.nodes[0].data.minimum[0];
	#print "%d"%Nodes.nodes[0].data.rxData.size();
	#print "%d"%Nodes.nodes[0].data.rxDataSize();
		if Nodes.nodes[0].data.rxDataSize()>0:
			print "%X"%Nodes.nodes[0].data.rxData[0]
        #print XBee.printSensors()
        #time.sleep(1)


       
#root = MainPage()
#root.config = ConfigPage()
#root.view = SensorViewPage()

#cherrypy.config.update({'server.socket_port': 8000,})

# Start the main function

'''def main():

	try:
		XBee.setDiscoveryTimeout(timeout)
		XBee.initializeNetwork() # initialize the network

		b=0
		reader = Thread(target=threadfunc, args=(b,))
		reader.start()
		reader.stop=threading.Event()
#c=0
#client = Thread(target=XBeeClientFunction,args=(c,))
#client.start()
#cherrypy.quickstart(root,config={'/': {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}})
		print "Waiting to terminate"
#exitCode =1

		while 1:
			if Nodes.nodes[0].data.rxDataSize()>0:
				print "%X"%Nodes.nodes[0].data.rxData[0]
			time.sleep(10)

	except:
		exitCode=1;
	#for t in threads:
	#t.stop()
	#t.join()
	#del threads[:]
		reader.join()
		print 'child thread exiting...'+'/n/n'


if __name__ == '__main__':'''


class HelloWorld:
    """ Sample request handler class. """

    def index(self):
        # CherryPy will call this method for the root URI ("/") and send
        # its return value to the client. Because this is tutorial
        # lesson number 01, we'll just send something really simple.
        # How about...
        return "Hello world!"

    # Expose the index method through the web. CherryPy will never
    # publish methods that don't have the exposed attribute set to True.
    index.exposed = True


import os.path
tutconf = os.path.join(os.path.dirname(__file__), 'tutorial.conf')

def threadfunc1(b):
	cherrypy.quickstart(HelloWorld())

XBee.setDiscoveryTimeout(timeout)
XBee.initializeNetwork() # initialize the network
XBee.stop()
time.sleep(2

exitCode=0
b=0
reader = Thread(target=threadfunc, args=(b,))
	#reader = Thread(target=threadfunc, args=(b,))
reader.start()

b=0
cherry = Thread(target=threadfunc1, args=(b,))
	#reader = Thread(target=threadfunc, args=(b,))
#cherry.start()
	#reader.stop=threading.Event()
try:
	cherrypy.quickstart(HelloWorld())
	while 1:
	
		time.sleep(10)

except:
	global exitCode
	exitCode=1;
	#for t in threads:
	#t.stop()
	#t.join()
	#del threads[:]
	reader.join()
	#cherry.join()
	XBee.stop()
	print 'child thread exiting...'+'/n/n'









#cherrypy.server.start()
#cherrypy.quickstart(XBeePage(), 'tutorial.conf')
#cherrypy.quickstart(XBeePage())
