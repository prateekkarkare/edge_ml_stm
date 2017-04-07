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
testSize = 30
meansSize = 10
dimension = 5
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
	testData = numpy.int8(range(0,257))
	# Construct a struct to send 
	cmd = createPacket('x', testData, 1)
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

''' Countdown timer display '''		
def countdown(tim):
	for i in xrange(tim,-1,-1):
		time.sleep(1)
		sys.stdout.write('\rWaiting ' + str(i) + ' seconds for MCU to respond')
		sys.stdout.flush()
	print '\n'
	
'''Create the means data and the classes array to send to MCU'''
def createSendData():
	ser = openSerialPort(port)
	print "Sending " + str(dimension) + " dimensional kNN data to MCU"
	
	############# Create data ########################
	#Xmeans = numpy.uint8([0,20,40,60,80,100])
	means = numpy.random.randint(0, 100, (dimension, meansSize), dtype='uint8')
	#Ymeans = numpy.uint8([1,21,41,61,81,101])
	#Zmeans = numpy.uint8([2,22,42,62,82,102])
	#means = numpy.vstack((Xmeans, Ymeans, Zmeans))
	classes = numpy.uint8(numpy.random.choice(meansSize, meansSize, replace=False))
	
	############## Create and send packets ###############
	classpkt = createPacket('c', classes, 1)
	meanspkt = createPacket('a', means.flatten(), 1)
	ser.write(meanspkt)		#Write means array to CPU
	ser.write(classpkt)		#Send classes array
	ser.close()
	return [means, classes]

'''This function creates a random test data and checks MCu classification
		and compares it to python classification'''
def testkNNMCU(means, classes):
	ser = openSerialPort(port)
	compute = numpy.uint8([0])			#This packet asks the MCU to start its compute
	test = numpy.random.randint(0, 100, (dimension, testSize), dtype='uint8')
	
	computepkt = createPacket('k', compute, 0)
	testpkt = createPacket('b', test.flatten(), 1)
	
	ser.write(testpkt)		#Send the array of testpoints
	ser.write(computepkt)	#This is a signal which asks the MCU to compute the kNN classification
	
	classification = []
	classification = kNNClassify(test, means, classes)
	countdown(timeoutSeconds)
	rxData = []
#	while True:
	while ser.inWaiting() > 0:
		r = ser.read()
		#print r
		if (r != ''):
			out = struct.unpack('B', r)
			rxData.append(out[0])
#		if (time.time() > timeout):
#			break
		#print rxData
	print "MCU Classification: \n" + str(rxData)
	print "Python classification: \n" + str(classification)
	if ((classification == rxData)):
		print "kNN test passed"
	ser.close()

''' This function perfoms the kNN classification given a means array of classes
	and an array of test points and the array of classes'''
def kNNClassify(testArray, meansArray, classes):
	predictedClasses = []
	for i in range(testSize):
		point = []
		for d in range(dimension):
			point.append(testArray[d][i])
		predictedClasses.append(predictClass(point, meansArray, classes))
	return predictedClasses

''' This function takes in a point and a means array of classes 
	and returns the predicted class for that point '''
def predictClass(testPoint, meansArray, classes):
	mindistance = sys.maxint
	minmean = []
	predictedClass = []
	for i in range(len(classes)):
		meansPoint = []
		for d in range(dimension):
			meansPoint.append(meansArray[d][i])
		distance = nDimDistance(testPoint, meansPoint)
		if (distance < mindistance):
			mindistance = distance
			for d in range(dimension):
				minmean.append(meansArray[d][i])
			predictedClass = classes[i]
	return predictedClass	

''' This function calculates the distance between two n dimensional points'''
def nDimDistance(testPoint, meansArray):
	squareDiff = 0
	for d in range(dimension):
		squareDiff = squareDiff + (int(testPoint[d]) - int(meansArray[d]))**2
	distance = numpy.int32(math.sqrt(squareDiff))
	return distance
	
def createPacket(header, payload, verbose=False):
	packet = struct.pack('Hc%sB' % len(payload), len(payload), header, *payload)
	if (verbose):
		print "Created packet with " + str(len(payload)) + " bytes of data" 
		print struct.unpack('Hc%sB' % len(payload), packet)
		return packet
	else:
		return packet

def main():
#	checkUSBComm(port)
	[means, classes] = createSendData()
	i = 0
	for i in range(3):
		testkNNMCU(means, classes)
		
if __name__ == "__main__":
    main()