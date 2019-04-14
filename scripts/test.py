import os
import sys
import difflib
import time

#os.system
port = sys.argv[1]
proxy = "comp112-03.cs.tufts.edu"
url1 = "http://www.cs.cmu.edu/~prs/bio.html"
url2 = "http://fossilinsects.myspecies.info/"
url3 = "http://portquiz.net:8080/"
url4 = "http://www.arcanedomain.com/Noahhead.jpg"
url5 = "http://www.theflowerfields.com/visitor-information/"

#Test 1: Text Based Files
os.system("curl -s " + url1 + " > real")
os.system("curl -s -x " + proxy + ":" + port + " " + url1 + " > ours")


if (os.system("diff real ours") == 0):
    print ("--------- TEST 1 (Text file diff) PASSED --------")
else:
    print ("--------- TEST 1 (Test file diff) FAILED --------")


os.system("rm real")
os.system("rm ours")

#Test 2: JPG files

os.system("curl -s " + url4 + " > real")
os.system("curl -s -x " + proxy + ":" + port + " " + url4 + " > ours")


if (os.system("diff real ours") == 0):
    print ("--------- TEST 2 (JPG file diff) PASSED --------")
else:
    print ("--------- TEST 2 (JPG file diff) FAILED --------")


os.system("rm real")
os.system("rm ours")

#Test 3: Inserting and fetching from cache - test passes based on time it took to retrieve

start = time.time()
os.system("curl -s -x " + proxy + ":" + port + " " + url3 + " > temp")
end = time.time()
noCache = end - start

start = time.time()
os.system("curl -s -x " + proxy + ":" + port + " " + url3 + " > temp")
end = time.time()
cache = end - start
if (cache < noCache):
    print ("--------- TEST 3 (Insert and fetch from cache) PASSED --------")
else:
    print ("--------- TEST 3 (Insert and fetch from cache) FAILED --------")

os.system("rm temp")





