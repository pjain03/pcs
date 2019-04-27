# Benchmarks

import os
import sys
from tabulate import tabulate
import time
import threading
import subprocess
from multiprocessing.pool import ThreadPool
#Performance (e.g., latency of small transfers, throughput of large transfers); 


port = sys.argv[1]
proxy = "comp112-02.cs.tufts.edu" # CHANGE THIS AS NECESSARY
url0 = "http://portquiz.net:8080/" # Small web transfer - 50 bytes
url1 = "https://www.hq.nasa.gov/alsj/a17/A17_FlightPlan.pdf" # Large web transfer - 20 MB
url2 = "www.cs.tufts.edu/comp/160/"

noPTimeShort = 0 # no Proxy Time short transfer
pTimeShort = 0
noPTimeLong = 0
pTimeLong = 0
runs = 20
shortTransferIncrease = 0
longTransferIncrease = 0


def findLatency(curlString):
	time = 0
	for x in range(runs):
		# Print the total time into temp file noProxy
		os.system(curlString)
		# Read the time printed into the file
		with open("time") as file: 
		   time += float(file.read())

	# Average out the times
	return time / runs

def convertThroughputToString(throughput):

	if (throughput > 1000 and throughput < 1000000):
		t = int (throughput/1000) # Convert into a integer
		s = str(t) + "kbps"
	elif (throughput > 1000000):
		t = int(throughput/1000000)
		s = str(t) + "mbps"
	else:
		t = int (throughput)
		s = str(t) + "bps"
	return s

# BENCHMARK 1: Latency of small transfers via proxy and no proxy

def runSmallLatency():
	global noPTimeShort
	global pTimeShort
	global shortTransferIncrease

	# Find average latency of small web HTTP transfer without a proxy
	noPTimeShort = findLatency("curl -s -w '%{time_total}' -o /dev/null " + url0 + " > time")


	# Find average latency of small web HTTP transfer with a proxy
	pTimeShort = findLatency("curl -s -w '%{time_total}' -o /dev/null -x " + proxy + ":" + port + " " + url0 + " > time")


	# Calculate proxy % performance increase
	shortTransferIncrease = ((noPTimeShort - pTimeShort) / noPTimeShort) * 100


	print("--- Benchmark 1: Latency of Small Web HTTP Transfer (50 bytes) Completed ---\n")



# Benchmark 2: Latency of small transfers via proxy and no proxy


def runLargeLatency():
	global noPTimeLong
	global pTimeLong
	global longTransferIncrease

	noPTimeLong = findLatency("curl -s -w '%{time_total}' -o /dev/null " + url1 + " > time")
	pTimeLong = findLatency("curl -s -w '%{time_total}' -o /dev/null -x " + proxy + ":" + port + " " + url1 + " > time")

	# Calculate proxy % performance increase
	longTransferIncrease = ((noPTimeLong - pTimeLong) / noPTimeLong) * 100

	print("--- Benchmark 2: Latency of Large Web HTTPS Transfer (20M bytes) Completed ---\n")

def calcThroughputAndPrintResults():
	# Throughput = size (bits) / time (s)
	throughput1 = 50 * 8 / noPTimeShort
	throughput2 = 50 * 8 / pTimeShort
	throughput3 = 20000000 * 8 / noPTimeLong
	throughput4 = 20000000 * 8 / pTimeLong

	shortTPIncrease = ((noPTimeShort - pTimeShort) / noPTimeShort) * 100
	longTPIncrease = ((noPTimeLong - pTimeLong) / noPTimeLong)


	# Remove the temp file
	os.system("rm time")

	print(tabulate([['No Proxy', str(noPTimeShort) + "s", str(noPTimeLong) + "s", convertThroughputToString(throughput1), convertThroughputToString(throughput3)], 
					['Proxy', str(pTimeShort) + "s", str(pTimeLong) + "s", convertThroughputToString(throughput2), convertThroughputToString(throughput4)], 
					['Proxy % Increase', str(shortTransferIncrease)+"%", str(longTransferIncrease)+"%", str(shortTPIncrease)+"%", str(longTPIncrease)+"%"]], 

					headers=[' ', 'Avg Latency of Small Cached Transfer (50B)', 'Avg Latency of Large Uncached Transfer (20MB)', 'Throughput of Small Transfer', 'Throughput of Large Transfer']))


def restart_line():
    sys.stdout.write('\r')
    sys.stdout.flush()


# Running latency benchmarks
print("Running benchmark tests for latency...")
print("Note : All results will be printed at the end\n")
#runSmallLatency()
#runLargeLatency()
#calcThroughputAndPrintResults()


#Scalability (e.g., how many requests per second, number of concurrent clients, etc)


concurrentRuns = 10

def runConcurrent(num):
	os.system("curl -s " + url2 + " > real")
	for x in range(num):
	# Running bash script that runs curl command in parallel with &
		t = os.system("curl -s -o /dev/null -x " + proxy + ":" + port + " " + url2 + " &")
		if (t != 0):
			return False
		#os.system("curl -x " + proxy + ":" + port + " " + url2 + " > temp" + str(x) + " &")
	return True

def checkConcurrent(num):
	success = True
	for x in range(num):
		t = subprocess.call("diff real temp" + str(x), shell=True)
		if (t == 0): 
			os.system("rm temp" + str(x))
		else:
			#os.system("rm temp" + str(x))
			success = False
	
	#os.system("rm real")
	return success



def singleRequest(filename):

	os.system("curl -s -x " + proxy + ":" + port + " " + url2 + " > " + filename)
	t = subprocess.call("diff real temp" + str(x), shell=True)
	os.system("rm temp" + str(x))

	if (t == 0): 
		return True
	else:
		return False

		


while (1):


	#success = runConcurrent(concurrentRuns)

	os.system("curl -s " + url2 + " > real")

	executor = ThreadPoolExecutor(max_workers=concurrentRuns)

	for x in range(concurrentRuns):	
		executor.submit(singleRequest("temp" + str(x)))
 


	#thread.start()

	# Wait here for the results to be available before continuing
	#thread.join()

	#success = checkConcurrent(concurrentRuns)
	os.system("rm real")

	if success:
		print("Successfully connected concurrently with " + str(concurrentRuns) + " clients")
		concurrentRuns = concurrentRuns + 10
	else:
		print("Unsuccessfully connected concurrently with " + str(concurrentRuns) + " clients")
		break;


