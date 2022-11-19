#!/usr/bin/python3
#==================================================================
# Translate data from pico sensor_remote wget output to a compact log file.
#==================================================================
import time

Anm_freq = "    "
temps = {'000008e37318': '?????', '000008e39594': '?????', '000008e38375': '?????', '3c01a81652df': '?????'}

for line in open("index.html", "r"):
    #print(line)
    if line[:7] == "Anm_frq":
        Anm_freq = line[9:13]
        #print("xx",Anm_freq)
    elif line[:2] == "t(":
        id=line[2:14]
        temp=line[16:21]
        #print(id,temp)
        temps[id]=temp
       

tm = time.localtime()
timestr = time.strftime("%d-%b-%y %H:%M:%S, ", tm)

print (timestr+Anm_freq+","+temps['000008e37318']+","+temps['000008e39594']+","+temps['000008e38375']+","+temps['3c01a81652df'])
   
        
