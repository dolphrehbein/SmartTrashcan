#include <ADXL362DMA.h>
#include <PowerShield.h>
#include <Adafruit_GPS.h>
#include "HC_SR04.h"


//for Ping sensor
#define trigPin A0
#define echoPin A1

//for GPS
#define GPSSerial Serial1
#define GPSECHO false

//for IR sensor
// Sharp IR GP2Y0A41SK0F (4-30cm, analog)
#define IRsensor A6 



//Global Variables
//For Ping sensor
double cm = 0.0;
double inches = 0.0;
int trash;
HC_SR04 rangefinder = HC_SR04(trigPin, echoPin);

//For battery
int battery;
double cellVoltage;
double stateOfCharge;
PowerShield batteryMonitor;

//For GPS
// Connect to the GPS on the hardware Serial port
Adafruit_GPS GPS(&GPSSerial);
uint32_t timer = millis();

//For accelerometer
SYSTEM_THREAD(ENABLED);
ADXL362DMA accel(SPI, A2);
// This is the pin connected to the INT1 pin on the ADXL362 breakout
// Don't use D0 or D1; they are used for I2C to the PowerShield battery monitor
const int ACCEL_INT_PIN = D2;
const unsigned long MAX_TIME_TO_PUBLISH_MS = 30000; // Only stay awake for 30 seconds trying to connect to the cloud and publish
const unsigned long TIME_AFTER_PUBLISH_MS = 4000; // After publish, wait 4 seconds for data to go out
const unsigned long TIME_AFTER_BOOT_MS = 10000; // At boot, wait 10 seconds before going to sleep again
const unsigned long TIME_PUBLISH_BATTERY_SEC = 60*60; // every hour send a battery update in addition to the autowake samples.
// Stuff for the finite state machine
enum State { RESET_STATE, RESET_WAIT_STATE, PUBLISH_STATE, SLEEP_STATE, SLEEP_WAIT_STATE, BOOT_WAIT_STATE };
State state = RESET_STATE;
unsigned long stateTime = 0;
int awake = 0;



double convertDegMinToDecDeg (float degMin) {
    double min = 0.0;
    double decDeg = 0.0;
    //get the minutes, fmod() requires double
    min = fmod((double)degMin, 100.0);
 
    //rebuild coordinates in decimal degrees
    degMin = (int) ( degMin / 100 );
    decDeg = degMin + ( min / 60 );
    return decDeg;
}


void checkBattery()
{
    // Read the volatge of the LiPo
    cellVoltage = batteryMonitor.getVCell();
    // Read the State of Charge of the LiPo
    stateOfCharge = batteryMonitor.getSoC();
    battery = (int)stateOfCharge;
    battery = min(100,battery);
}

void checkTrashIR()
{
    double volts;
    double sum = 0;
    double sample;
    double denominator = 0;
    double distance;
    double invtrash;
    
    //averaging out 10 measurments
    for(unsigned char i= 0;i<10;i++)
    {
        // value from sensor * (3.3/4095)
        volts =  analogRead(IRsensor)*0.0008058608059;  
        // worked out from datasheet graph
        distance = 12*pow(volts, -1);
        //scaling from 0-100 trash percentage 
        sample = (distance - 5.8) * (100/17);
        sum += sample;
        denominator++;
        delay(100);
    }
    
    invtrash = sum / denominator;
    trash = 100 - invtrash;
    trash = max(0,trash);
    trash = min(100,trash);
}


//Used for the Ping Sensor. However I found it to be unreliable so i'm using the IR sensor instead.
void checkTrash()
{
    double sum = 0;
    double sample = 0;
    double denominator = 0;
    
    for(unsigned char i= 0;i<10;i++)
    {
        sample = rangefinder.getDistanceCM();
        if (sample == -1)
        {
            //if sample is negative 1 the data is bad
            sum += 0;
        }
        else
        {
            sum += sample;
            denominator++;
        }
        //data sheet asks for at least 50 us between measurments
        delay(1);
    }
    
    if(sum == 0)
    {
        cm = 0;
    }
    else
    {
        cm = sum / denominator;
    }
    //scaling for the 0-100 trash scale.
    trash = (11 - cm/2.54)*(100/11);
    trash = max(0,trash);
}

void publishTrashIR()
{
    checkTrashIR();
    Particle.publish("trash", (String)trash, 60, PUBLIC);
}

void publishTrash()
{
    checkTrash();
    Particle.publish("trash", (String)trash, 60, PUBLIC);
}

void publishBattery()
{
    checkBattery();
    Particle.publish("battery", (String)battery, 60, PUBLIC);
}

