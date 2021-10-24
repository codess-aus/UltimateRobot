// #define CAYENNE_DEBUG Serial
// #define CAYENNE_PRINT Serial

#include <Arduino.h>
#include <CayenneMQTTESP8266.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <EEPROM.h>

extern "C" {
#include "user_interface.h"
}

RF_MODE(RF_NO_CAL);// tells the ESP to just turn on radio, no calibration which extra takes power


char ssid[] = "IoT";
char wifiPassword[] = "open4meplease";

char username[] = "a4f0c830-bf24-11e6-9638-53ecf337e03f";
char password[] = "65733121a106fc7879f76d6c999a90e130858dfe";
char clientID[] = "c82b0a50-9c9a-11e7-b177-579293954599";


Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *moto1 = AFMS.getMotor(1);
Adafruit_DCMotor *moto2 = AFMS.getMotor(2);


// resistor values for volt divider
// R1 = 7950;
// R2 = 2176;
// voltFactor = R2 / (R1 + R2);

// float voltFactor = 0.2149; // orginal
double voltFactor = 0.2049;  // adjusted
float voltage;
float rawVolt;


// pins
const int battPin = A0;
const int ledBluePin = 2;
const int ledRedPin = 0;
const int irPin = 14;
const int irPinPwr = 13;
const int pirPin = 15;
const int pirPinPwr = 12;


// Settings
const int eepromAddress = 0;
const int drive2eepromAddress = 1;
const int sleepTime = 5;  // minutes
const int timezone = -5;  // hours
const int dst = 0;
int maxRandSleepDur = 60;  // minutes

// delays in seconds
const byte objectCountNum = 4;
const byte backupTimeDelay = 3;
const byte rotationTime = 3;
const byte objectZeroReset = 10;


byte disco = 0;
byte randomBackup;
byte objectCount = 0;
byte forwardButton = 0;
byte backwardButton = 0;
byte leftButton = 0;
byte rightButton = 0;
byte manualControl = 0;
byte sleepCycle;
byte timeHour;
byte timeMin;


bool drive1 = false;
bool drive2 = false;
bool sleepNow = false;
bool object = false;
bool objectPIR = false;
bool online = false;
bool drive2FirstRun = true;

byte directionBut;
char direction;



unsigned long time1;
unsigned long time2;
unsigned long time3;
unsigned long onlineTime;
unsigned long prevTime;
unsigned long objectTime = 0;
unsigned long backupTime = 0;
unsigned long randomTime1 = 0;
unsigned long randomTime2 = 0;
unsigned long randomTime3 = 0;








