#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Event.h>
#include "Timer.h"
#include <ESP8266WiFi.h>

#define S Serial

//========     SETUP DHT Sensor    ========//
#define DHTPIN D2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//========       SETUP TIMER       ========//
Timer sensorTimer;
const int sensorIntervall = 5; // in seconds
Timer wifiTimer;
const int wifiIntervall = 5; // in seconds

//========   SETUP Messvariablen   ========//
const int anzMess = 5;
float messHumArray[anzMess];
float messTempArray[anzMess];
int userhum = 70; //default value, can be changed on app

//========   SETUP Wifi            ========//
const char* ssid = "BumeSpot";
const char* password = "tschussi95";
const char* host = "http://web304.126.hosttech.eu";




void setup()
{
	S.begin(9600);
	S.println("Module started!");
	dht.begin();
	S.println("Sensors initialized");

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
}

void sendValue() {
	float hum;
	float temp;

	for (int i = 0; i < anzMess; i++) {
		hum += messHumArray[i];
		temp += messTempArray[i];
	}
	hum = hum / anzMess;
	temp = temp / anzMess;

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
	url += hum;
	url += ",";
	url += temp;
	url += ",";
	url += 0;
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

void loop()
{
	sensorTimer.update();
	wifiTimer.update();
}