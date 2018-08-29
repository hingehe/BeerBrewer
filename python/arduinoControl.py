import time
import datetime
import json
import threading
from fakeArduino import Serial
from threading import Thread
from collections import deque

class _ArduinoStatus(object):
    def __init__(self):
        self.status = "idle"
        self.timestamp = time.time()
        self.temp = 0.0
        self.resTime = 0

    def update(self, incoming):
        self.status = incoming.split(";")[0]
        self.timestamp = time.time()
        if(self.status == "heat"):
            self.temp = incoming.split(";")[1]
            self.resTime = incoming.split(";")[2]

class ArduinoControl(object):
    def __init__(self):
        self.arduinoStatus = _ArduinoStatus()
        self.lastNStatus = deque(maxlen=20)
        self.arduinoSerial = Serial('/dev/ttyS1', 19200, timeout=1)
        self.currentOrder = []
        self.currentOrderFile = open("orderLog/default.json", "a")
        self.currentSequence = 0
        self.executeOrder = False
        self.thread = Thread(target=self._handleOrder, args=())     
        self._start()

    def _readArduinoStatus(self):
        val = self.arduinoSerial.readline()
        self.arduinoStatus.update(val)

    def getStatus(self):
        return json.dumps(self.arduinoStatus.__dict__)

    def _reinitOrderLog(self, name):
        self.currentOrderFile.close()
        self.currentOrderFile = open("orderLog/"+name+"_"+str(time.time())+".json", "a")

    def getLastNStatus(self):
        return json.dumps(list(self.lastNStatus))
    
    def newOrder(self, newOrder):
        print("New Order received, killing old one...")
        self.stopOrder()
        time.sleep(2)
        print("Saving new Order")
        self.currentOrder = newOrder
        self.executeOrder = True
        self._reinitOrderLog(self.currentOrder["BrauOrder"]["name"])
        print("Triggering Arduino")
        self._setStatus()

    def stopOrder(self):
        self.arduinoSerial.write("idle;\n")
        self.currentOrder = []
        self.executeOrder = False
        self.currentSequence = 0

    def _handleOrder(self):
        t = threading.currentThread()
        while getattr(t, "do_run", True):
            self._readArduinoStatus()
            self.lastNStatus.append(self.getStatus())

            if(self.arduinoStatus.status != 'idle'):
                json.dump(self.arduinoStatus.__dict__, self.currentOrderFile)
                self.currentOrderFile.write("\n")
                self.currentOrderFile.flush()

            if(self.arduinoStatus.status == "done" and self.executeOrder):
                #Hier noch kieken, ob die ganze order nicht schon fertig ist!
                self.currentSequence += 1
                self._setStatus()
            time.sleep(1)

    def _setStatus(self):
        commandString = "heat;"
        commandString += str(self.currentOrder["BrauOrder"]["MaischePlan"][self.currentSequence]["temp"])+";"
        commandString += str(self.currentOrder["BrauOrder"]["MaischePlan"][self.currentSequence]["duration"]+";\n")
        self.arduinoSerial.write(commandString)

    def forceStatus(self, status):
        self.arduinoSerial.write(status+";\n")
        
    def _start(self):
        print("Starting Arduino communication")
        self.thread.start()

    def stop(self):
        print("Stopping Arduino communication...")
        self.thread.do_run = False
        self.currentOrderFile.close()
        self.thread.join()
        print("Bye")