void setup()
{
	pinMode(battPin, INPUT);
	pinMode(pirPin, INPUT);
	pinMode(irPin, INPUT_PULLUP);
	pinMode(irPinPwr, OUTPUT);
	pinMode(pirPinPwr, OUTPUT);
	pinMode(ledBluePin, OUTPUT);
	pinMode(ledRedPin, OUTPUT);
	// pinMode(LED_BUILTIN, OUTPUT);

	digitalWrite(ledBluePin, HIGH);
	digitalWrite(ledRedPin, HIGH);

	wifi_set_phy_mode(PHY_MODE_11N);

	// Serial.begin(9600);
	// delay(10);

	EEPROM.begin(512);
	sleepCycle = EEPROM.read(eepromAddress); //checks for sleep cycle
	drive2 = EEPROM.read(drive2eepromAddress); //checks for drive2 mode

	if (sleepCycle == 1) 
	  {
		  for (byte i = 0; i < 3; i++)
		    {
			    digitalWrite(ledBluePin, LOW);
			    delay(50);
			    digitalWrite(ledBluePin, HIGH);
			    digitalWrite(ledRedPin, LOW);
			    delay(50);
			    digitalWrite(ledRedPin, HIGH);
		    }
		  digitalWrite(pirPinPwr, HIGH);
		  delay(1000);
		  if (digitalRead(pirPin) == 1)
		    {
			    sleepCycle = 0;
			    EEPROM.write(eepromAddress, 0);
			    EEPROM.commit();
			    digitalWrite(pirPinPwr, LOW);
			    digitalWrite(ledRedPin, LOW);
			    digitalWrite(ledBluePin, LOW);
			    delay(1000);
			    digitalWrite(ledRedPin, HIGH);
			    digitalWrite(ledBluePin, HIGH);
		    }
		  else
		    {
			    digitalWrite(pirPinPwr, LOW);
			    moto1->run(RELEASE);
			    moto2->run(RELEASE);
			    ESP.deepSleep(sleepTime * 60000 * 1000);
		    }
	  }


	AFMS.begin();

	setRTC();

	Cayenne.begin(username, password, clientID, ssid, wifiPassword);

	if (drive2 == 1)
	  {
		  Cayenne.virtualWrite(9, 1, "digital_sensor", "d");
	  }
	else
	  {
		  Cayenne.virtualWrite(9, 0, "digital_sensor", "d");
	  }
	Cayenne.virtualWrite(10, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(11, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(14, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(15, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(16, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(17, 0, "digital_sensor", "d");

}








void loop()
{
	Cayenne.loop();
	maint();
	pollSensors();
	// randomSleep();

	yield();




	if (manualControl == 0)
	  {
		  digitalWrite(irPinPwr, HIGH); //turn on IR and PIR
		  digitalWrite(pirPinPwr, HIGH);

		  if (drive1 == true)
		    {
			    moto1->run(FORWARD);
			    moto1->setSpeed(255);
			    moto2->run(FORWARD);
			    moto2->setSpeed(255);

			    checkIR();
			    timers();
		    }

		  if (drive2 == true)
		    {
			    if (drive2FirstRun == true)
			      {
				      drive2FirstRun = false;
				      randomActions();  // get random times
			      }

			    checkIR();
			    timers();

			    if (randomTime2 - millis() >= 0)
			      {
				      moto1->run(FORWARD);
				      moto1->setSpeed(255);
				      moto2->run(FORWARD);
				      moto2->setSpeed(255);
			      }

			    if (randomTime3 - millis() >= 0 && randomTime2 - millis() >= 0)
			      {
				      moto1->run(RELEASE);
				      moto2->run(RELEASE);
				      sleepCycle = 0;
				      EEPROM.write(eepromAddress, 0);
				      EEPROM.commit();
				      ESP.deepSleep(randomTime3 * 1000);

			      }

		    }

		  if (drive1 == false && drive2 == false)
		    {
			    moto1->run(RELEASE);
			    moto2->run(RELEASE);
		    }
	  }
	else
	  {
		  digitalWrite(irPinPwr, LOW);
		  digitalWrite(pirPinPwr, LOW);
		  manualDrive();
	  }
}







void manualDrive()
{
	if (forwardButton == 1)
	  {
		  moto1->run(FORWARD);
		  moto1->setSpeed(255);
		  moto2->run(FORWARD);
		  moto2->setSpeed(255);
		  delay(1000);
	  }

	if (backwardButton == 1)
	  {
		  moto1->run(BACKWARD);
		  moto1->setSpeed(255);
		  moto2->run(BACKWARD);
		  moto2->setSpeed(255);
		  delay(1000);
	  }

	if (leftButton == 1)
	  {
		  moto1->run(FORWARD);
		  moto1->setSpeed(255);
		  moto2->run(FORWARD);
		  moto2->setSpeed(50);
		  delay(1000);
	  }

	if (rightButton == 1)
	  {
		  moto1->run(FORWARD);
		  moto1->setSpeed(50);
		  moto2->run(FORWARD);
		  moto2->setSpeed(255);
		  delay(1000);
	  }

	if (forwardButton == 0 && leftButton == 0 && rightButton == 0 && backwardButton == 0)
	  {
		  moto1->run(RELEASE);
		  moto2->run(RELEASE);
	  }

	Cayenne.virtualWrite(10, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(14, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(15, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(16, 0, "digital_sensor", "d");
	Cayenne.virtualWrite(17, 0, "digital_sensor", "d");
	forwardButton = 0;
	leftButton = 0;
	rightButton = 0;
	backwardButton = 0;
}





void checkIR()
{
	pollSensors();

	objectTime = millis();

	if (object == 0)
	  {
		  objectCount++;

		  moto1->run(RELEASE);
		  moto2->run(RELEASE);

		  delay(150);

		  randomBackup = random(1);

		  if (objectCount > objectCountNum)
		    {
			    Cayenne.loop();
			    backupTime = millis();
			    while (millis() - backupTime < ( backupTimeDelay * 1000 ))
			      {
				      moto1->run(BACKWARD);
				      moto1->setSpeed(255);
				      moto2->run(BACKWARD);
				      moto2->setSpeed(255);
				      yield();
			      }

			    Cayenne.loop();

			    while (millis() - backupTime >= ( backupTimeDelay * 1000 ) && millis() - backupTime <= ( rotationTime * 1000 ))
			      {

				      moto1->run(FORWARD);
				      moto1->setSpeed(255);
				      moto2->run(BACKWARD);
				      moto2->setSpeed(255);
				      yield();
			      }


		    }

		  while (object == 0)
		    {
			    Cayenne.loop();
			    backupTime = millis();

			    switch (randomBackup)
			      {
			      case 0:
			      {
				      moto1->run(BACKWARD);
				      moto1->setSpeed(100);
				      moto2->run(BACKWARD);
				      moto2->setSpeed(255);
				      yield();

				      while (( millis() - backupTime ) < 1500)
					{
						delay(0);
						Cayenne.loop();
					}
			      }

			      case 1:
			      {
				      moto1->run(BACKWARD);
				      moto1->setSpeed(255);
				      moto2->run(BACKWARD);
				      moto2->setSpeed(100);
				      yield();

				      while (( millis() - backupTime ) < 1500)
					{
						delay(0);
						Cayenne.loop();
					}
			      }
			      }

			    object = digitalRead(irPin);
		    }

		  if (randomBackup == 0)
		    {
			    moto1->run(FORWARD);
			    moto1->setSpeed(255);
			    moto2->run(FORWARD);
			    moto2->setSpeed(50);
		    }
		  else
		    {
			    moto1->run(FORWARD);
			    moto1->setSpeed(50);
			    moto2->run(FORWARD);
			    moto2->setSpeed(255);
		    }

		  Cayenne.loop();
		  delay(1000);


	  }
	else
	  {
		  Cayenne.virtualWrite(20, 0, "digital_sensor", "d");
	  }
}



void pollSensors()
{
	object = digitalRead(irPin);
	objectPIR = digitalRead(pirPin);

	if (object == 0)
	  {
		  Cayenne.virtualWrite(20, 1, "digital_sensor", "d");
		  digitalWrite(ledRedPin, LOW);
	  }
	else
	  {
		  Cayenne.virtualWrite(20, 0, "digital_sensor", "d");
		  digitalWrite(ledRedPin, HIGH);
	  }

	if (objectPIR == 1)
	  {
		  Cayenne.virtualWrite(21, 1, "digital_sensor", "d");
	  }
	else
	  {
		  Cayenne.virtualWrite(21, 0, "digital_sensor", "d");
	  }
}



void timers()
{
	if (millis() - objectTime > ( objectZeroReset * 1000 ))
	  {
		  objectCount = 0;
	  }

	if (timeHour > 23 || timeHour < 10)
	  {
		  moto1->run(RELEASE);
		  moto2->run(RELEASE);
		  ESP.deepSleep(sleepTime * 60000 * 1000);
	  }

}


void maint()
{
	if (millis() - time2 > 10000)
	  {
		  Cayenne.virtualWrite(23, disco);

		  time1 = ( millis() / 1000 ) / 60;
		  Cayenne.virtualWrite(24, time1);

		  onlineTime = ( ( millis() - prevTime ) / 1000 ) / 60;
		  Cayenne.virtualWrite(25, onlineTime);

		  int reading = analogRead(battPin);
		  rawVolt = float(reading) / 1024;
		  voltage = ( rawVolt / voltFactor );
		  Cayenne.virtualWrite(22, voltage);

		  Serial.println(reading);
		  Serial.println(rawVolt);
		  Serial.println(voltage);

		  refreshRTC();

		  Cayenne.virtualWrite(5, timeHour);
		  Cayenne.virtualWrite(6, timeMin);



		  time2 = millis();

			Cayenne.loop();

		  if (sleepCycle == 1) // if sleep is active, chip sleeps after data transmit
		    {
			    ESP.deepSleep(sleepTime * 60000 * 1000);
		    }
	  }

	// if (millis() - time3 > 5000 && online == true)
	//   {
	//        time3 = millis();
	//   }
}




void redLED()
{
	if (digitalRead(ledRedPin) == 0)
	  {
		  digitalWrite(ledRedPin, 1);
	  }
	else
	  {
		  digitalWrite(ledRedPin, 0);
	  }
}


void setRTC()
{
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, wifiPassword);

	while (WiFi.status() != WL_CONNECTED)
	  {
		  delay(1);
	  }

	configTime(timezone * 3600, dst, "pool.ntp.org", "time.nist.gov");
	delay(500);

	WiFi.disconnect();
}

void refreshRTC()
{
	time_t now;
	struct tm * timeinfo;
	time(&now);
	timeinfo = localtime(&now);

	timeHour = ( timeinfo->tm_hour );
	timeMin = ( timeinfo->tm_min );
}




void randomActions()
{
	randomTime1 = millis() + random(120000);
	randomTime2 = millis() + random(120000);
	randomTime3 = random(3600000);
}










CAYENNE_CONNECTED()
{
	online = true;
	digitalWrite(ledBluePin, 0);
}


CAYENNE_DISCONNECTED()
{
	online = false;
	digitalWrite(ledBluePin, 1);

	prevTime = millis();
	disco++;
}





//////////////////////////////////////////////////////////////////////

CAYENNE_IN(9)
{
	drive2 = getValue.asInt();

	if (drive2 == 1) // save sleep cycle status to eeprom to read after reboot
	  {
		  drive2FirstRun = true;
		  system_deep_sleep_set_option(2);

		  EEPROM.write(drive2eepromAddress, 1);
		  EEPROM.commit();
	  }
	else
	  {
		  EEPROM.write(drive2eepromAddress, 0);
		  EEPROM.commit();
	  }
}

CAYENNE_IN(10)
{
	drive1 = getValue.asInt();
}



CAYENNE_IN(11)
{
	sleepCycle = getValue.asInt();

	if (sleepCycle == 1) // save sleep cycle status to eeprom to read after reboot
	  {
		  system_deep_sleep_set_option(2);

		  EEPROM.write(eepromAddress, 1);
		  EEPROM.commit();
	  }
	else
	  {
		  EEPROM.write(eepromAddress, 0);
		  EEPROM.commit();
	  }
}

CAYENNE_IN(12)
{
	byte restartChip = getValue.asInt();

	delay(500);

	if (restartChip == 1)
	  {
		  Cayenne.virtualWrite(12, 0, "digital_sensor", "d");
		  Cayenne.loop();
		  delay(1500);
		  ESP.restart();
	  }
}

CAYENNE_IN(14)
{
	backwardButton = getValue.asInt();
}

CAYENNE_IN(15)
{
	forwardButton = getValue.asInt();
}

CAYENNE_IN(16)
{
	leftButton = getValue.asInt();
}

CAYENNE_IN(17)
{
	rightButton = getValue.asInt();
}

CAYENNE_IN(18)
{
	manualControl = getValue.asInt();

	if (manualControl == 0)
	  {
		  digitalWrite(ledRedPin, HIGH);
	  }
}

CAYENNE_IN(19)
{
	int powerSave = getValue.asInt();

	if (powerSave == 0)
	  {
		  system_update_cpu_freq(160);
		  system_phy_set_max_tpw(82);
		  wifi_set_sleep_type(NONE_SLEEP_T);
	  }

	else
	  {
		  system_update_cpu_freq(80);
		  system_phy_set_max_tpw(35);  //0-82  tx pwr
		  wifi_set_sleep_type(MODEM_SLEEP_T);
	  }
}
