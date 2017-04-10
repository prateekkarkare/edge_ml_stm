import serial
import time
import numpy
import struct
import math
import sys

#Connection settings
port = "COM6"
baud = 115200
timeoutSeconds = 5

#Global variables
testSize = 1
meansSize = 10
dimension = 10
dataRange = 255

#Time out for reading from serial port
timeout = time.time() + 5   # 5 secs from now

""" Function to try open a serial port and return a serial object """
def openSerialPort(port):
	try:
		print "Serial port %s opened" % (port)
		return serial.Serial(port, baud, timeout=timeoutSeconds)
	except:
		print "Cannot open serial port, check if another application is using the port"
		return None

''' Function to test USB communication '''
''' Packet structure --> (Size of data (int16), header (char), data array (8 bit integers)) ''' 
def checkUSBComm(port):
	ser = openSerialPort(port)
	# Create test array by providing a range
	testData = numpy.uint8([dimension])
	# Construct a struct to send 
	cmd = createPacket('d', testData, 1)
	sentData = ser.write(cmd)
	print "Sent " + str(sentData-3) + " bytes of data for USB communication check" 
	#print struct.unpack('Hc%sb' % len(testData), cmd)
	rxData = []
	# Wait for device to respond
	print "Waiting " + str(timeoutSeconds) + " seconds for device to respond"
	time.sleep(timeoutSeconds)
	print "Received " + str(ser.inWaiting()) + " bytes from MCU"
	while ser.inWaiting() > 0:
		r = ser.read()
		#print r
		if (r != ''):
			out = struct.unpack('b', r)
			#print out
			rxData.append(out[0])
	ser.close()
	print rxData
	if ((rxData == testData).all()):
		print "Received " + str(len(rxData)) + " bytes of data"
		print "USB communication test passed"
	else:
		print "USB test failed"
		print rxData

		
def createPacket(header, payload, verbose=False):
	packet = struct.pack('Hc%sB' % len(payload), len(payload), header, *payload)
	if (verbose):
		print "Created packet with " + str(len(payload)) + " bytes of data" 
		print struct.unpack('Hc%sB' % len(payload), packet)
		return packet
	else:
		return packet

def main():
	checkUSBComm(port)
		
if __name__ == "__main__":
    main()