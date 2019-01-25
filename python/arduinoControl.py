################################################################################
# Controller class for Arduino BeerBrewer (temperature control and stiring)
# Is managed by Flask Webservice and basically translates json brew receipies
# into arduino actions.
# 
# Date: 20181015
# Author: Sebastian Sauer
#
# TODO:
# * Queue basteln, damit der Webservice die lastNSTatus melden kann
# * ValueError: I/O operation on closed file. beim initialen ctrl+c??
# * File logging does not work (wrong numbers)
#
# History:
# 0.1 - 20181015 - sauer - Alpha Version
# 0.5 - 20190123 - sauer - Beta Version (running without stir)
################################################################################

import time
import datetime
import json
import threading
import serial
from threading import Thread
from collections import deque

BEER_HOME = "/home/pi/BeerBrewerFiles"
ARDUINO_PORT = "/dev/ttyACM0"

class _ArduinoStatus(object):
    def __init__(self):
        self.status = 0 # idle
        self.timestamp = time.time()
        self.temp = 0.0
        self.resTime = 0

    def update(self, incoming):
        self.status = incoming.split(";")[0]
        print("received status: "+incoming)
        self.timestamp = time.time()

        if(self.status == 1): # heat
            self.temp = incoming.split(";")[1]
            self.resTime = incoming.split(";")[3]

class ArduinoControl(object):
    def __init__(self):
        self.arduinoStatus = _ArduinoStatus()
        self.lastNStatus = deque(maxlen=20)
        self.arduinoSerial = serial.Serial(ARDUINO_PORT, 9600, timeout=2)
        self.currentOrder = []
        self.currentOrderFile = open(BEER_HOME+"/default.json", "a")
        self.currentSequence = 0
        self.executeOrder = False
        self.thread = Thread(target=self._handleOrder, args=())     
        self._start()

    def _readArduinoStatus(self):
        rawMsg = "void"
        
        try:
            rawMsg = self.arduinoSerial.readline()
            decodedMsg = rawMsg[0:len(rawMsg)-2].decode()
            self.arduinoStatus.update(decodedMsg)
        except:
            print("Error reading Arduino Status: \n"+str(rawMsg))
            self.stop()

    def getStatus(self):
        return json.dumps(self.arduinoStatus.__dict__)

    def _reinitOrderLog(self, name):
        self.currentOrderFile.close()
        newFileName = BEER_HOME+"/"+name+"_"+str(time.time())+".json"
        self.currentOrderFile = open(newFileName, "a")
        print("Reinit order file to: "+newFileName)

    def getLastNStatus(self):
        return json.dumps(list(self.lastNStatus))
    
    def newOrder(self, newOrder):
        print("New Order received, killing old one...")
        self.stopOrder()
        time.sleep(2)
        print("Saving new Order with "+str(len(newOrder["BrauOrder"]["MaischePlan"]))+" cycles")
        self.currentOrder = newOrder
        self.executeOrder = True
        self._reinitOrderLog(self.currentOrder["BrauOrder"]["name"])
        print("Triggering Arduino")
        self._setStatus()

    def stopOrder(self):
        print("Force Arduino idle...")
        self.arduinoSerial.write("0;\n".encode()) # idle
        self.currentOrder = []
        self.executeOrder = False
        self.currentSequence = 0

    def _handleOrder(self):
        t = threading.currentThread()
        while getattr(t, "do_run", True):
            self._readArduinoStatus()
            self.lastNStatus.append(self.getStatus())

            if(self.arduinoStatus.status != '0'): # idle
                json.dump(self.arduinoStatus.__dict__, self.currentOrderFile)
                self.currentOrderFile.write("\n")
                self.currentOrderFile.flush()

            if(self.arduinoStatus.status == "2" and self.executeOrder): # done
                if(self.currentSequence + 1 < len(self.currentOrder["BrauOrder"]["MaischePlan"])):
                    self.currentSequence += 1
                    self._setStatus()
                else:
                    self.forceStatus("0"); # idle

            time.sleep(1)

    def _setStatus(self):
        commandString = "1;" # heat
        commandString += str(self.currentOrder["BrauOrder"]["MaischePlan"][self.currentSequence]["temp"])+";"
        commandString += str(self.currentOrder["BrauOrder"]["MaischePlan"][self.currentSequence]["duration"]+";\n")
        print("Sending: "+commandString)
        self.arduinoSerial.write(commandString.encode())

    def forceStatus(self, status):
        msg = str(status)+";\n"
        self.arduinoSerial.write(msg.encode())
        
    def _start(self):
        print("Starting Arduino communication")
        self.thread.start()

    def stop(self):
        self.stopOrder()
        print("Stop Arduino communication...")
        self.thread.do_run = False
        self.currentOrderFile.close()
        self.thread.join()
        print("Bye")
