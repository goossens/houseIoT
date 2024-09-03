//	houseIoT
//	Copyright (c) 2022-2024 Johan A. Goossens. All rights reserved.
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
//	onWifiIP
//

void onWifiIP(WiFiEvent_t event, WiFiEventInfo_t info) {
	// show our IP address
	Serial.println("WIFI is connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("RRSI: ");
	Serial.print(WiFi.RSSI());
	Serial.println("dB");

	// start the HTTP server
	Serial.println("Starting web server...");
	server.begin();
}


//
//	onWifiDisconnect
//

void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
	// stop web server
	server.close();

	// try to reconnect to wifi
	Serial.print("WiFi lost connection. Reason: ");
	Serial.println(info.wifi_sta_disconnected.reason);
	Serial.println("Reconnecting...");
	WiFi.begin(ssid, password);
}


//
//	setup
//

void setup() {
	// setup console
	Serial.begin(115200);

	while (!Serial) {
		sleep(1000);
	}

	Serial.println();

	// setup wifi
	WiFi.mode(WIFI_STA);
	WiFi.config(ip, gateway, subnet);
	WiFi.setHostname(hostname);

	WiFi.onEvent(onWifiIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
	WiFi.onEvent(onWifiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
	WiFi.begin(ssid, password);

	// setup HTTP request handlers
	server.on("/", HTTP_GET, []() {
		String info = "";
		info += "Hostname: " + String(hostname) + "\n";
		info += "IP address: " + WiFi.localIP().toString() + "\n";
		info += "Wifi RRSI: " + String(WiFi.RSSI()) + "dB\n";
		String id = String(sht4.readSerial(), HEX);
		id.toUpperCase();
		info += "Sensor ID: 0x" +  id + "\n";
		info += "Temperature: " + String(temperature) + "\n";
		info += "Humidity: " + String(humidity) + "\n";
		server.send(200, "text/plain", info);
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

		// do another scan in 10 seconds
		nextScan = now + 10000;
	}
}