//based on the Adafruit GPS library example GPS_Hardware_Serial_Parsing
void publishGPS()
{   
    while(1)  
    {       
        // read data from the GPS
        char c = GPS.read();
        
        //checks to see if a new NMEA message is recieve.
        //if the last NMEA message was unable to be parsed wait for the next one.
        //loop until you parse a message... It's a trash can we have time.
        if (GPS.newNMEAreceived()) {
            if (!GPS.parse(GPS.lastNMEA()))
            continue; 
            }
     
        // approximately every 2 seconds or so, print out the current stats
        if (millis() - timer > 5000) {
            timer = millis(); // reset the timer
            //If the GPS has a connection to satellites print out the publish the lattitude and longitude so we can find our trash can.
            if (GPS.fix) {
            double latitude = GPS.latitude;
            double longitude = GPS.longitude;
            Particle.publish("latitude", String(convertDegMinToDecDeg(latitude))+String(GPS.lat), 60, PUBLIC);
            Particle.publish("longitude", String(convertDegMinToDecDeg(longitude))+String(GPS.lon), 60, PUBLIC);
            return;
            }
        }
    }
}


void setup() 
{
    //for Ping sensor
    Spark.variable("cm", &cm, DOUBLE);
    Particle.variable("trash", trash);
    
    
    //for battery
    Particle.variable("stateOfCharge", stateOfCharge);
    Particle.variable("battery", battery);
    batteryMonitor.begin(); 
    batteryMonitor.quickStart();
    
    //for GPS
    GPS.begin(9600);
    //parse RMC data only because that's all we need for the location.
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    //At a 1 Hz update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    //delay for Battery and GPS setup
    delay(1000);
}


//Finite State Machine to wake up the trash can when the lid moves or after a certain amount of time to take a sample.
//Based on Rickkas7's code
//https://github.com/rickkas7/AccelWake
void loop() 
{
	uint8_t status;

	switch(state) {
	case RESET_STATE:
		accel.softReset();
		state = RESET_WAIT_STATE;
		break;

	case RESET_WAIT_STATE:
		status = accel.readStatus();
		if (status != 0) {
			// Reset complete
			// Set accelerometer parameters
			// Set sample rate to 50 samples per second
			accel.writeFilterControl(accel.RANGE_2G, false, false, accel.ODR_50);

			// Set activity and inactivity thresholds, change these to make the sensor more or less sensitive
			accel.writeActivityThreshold(250);
			accel.writeInactivityThreshold(150);

			// Activity timer is not used because when inactive we go into sleep mode; it automatically wakes after 1 sample
			// Set inactivity timer to 100 or 2 seconds at 50 samples/sec
			accel.writeInactivityTime(100);

			// Enable loop operation (automatically move from activity to inactivity without having to clear interrupts)
			// Enable activity and inactivity detection in reference mode (automatically accounts for gravity)
			accel.writeActivityControl(accel.LINKLOOP_LOOP, true, true, true, true);

			// Map the AWAKE bit to INT1 so activity can wake up the Photon
			accel.writeIntmap1(accel.STATUS_AWAKE);

			// Enable measuring mode with auto-sleep
			accel.writePowerCtl(false, false, false, true, accel.MEASURE_MEASUREMENT);

			state = BOOT_WAIT_STATE;
		}
		break;

	case PUBLISH_STATE:
		if (Particle.connected()) {
            //Publish the GPS location, Trash level from the IR sensor and the battery percentage.
            publishGPS();
            publishTrashIR();
            publishBattery();
			// Wait for the publish to go out
			stateTime = millis();
			state = SLEEP_WAIT_STATE;
		}
		else {
			// Haven't come online yet
			if (millis() - stateTime >= MAX_TIME_TO_PUBLISH_MS) {
				// Took too long to publish, just go to sleep
				state = SLEEP_STATE;
			}
		}
		break;

	case SLEEP_WAIT_STATE:
		if (millis() - stateTime >= TIME_AFTER_PUBLISH_MS) {
			state = SLEEP_STATE;
		}
		break;

	case BOOT_WAIT_STATE:
		if (millis() - stateTime >= TIME_AFTER_BOOT_MS) {
			// To publish the trashcan stats after boot, set state to PUBLISH_STATE
			// To go to sleep immediately, set state to SLEEP_STATE
			state = PUBLISH_STATE;
		}
		break;

	case SLEEP_STATE:
		// Sleep
		System.sleep(ACCEL_INT_PIN, RISING, TIME_PUBLISH_BATTERY_SEC);
		delay(1000);

		awake = ((accel.readStatus() & accel.STATUS_AWAKE) != 0);

		state = PUBLISH_STATE;
		stateTime = millis();
		break;
	}
}

