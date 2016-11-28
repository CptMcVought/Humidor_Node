#include "Timer.h"
#include "DHT.h"

#define S Serial

//========     SETUP DHT Sensor    ========//
#define DHTPIN D2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//========       SETUP TIMER       ========//
Timer sensorTimer;
const int sensorIntervall = 5;
Timer wifiTimer;
const int wifiIntervall = 5;

//========   SETUP Messvariablen   ========//
const int anzMess = 5;
float messHumArray[anzMess];
float messTempArray[anzMess];




void setup()
{
	S.begin(9600);
	S.println("Module started!");
	dht.begin();
	S.println("Sensors initialized");

	sensorTimer.every(sensorIntervall * 1000, readSensor);
	wifiTimer.every(wifiIntervall * 1000, sendValue);
}

void readSensor() {
	for (int i = 0; i < anzMess - 1; i++) {
		messHumArray[i + 1] = messHumArray[i];
		messTempArray[i + 1] = messTempArray[i];
	}
	messHumArray[0] = dht.readHumidity();
	messTempArray[0] = dht.readTemperature();
}

void sendValue() {

}

void loop()
{
	sensorTimer.update();
	wifiTimer.update();
}