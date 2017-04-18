import serial
import time
import numpy
import struct
import math
import sys

#Connection settings
port = "COM6"
baud = 115200
timeoutms = 300
delayAfterWrite = 100	#ms


#Time out for reading from serial port
timeout = time.time() + 5   # 5 secs from now

target = open('report.txt', 'a')

""" Function to try open a serial port and return a serial object """
def openSerialPort(port):
	try:
		#print "Serial port %s opened" % (port)
		return serial.Serial(port, baud, timeout=timeoutms/1000)
	except:
		print "Cannot open serial port, check if another application is using the port"
		return None
		
ser = openSerialPort(port)

while True:
	data = ser.read(3)
	print data
	if (data != ''):
		print struct.unpack('3H', data)