//	houseIoT
//	Copyright (c) 2022 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <U8g2lib.h>

#include "settings.h"


//
//	Globals
//

ESP8266WebServer server(80);
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R2, U8X8_PIN_NONE);

unsigned long nextScan = 0;

AirGradient ag = AirGradient();
int pm02;
int co2;
float temperature;
float humidity;


//
//	showText
//

void showText(String line1, String line2, String line3) {
	display.firstPage();

	do {
		display.setFont(u8g2_font_t0_16_tf);
		display.drawStr(1, 10, String(line1).c_str());
		display.drawStr(1, 30, String(line2).c_str());
		display.drawStr(1, 50, String(line3).c_str());
	} while (display.nextPage());
}


//
//	setup
//

void setup() {
	// initialize display
	display.begin();
	showText("Warming up the", "sensors...", "");

	// enable sensors
	ag.PMS_Init();
	ag.CO2_Init();
	ag.TMP_RH_Init(0x44);

	// setup wifi
	WiFi.mode(WIFI_STA);
	WiFi.config(ip, gateway, subnet);
	WiFi.setHostname(hostname);

	gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
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
	});

	disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
		// stop web server
		server.close();

		// try to reconnect to wifi
		Serial.print("WiFi lost connection. Reason: ");
		Serial.println(event.reason);
		Serial.println("Reconnecting...");
		WiFi.begin(ssid, password);
	});

	WiFi.begin(ssid, password);

	// setup HTTP request handlers
	server.on("/", []() {
		String info = "";
		info += "Hostname: " + String(hostname) + "\n";
		info += "IP address: " + WiFi.localIP().toString() + "\n";
		info += "Wifi RRSI: " + String(WiFi.RSSI()) + "dB\n";
		info += "Temperature: " + String(temperature) + "\n";
		info += "Humidity: " + String(humidity) + "\n";
		info += "PM02: " + String(pm02) + "\n";
		info += "CO2: " + String(co2) + "\n";
		server.send(200, "text/plain", info);
	});

	server.on("/metrics", []() {
		String metrics = "";
		String id = "{id=\"" + String(hostname) + "\"}";

		metrics += "# HELP pm02 Particulate Matter PM2.5 value in mcg/m3\n";
		metrics += "# TYPE pm02 gauge\n";
		metrics += "pm02" + id + String(pm02) + "\n";

		metrics += "# HELP co2 CO2 value in ppm\n";
		metrics += "# TYPE co2 gauge\n";
		metrics += "co2" + id + String(co2) + "\n";

		metrics += "# HELP temperature Temperature in degrees Fahrenheit\n";
		metrics += "# TYPE temperature gauge\n";
		metrics += "temperature" + id + String(temperature) + "\n";

		metrics += "# HELP humidity Relative humidity in percent\n";
		metrics += "# TYPE humidity gauge\n";
		metrics += "humidity" + id + String(humidity) + "\n";
		metrics += "\n";

		server.send(200, "text/plain", metrics);
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not found");
	});
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
		pm02 = ag.getPM2_Raw();
		co2 = ag.getCO2_Raw();

		TMP_RH stat = ag.periodicFetchData();
		temperature = stat.t * 9.0 / 5.0 + 32.0;
		humidity = stat.rh;

		// update display
		String line1 = "T: " + String(temperature, 0) + "F RH: " + String(humidity, 0) + "%";
		String line2 = "PM02: " + String(pm02);
		String line3 = "CO2: " + String(co2);
		showText(line1, line2, line3);

		// do another scan in 5 seconds
		nextScan = now + 5000;
	}
}
