// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature is plugged into this digital port
#define ONE_WIRE_BUS 2

// Heat relay control is plugged into this digital port
#define HEAT_RELAY 4

// Valid temperature range
const double VALID_TEMP_LO = 10.0;
const double VALID_TEMP_HI = 120.0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

/*
 * The setup function. We only start the sensors here
 */
void setup(void) {
  Serial.println("Maische-Control initializing...");

  pinMode(HEAT_RELAY, OUTPUT);

  // start serial port
  Serial.begin(9600);

  // Start up the library
  sensors.begin();
  Serial.println("... Done");
}

/*
 * Main function, get and show the temperature
 */
void loop(void) { 
  sensors.requestTemperatures(); // Send the command to get temperatures
  double mTemp = sensors.getTempCByIndex(0);

  if(!validateTemp(mTemp)) {
    // Shutdown everything and stop doing anything
    emergencyMode();
    return;
  }
    
  boolean heat = decideToHeat(mTemp);
  
  Serial.print("Maische Temp is: ");
  Serial.print(mTemp);
  Serial.print(" - Heat decision: ");
  Serial.println(heat);

  if(heat)
    digitalWrite(HEAT_RELAY, LOW);
  else
    digitalWrite(HEAT_RELAY, HIGH);
  
  delay(2000);
}

void emergencyMode(void) {
  Serial.println("FATAL: Entering emergency mode!");

  // Shutting down the heat relay
  digitalWrite(HEAT_RELAY, HIGH);

  delay(2000);
}

boolean validateTemp(double temp) {
  if(temp < VALID_TEMP_LO or temp >= VALID_TEMP_HI)
    return false;
  else
    return true;
}

boolean decideToHeat(double temp) {
  if(temp <= 34.5)
    return true;
  else
    return false;
}

