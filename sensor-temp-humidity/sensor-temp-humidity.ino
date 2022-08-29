//	houseIoT
//	Copyright (c) 2022 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#include <Adafruit_SHT4x.h>

#include "settings.h"


//
//	Globals
//

WebServer server(80);

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
unsigned long nextScan = 0;

float temperature;
float humidity;


//
//	setup
//

void setup() {
	// setup console
	Serial.begin(115200);

	while (!Serial) {
		sleep(1000);
	}

	// setup wifi
	Serial.println("Connecting to WiFi...");
	WiFi.config(ip, gateway, subnet);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Waiting for WiFi connection...");
	}

	// setup HTTP request handlers
	server.on("/", HTTP_GET, []() {
		server.send(200, "text/plain", "Hello world");
	});

	server.on("/metrics", HTTP_GET, []() {
		String metrics = "";
		String id = "{id=\"" + String(hostname) + "\"}";

		metrics += "# HELP temperature Temperature in degrees Fahrenheit\n";
		metrics += "# TYPE temperature gauge\n";
		metrics += "temperature" + id + String(temperature) + "\n";

		metrics += "# HELP humidity Relative humidity in percent\n";
		metrics += "# TYPE humidity gauge\n";
		metrics += "humidity" + id + String(humidity) + "\n";

		server.send(200, "text/plain", metrics);
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not found");
	});

	// start the HTTP server
	Serial.println("Starting web server...");
	server.begin();

	// connect to temerature/humidity sensor
	Serial.println("Connecting to sensors...");

	if (sht4.begin()) {
		Serial.print("Sensor ID: 0x");
		Serial.println(sht4.readSerial(), HEX);

		sht4.setPrecision(SHT4X_HIGH_PRECISION);
		sht4.setHeater(SHT4X_NO_HEATER);

	} else {
		Serial.println("Couldn't find SHT4x");
	}
}


//
//	loop
//

void loop() {
	// handle next HTTP request (if required)
	server.handleClient();

	// scan sensors (if required)
	unsigned long now = millis();

	if (now > nextScan) {
		// get values
		sensors_event_t hum, temp;
		sht4.getEvent(&hum, &temp);
		temperature = temp.temperature * 9.0 / 5.0 + 32.0;
		humidity = hum.relative_humidity;

		// do another scan in 5 seconds
		nextScan = now + 5000;
	}
}
