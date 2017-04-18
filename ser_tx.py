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

#Global variables
testSize = 20
meansSize = 6
dimension = 6
dataRange = 255

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

''' Function to test USB communication '''
''' Packet structure --> (Size of data (int16), header (char), data array (8 bit integers)) ''' 
def checkUSBComm(port):
	ser = openSerialPort(port)
	# Create test array by providing a range
	testData = numpy.int8(range(0,257))
	# Construct a struct to send 
	cmd = createPacket('x', testData, 1)
	sentData = delaySerWrite(cmd, ser, delayAfterWrite)
	print "Sent " + str(sentData-3) + " bytes of data for USB communication check" 
	#print struct.unpack('Hc%sb' % len(testData), cmd)
	rxData = []
	# Wait for device to respond
	print "Waiting " + str(timeoutms) + " milli-seconds for device to respond"
	time.sleep(timeoutms/1000)
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
		time.sleep(0.001)
		#sys.stdout.write('Waiting %i milli-seconds for MCU to respond\r' % tim)
		sys.stdout.flush()
	print '\n'
	
'''Create the means data and the classes array to send to MCU'''
def createSendData():
	ser = openSerialPort(port)
	print "Sending " + str(dimension) + " dimensional kNN data to MCU"
	
	############# Create data ########################
	#Xmeans = numpy.uint8([0,20,40,60,80,100])
	means = numpy.random.randint(-128, 127, (dimension, meansSize), dtype='int8')
	#Ymeans = numpy.uint8([1,21,41,61,81,101])
	#Zmeans = numpy.uint8([2,22,42,62,82,102])
	#means = numpy.vstack((Xmeans, Ymeans, Zmeans))
	classes = numpy.uint8(numpy.random.choice(meansSize, meansSize, replace=False))
	
	############## Create and send packets ###############
	numOfDimpkt = createPacket('d', numpy.uint8([dimension]), 1)
	classpkt = createPacket('c', classes, 1)
	meanspkt = createPacket('a', means.flatten(), 1)
	delaySerWrite(numOfDimpkt, ser, delayAfterWrite)	#This needs to be sent first ALWYAS
	delaySerWrite(meanspkt, ser, delayAfterWrite)		#Write means array to CPU
	delaySerWrite(classpkt, ser, delayAfterWrite)		#Send classes array
	ser.close()
	return [means, classes]

def delaySerWrite(data, serialHandle, delayms):
	serialHandle.write(data)
	time.sleep(delayms/1000.0)
	
'''This function creates a random test data and checks MCu classification
		and compares it to python classification'''
def testkNNMCU(means, classes):
	ser = openSerialPort(port)
	compute = numpy.uint8([0])			#This packet asks the MCU to start its compute
	test = numpy.random.randint(-128, 127, (dimension, testSize), dtype='int8')
	
	computepkt = createPacket('k', compute, 0)
	testpkt = createPacket('b', test.flatten(), 0)
	
	delaySerWrite(testpkt, ser, delayAfterWrite)		#Send the array of testpoints
	delaySerWrite(computepkt, ser, delayAfterWrite)	#This is a signal which asks the MCU to compute the kNN classification
	
	classification = []
	classification = kNNClassify(test, means, classes)
	countdown(timeoutms)
	rxData = []
#	while True:
	while ser.inWaiting() > 0:
		r = ser.read()
		#print r
		if (r != ''):
			out = struct.unpack('b', r)
			rxData.append(out[0])
#		if (time.time() > timeout):
#			break
		#print rxData
	print "MCU Classification: \n" + str(rxData)
	target.write("MCU Classification: \n" + str(rxData))
	print "Python classification: \n" + str(classification)
	target.write("Python classification: \n" + str(classification))
	if ((classification == rxData)):
		print "kNN test passed\n"
		target.write("kNN test passed\n")
		return [0, rxData, classification]
	else:
		print "Fail!\n"
		target.write("Fail!\n")
		return [1, rxData, classification]
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
	packet = struct.pack('Hc%sb' % len(payload), len(payload), header, *payload)
	if (verbose):
		print "Created packet with " + str(len(payload)) + " bytes of data" 
		print struct.unpack('Hc%sb' % len(payload), packet)
		return packet
	else:
		return packet

def main():
#	checkUSBComm(port)
	[means, classes] = createSendData()
	i = 0
	testCount = 10
	failCount = 0
	target.write("Running kNN random batch test %i times\n" % testCount)
	print "Running kNN random batch test %i times\n" % testCount
	for i in range(testCount):
		print "Test vector %i" % i
		if (testkNNMCU(means, classes)[0]):
			failCount = failCount + 1
	print "\nkNN random batch test passed " + str(testCount - failCount) + "/" +  str(testCount) + " times"
	target.write("\nkNN random batch test passed " + str(testCount - failCount) + "/" +  str(testCount) + " times")  
	target.close()
	
if __name__ == "__main__":
    main()
	