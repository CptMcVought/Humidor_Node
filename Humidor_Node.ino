#include "DHT.h"
#include <Event.h>
#include "Timer.h"
#include <ESP8266WiFi.h>

#define S Serial

//========     SETUP DHT Sensor    ========//
#define DHTPIN D2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//========    SETUP Pins     ========//
#define USONICPIN D0
#define FANPIN D1
#define WATERPIN D3
#define LIGHTPIN D6
#define IRPIN D5
bool IRPreState = 0;

//========       SETUP TIMER       ========//
Timer sensorTimer;
const int sensorIntervall = 3; // in seconds
Timer wifiTimer;
const int wifiIntervall = 20; // in seconds
Timer lightTimer;
const int lightSmoothness = 2000; // in milli seconds
const int lightOffTime = 5; //in seconds
bool lightChangeable = true;
							  
//========   SETUP Messvariablen   ========//
const float upHumTreshold = 0.8;
const float downHumTreshold = 0.1;
const int anzMess = 5;
float messHumArray[anzMess];
float messTempArray[anzMess];
float hum; // durchschnitt
float temp;
int userhum = 70; //default value, can be changed on app
bool waterState = 0;

//========   SETUP Wifi            ========//
const char* ssid = "XT1092";
const char* password = "peterpeter";
const char* host = "http://web304.126.hosttech.eu";




void setup()
{
	S.begin(9600);
	S.println("Module started!");
	dht.begin();
	S.println("Sensors initialized");

	pinMode(WATERPIN, INPUT);
	pinMode(USONICPIN, OUTPUT);
	pinMode(FANPIN, OUTPUT);
	pinMode(IRPIN, INPUT);
	pinMode(LIGHTPIN, OUTPUT);

	analogWrite(LIGHTPIN, 0);

	connectWifi();

	sensorTimer.every(sensorIntervall * 1000, readSensor);
	wifiTimer.every(wifiIntervall * 1000, sendValue);
}

void connectWifi() {
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	int i = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		i++;
		if (i > 15) {
			i = 0;
			WiFi.begin(ssid, password);
		}
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void readSensor() {
	for (int i = 0; i < anzMess - 1; i++) {
		messHumArray[i + 1] = messHumArray[i];
		messTempArray[i + 1] = messTempArray[i];
	}
	messHumArray[0] = dht.readHumidity();
	messTempArray[0] = dht.readTemperature();
	waterState = digitalRead(WATERPIN);

	evaluateSituation();
}

void evaluateSituation() {
	hum = 0;
	temp = 0;
	for (int i = 0; i < anzMess; i++) {
		hum += messHumArray[i];
		temp += messTempArray[i];
	}
	hum = hum / anzMess;
	temp = temp / anzMess;

	bool USONICState = 0;
	bool FANState = 0;

	if (hum < userhum & waterState ) {
		S.println("ich muen heize!!");
		USONICState = 1;
		FANState = 1;
	}
	else if (hum > userhum + upHumTreshold) {
		S.println("ich bin z fueecht");
		USONICState = 0;
		FANState = 1;
	}
	else {
		USONICState = 0;
		FANState = 0;
	}

	digitalWrite(USONICPIN, USONICState);
	digitalWrite(FANPIN, FANState);

}

void sendValue() {
	float humTemp = (int(hum * 10 + 0.5)) / 10.0;
	float tempTemp = (int(temp * 10 + 0.5)) / 10.0;
	String humTempString = String(humTemp).substring(0, 4);
	String tempTempString = String(tempTemp).substring(0, 4);
	Serial.print("connecting to ");
	Serial.println(host);

	// Use WiFiClient class to create TCP connections
	WiFiClient client;
	const int httpPort = 80;
	if (!client.connect(host, httpPort)) {
		Serial.println("connection failed");
		return;
	}

	// We now create a URI for the request
	String url = "http://web304.126.hosttech.eu/arduino/humi/send.php?android=";
	url += humTempString;
	url += ",";
	url += tempTempString;
	url += ",";
	url += waterState;
	url += ",";

	Serial.print("Requesting URL: ");
	Serial.println(url);

	// This will send the request to the server
	client.print(String("GET ") + url + " HTTP/1.1\r\n" +
		"Host: " + host + "\r\n" +
		"Connection: close\r\n\r\n");
	unsigned long timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 5000) {
			Serial.println(">>> Client Timeout !");
			client.stop();
			return;
		}
	}

	// Read all the lines of the reply from server and print them to Serial
	String line = "";
	while (client.available()) {
		line = client.readStringUntil('\r');
		//S.println(line);
		int pos = findInString("user", line);
		if (pos != -1) {
			String user = line.substring(pos+7, pos + 9);
			userhum = user.toInt();
		}
	}

	Serial.println();
	Serial.println("closing connection");
}

int findInString(String needle, String haystack) {
	int foundpos = -1;
	if (haystack.length() > needle.length()) {
		for (int i = 0; i < haystack.length() - needle.length(); i++) {
			if (haystack.substring(i, needle.length() + i) == needle) {
				foundpos = i;
			}
		}
	}
	return foundpos;
}

void smoothLightIn() {
	int del = lightSmoothness / 255;
	for (int i = 0; i < 255; i++) {
		analogWrite(LIGHTPIN, i);
		delay(del);
	}
}

void smoothLightOut() {
	int del = lightSmoothness / 255;
	for (int i = 255; i >= 0; i--) {
		analogWrite(LIGHTPIN, i);
		delay(del);
	}
	lightChangeable = true;
}

void loop()
{
// --- UPDATE TIMERS --- //
	sensorTimer.update();
	wifiTimer.update();
	lightTimer.update();
// --- END UPDATE TIMER --- //

// --- CHECK LIGHTSITUATION --- //
	if (digitalRead(IRPIN) & !IRPreState & lightChangeable) {
		smoothLightIn();
		lightTimer.after(lightOffTime*1000, smoothLightOut);
		lightChangeable = false;
	}
	IRPreState = digitalRead(IRPIN);
// --- END LIGHTSITUATION --- //

	delay(100);
}