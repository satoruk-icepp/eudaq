#!/usr/bin/env python

import socket
import sys
from thread import *
import threading
import time

# HOST = '128.141.89.137'
HOST = '127.0.0.1'
PORT = 55511


t1_stop = threading.Event()

EventRate = 100 # Events per spill
SpillTime = 5   # Duration of the spill in seconds
InterSpill = 5 # Time between spills in seconds

def sendFakeData(conn, stop_event):
  print 'This is conn:', conn

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
        conn.sendall(d)
        #conn.sendall('-->>>  Here is your data <<< --')
    except socket.error:
        print 'The client socket is probably closed. We stop sending data.'
        break
    time.sleep(SpillTime/EventRate)
    # stop_event.wait(SpillTime/EventRate)
    ev+=1
    

def clientthread(conn):
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
            time.sleep(0.2)
            start_new_thread(sendFakeData, (conn,t1_stop))

        elif str(data)=='STOP_RUN':
            t1_stop.set()
            time.sleep(2)
            conn.sendall('STOPPED_OK')
            print 'Sent STOPPED_OK confirmation'
        else:
            conn.sendall('Message Received at the server: %s\n' % str(data))


    conn.close()
            

if __name__ == "__main__":

  # blink is only for RPI tests
  # from blink import *

  #Blink(8, 3,0.1)

  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  
  print 'Socket created:', s

  try:
    s.bind((HOST, PORT))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

  print 'Socket bind complete'

  s.listen(3)
  print 'Socket now listening'


  try:
    while True:
      # wait to accept a connection
      conn, addr = s.accept()
      print 'Connected with ' + addr[0] + ':' + str(addr[1])

      #start new thread
      start_new_thread(clientthread ,(conn,))

  finally:
    s.close()
    
    #Blink(8, 3,1)
