#include <OneWire.h>
#include <DallasTemperature.h>

float curTemp = 0.;
short curMode = 0; // idle
boolean tempReached = false;
int tempReachedTime = 0;
float targetTemp = 0.;
int targetDuration = 0;
String addInfo = "idling";
char lastMsg[40];

const int RELAY_HEAT = 4;
const int ONE_WIRE_BUS = 2;

// Valid temperature range
const double VALID_TEMP_LO = 10.0;
const double VALID_TEMP_HI = 120.0;

const int SERIAL_BUFFER_SIZE = 30;
char serial_buffer[SERIAL_BUFFER_SIZE];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup(void) {
	Serial.begin(9600);
  pinMode(RELAY_HEAT, OUTPUT);
  sensors.begin();
  curMode = 0; // idle
}

void resetEverything(void) {
	curTemp = 0.;
	tempReached = false;
	targetTemp = 0.;
	targetDuration = 0;
	tempReachedTime = 0;
}

bool readSerial() {
  static byte index;

  while (Serial.available()) {
    char c = Serial.read();

    if (c >= 32 && index < SERIAL_BUFFER_SIZE - 1) {
      serial_buffer[index++] = c;
    } else if (c == '\n' && index > 0) {
      serial_buffer[index] = '\0';
      index = 0;
      return true;
    }
  }
  
  return false;
}

short parseSerial() {
  short rMode = atoi(strtok(serial_buffer, ";"));
  if(rMode == 1) { // heat
    targetTemp = atoi(strtok(NULL, ";"));
    targetDuration = atoi(strtok(NULL, ";"));
  }
  
  return rMode;
}

void sendStatus() {
	Serial.print(curMode);
  Serial.print(";");
	Serial.print(addInfo);
  Serial.print(";");
	
	if(curMode == 1) { // heat
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
		curMode = 3; // error
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
		curMode = 2; // done
	}
}

void loop() {
  short rMode = curMode;
  short chill = 1;

  if (readSerial())
    rMode = parseSerial();

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
	delay(chill);
}
