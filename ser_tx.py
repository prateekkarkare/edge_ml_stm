import serial
import time
import numpy
import struct
import math

#Connection settings
port = "COM6"
baud = 115200
timeoutSeconds = 5

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

def createPacket(header, payload, verbose=False):
	packet = struct.pack('Hc%sb' % len(payload), len(payload), header, *payload)
	if (verbose):
		print "Created packet with " + str(len(payload)) + " bytes of data" 
		print struct.unpack('Hc%sb' % len(payload), packet)
		return packet
	else:
		return packet
		
		
timeout = time.time() + 5   # 5 minutes from now
def checkData(port):
	ser = openSerialPort(port)
	# Create test array of int8 by providing a range
	test = numpy.random.randint(0, 100, (3, 10), dtype='int8')
	Xmeans = numpy.int8([0,20,40,60,80,100])
	Ymeans = numpy.int8([1,21,41,61,81,101])
	Zmeans = numpy.int8([2,22,42,62,82,102])
	means = numpy.vstack((Xmeans, Ymeans, Zmeans))
	classes = numpy.int8(numpy.random.choice(len(Xmeans), len(Xmeans), replace=False))
	classpkt = createPacket('c', classes, 1)
	meanspkt = createPacket('a', means.flatten(), 1)
	testpkt = createPacket('b', test.flatten(), 1)
	classification = []
	classify(test, means, classes)
	#for point in test:
	#	 classification.append(computeDistances(point, means)[0])
	print "Writing data to MCU..."
	ser.write(meanspkt)
	time.sleep(1)
	ser.write(classpkt)
	time.sleep(1)
	ser.write(testpkt)
	time.sleep(1)
	#print ser.inWaiting()
	rxData = []
	while True:
		r = ser.read()
		#print r
		if (r != ''):
			out = struct.unpack('b', r)
			#print out
			rxData.append(out[0])
		if (time.time() > timeout):
			break
	ser.close()
	print "MCU Classification: \n" + str(rxData)
	print "Python classification: \n" + str(classification)
	if ((classification == rxData)):
		print "Equal"

def classify(testArray, meansArray, classes):
	for i in range(len(testArray[0])):
		point = [testArray[0][i], testArray[1][i], testArray[2][i]]
		print computeDistances(point, meansArray, classes), point

def computeDistances(testPoint, meansArray, classes):
	mindistance = 174
	minmean = -1
	predictedClass = -1
	for i in range(len(meansArray[0])):
		distance = numpy.int32(math.sqrt((testPoint[0] - meansArray[0][i])**2 + (testPoint[1] - meansArray[1][i])**2 + (testPoint[2] - meansArray[2][i])**2))
		if (distance < mindistance):
			mindistance = distance
			minmean = [meansArray[0][i], meansArray[1][i], meansArray[2][i]]
			predictedClass = classes[i]
	return [minmean, mindistance, predictedClass]	
		
def main():
#	checkUSBComm(port)
	checkData(port)	
	
	
		
if __name__ == "__main__":
    main()