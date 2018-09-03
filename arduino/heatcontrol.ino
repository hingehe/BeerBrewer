curTemp = 0.
curMode = "idle"
tempReached = false
tempReachedTime = 0
targetTemp = 0.
targetDuration = 0
addInfo = "idling"

init() {
	startSerial(9600)
}

resetEverything() {
	curTemp = 0.
	curMode = "idle"
	tempReached = false
	targetTemp = 0.
	targetDuration = 0
	tempReachedTime = 0
	addInfo = "idling..."
}

sendStatus() {
	statusMsg = curMode+";";
	statusMsg += addInfo+";";
	
	if(curMode == "heat") {
		statusMsg += curTemp+";";
		statusMsg += tempReached+";";
		statusMsg += targetTemp+";";
		statusMsg += (targetDuration * 60) - ((time.millis() - tempReachedTime) / 1000) #Orly?
	}
	
	statusMsg += "\n";
	
	Serial.write(statusMsg);
}

readMode() {
	while(Serial.stillAmStart())
		# Only read the last incoming message!
		incomingMsg = Serial.readln();		
}

idle() {
	digitalWrite(RELAY, HIGH)
	addInfo = "Idling..."
}

heat() {
	curTemp = readTemp();
	
	// Plausicheck for Temp
	if(curTemp < LOWER or curTemp > HIGHER) {
		resetEverything();
		curMode = "error";
		idle();
		addInfo = "Temperature out of bounds: "+str(curTemp);
		return;
	}
	
	// Hot enough?
	if(curTemp >= targetTemp) {
		digitalWrite(RELAY, HIGH);
		addInfo = "Holding temperature..."
		
		if(!tempReached) {
			// Watermarking current heat period
			tempReachedTime = time.millis()
			tempReached = true;
			addInfo = "Target temperature reached at "+str(tempReachedTime);
		}
		
	} else {
		// HEAT!
		digitalWrite(RELAY, LOW)
		addInfo = "Heating..."
	}
	
	if(tempReached && (time.millis() - tempReachedTime) / 1000 > targetDuration) {
		// This heating period has finished!
		resetEverything();
		mode = "done";
	}
}

loop() {
	readMode = readMode()
	
	switch(curMode) {
		
		case "idle":
			switch(readMode) {
				default:
				case "idle":
					// Chill even harder!
					idle()
					chill = 2
					break;
				case "heat":
					// Prepare for heating and continue immediately
					resetEverything();
					curMode = "heat"
					readTargets()
					chill = 0
					break;
			}
			break;
		
		case "heat":
			switch(readMode) {
				default:
				case "idle":
					resetEverything()
					chill = 1;
					break;
				case "heat":
					heat();
					chill = 1;
					break;
			}
			break;
			
		case "done":
			switch(readMode) {
				default:
				case "idle":
					resetEverything();
					idle();
					chill = 1;
					break;
				case "heat":
					// Prepare for heating and continue immediately
					resetEverything();
					curMode = "heat"
					readTargets()
					chill = 0
					break;
			}
			break;
	}
	
	sendStatus();
	sleep(chill)
}
