#include <OneWire.h>
#include <DallasTemperature.h>

float curTemp = 0.;
short curMode = 0; // idle
boolean tempReached = false;
int tempReachedTime = 0;
float targetTemp = 0.;
int targetDuration = 0;
String addInfo = "idling";

const int RELAY_HEAT = 2;
const int ONE_WIRE_BUS = 2;

// Valid temperature range
const double VALID_TEMP_LO = 10.0;
const double VALID_TEMP_HI = 120.0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void) {
	Serial.begin(9600);
  pinMode(RELAY_HEAT, OUTPUT);
  sensors.begin();
}

void resetEverything() {
	curTemp = 0.;
	curMode = 0; // idle
	tempReached = false;
	targetTemp = 0.;
	targetDuration = 0;
	tempReachedTime = 0;
	addInfo = "idling...";
}

void sendStatus() {
	Serial.print(curMode);
  Serial.print(";");
	Serial.print(addInfo);
  Serial.print(";");
	
	if(curMode == "heat") {
		Serial.print(curTemp);
    Serial.print(";");
		Serial.print(tempReached);
    Serial.print(";");
		Serial.print(targetTemp);
    Serial.print(";");
    float tempTemp = (targetDuration * 60) - ((millis() - tempReachedTime) / 1000);
		Serial.print(tempTemp);
	}
		
	Serial.println();
  Serial.flush();
}

short readMode() {
  String incomingMsg = "";
  
	while(Serial.available() > 0) {
		// Only read the last incoming message!
 		incomingMsg = Serial.readStringUntil("\n");
	}
 
  Hier noch die verfickte scheisse einlesen in:
  mode (return)
  targetTemp und targetDuration = 0;

  return incomingMsg;
}

void idle() {
	digitalWrite(RELAY_HEAT, HIGH);
	addInfo = "Idling...";
}

void heat() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  float mTemp = sensors.getTempCByIndex(0);

	String addInfo = "";
  
	// Plausicheck for Temp
	if(curTemp < VALID_TEMP_LO or curTemp > VALID_TEMP_HI) {
		resetEverything();
		curMode = "error";
		idle();
		addInfo = "Temperature out of bounds";
		return;
	}
	
	// Hot enough?
	if(curTemp >= targetTemp) {
		digitalWrite(RELAY_HEAT, HIGH);
		addInfo = "Holding temperature...";
		
		if(!tempReached) {
			// Watermarking current heat period
			tempReachedTime = millis();
			tempReached = true;
			addInfo = "Target temperature reached";
		}
		
	} else {
		// HEAT!
		digitalWrite(RELAY_HEAT, LOW);
		addInfo = "Heating...";
	}
	
	if(tempReached && (millis() - tempReachedTime) / 1000 > targetDuration) {
		// This heating period has finished!
		resetEverything();
		curMode = "done";
	}
}

void loop() {
	short rMode = readMode();
	
	switch(curMode) {
		
		case 0: // idle
			switch(rMode) {
				default:
				case 0: // idle
					// Chill even harder!
					idle();
					chill = 2;
					break;
				case 1: // heat
					// Prepare for heating and continue immediately
					resetEverything();
					curMode = 1; // heat
					chill = 0;
					break;
			}
			break;
		
		case 1: // heat
			switch(rMode) {
				default:
				case 0: // idle
					resetEverything();
					chill = 1;
					break;
				case 1: //heat
					heat();
					chill = 1;
					break;
			}
			break;
			
		case 2: // done
			switch(rMode) {
				default:
				case 0: // idle
					resetEverything();
					idle();
					chill = 1;
					break;
				case 1: // heat
					// Prepare for heating and continue immediately
					resetEverything();
					curMode = 1; // heat
					chill = 0;
					break;
			}
			break;
	}
	
	sendStatus();
	sleep(chill);
}
