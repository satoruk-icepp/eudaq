#!/usr/bin/env python

import socket
import sys
from thread import *
import threading
import time
import numpy as np

HOSTTCP = '127.0.0.1'
#HOSTTCP = '128.141.196.126'
PORTTCP = 55511
#HOSTUDP = '128.141.196.126'
HOSTUDP = '128.141.196.154'
PORTUDP = 55512

t1_stop = threading.Event()

EventRate = 20 # Events per spill
SpillTime = 5   # Duration of the spill in seconds
InterSpill = 5 # Time between spills in seconds


data32bit = False
print sys.argv
if len(sys.argv)>1 and sys.argv[1]=='32':
  data32bit = True  
  print 'Doing 32 bit data'

def sendFakeData_32bit(sudp, stop_event):

  # Here read the file with some raw data from HexaBoard
  fname = 'RUN_170317_0912.raw' # This file is not on github, get it elsewhere
  rawData = None
  try:
    with open(fname, "rb") as f:
      rawData = f.read()
  except EnvironmentError:
    print "Can't open file?", fname
    
  ev=0
  RAW_EV_SIZE = 30787
  
  while (not stop_event.is_set()):
    evmod = ev%500 # There are only 500 events in that file...

    if ev%EventRate==0:
      stop_event.wait(InterSpill)
      
    if rawData!=None:
      d = rawData[evmod*RAW_EV_SIZE:(evmod+1)*RAW_EV_SIZE]

      npd = np.fromstring(d, dtype=np.uint8)
      #print len(npd), npd.nbytes, npd
      npd32 = npd.astype(np.uint32)
      # print len(npd32), npd32.nbytes, npd32
      half1 = npd32[0:15500]
      half2 = npd32[15500:]
      half2.append(0x0a0b0c0d) # this is to check endienness
      print len(half1), half1.nbytes, len(half2), half2.nbytes
      # print half1
      
    else:
      d = 'Hello World'

    print 'submitting data', ev, evmod, len(d)

    try:
      # The data is too large now to send in one go. So, plit it in two packets:
      sudp.sendto(half1, (HOSTUDP,PORTUDP))
      sudp.sendto(half2, (HOSTUDP,PORTUDP))
      #conn.sendall('-->>>  Here is your data <<< --')
    except socket.error:
      print 'The client socket is probably closed. We stop sending data.'
      break
    time.sleep(SpillTime/EventRate)
    # stop_event.wait(SpillTime/EventRate)
    ev+=1
    



def sendFakeData_8bit(sudp, stop_event):
    
  # Here read the file with some raw data from HexaBoard
  fname = 'RUN_170317_0912.raw' # This file is not on github, get it elsewhere
  rawData = None
  try:
    with open(fname, "rb") as f:
      rawData = f.read()
  except EnvironmentError:
    print "Can't open file?", fname
    
  ev=0
  RAW_EV_SIZE = 30787
  
  while (not stop_event.is_set()):
    evmod = ev%500 # There are only 500 events in that file...

    if ev%EventRate==0:
      stop_event.wait(InterSpill)
      
    if rawData!=None:
      d = rawData[evmod*RAW_EV_SIZE:(evmod+1)*RAW_EV_SIZE]
      # d = rawData[evmod*RAW_EV_SIZE] # this should give ff always
    else:
      d = 'Hello World'
    print 'submitting data', ev, evmod, len(d)

    try:
        sudp.sendto(d, (HOSTUDP,PORTUDP))
        #conn.sendall('-->>>  Here is your data <<< --')
    except socket.error:
        print 'The client socket is probably closed. We stop sending data.'
        break
    time.sleep(SpillTime/EventRate)
    # stop_event.wait(SpillTime/EventRate)
    ev+=1
    

def clientthread(conn, sudp):
    #Sending message to connected client
    #conn.send('Welcome to the server. Receving Data...\n')

    #infinite loop so that function do not terminate and thread do not end.

    while True:
        #Receiving from client
        try:
            data = conn.recv(1024)
        except socket.error:
            print 'The client socket is probably closed. break this thread'
            break

        if not data:
            break
        
        print 'Command recieved:', str(data), type(data)

        if str(data)=='START_RUN':
            t1_stop.clear()
            conn.sendall('GOOD_START\n')
            #sudp.sendto('GOOD_START\n', (HOST,PORTUDP))
            time.sleep(0.2)
            if data32bit:
              start_new_thread(sendFakeData_32bit, (sudp,t1_stop))
            else:
              start_new_thread(sendFakeData_8bit, (sudp,t1_stop))
            print 'Sent GOOD_START confirmation'

        elif str(data)=='STOP_RUN':
            t1_stop.set()
            time.sleep(2)
            conn.sendall('STOPPED_OK')
            #sudp.sendto('STOPPED_OK', (HOST,PORTUDP))
            print 'Sent STOPPED_OK confirmation'
        else:
            conn.sendall('Message Received at the server: %s\n' % str(data))
            #sudp.sendto('Message Received at the server: %s\n' % str(data), (HOST,PORTUDP))


    conn.close()
            

if __name__ == "__main__":
  
  # This socket is to be used for communication:
  sTcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  
  print 'Socket created:', sTcp

  try:
    sTcp.bind((HOSTTCP, PORTTCP))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

  print 'Socket bind complete'

  sTcp.listen(3)
  print 'Socket now listening'

  # This one is to be used for sending the data
  sUdp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  
  print 'Socket created:', sUdp

  '''
  try:
    sUdp.bind((HOSTUDP, PORTUDP))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()
  '''
  
  try:
    while True:
      # wait to accept a connection
      conn, addr = sTcp.accept()
      print 'Connected with ' + addr[0] + ':' + str(addr[1])

      #start new thread
      start_new_thread(clientthread ,(conn,sUdp,))

  finally:
    sTcp.close()
    sUdp.close()
