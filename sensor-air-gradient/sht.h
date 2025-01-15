//	houseIoT
//	Copyright (c) 2022-2025 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.
//
//	Particulate Matter sensor class
//


#include <Ticker.h>
#include <Wire.h>


//
//	Constants
//

#define SHT_ADDRESS 0x44


//
//	Temperature and humidity senso class
//

class ShtSensor {
public:
	// initialize sensor
	void init() {
		// initialize library
		Wire.begin();

		// set high repeatability at 10 HZ
		writeCommand(0x2737);
		delay(100);

		// get initial data
		getData();

		// now get the data every 10 seconds
		readTimer.attach_scheduled(10, [this] {
			getData();
		});
	}

	float getTemperature() {
		return temperature;
	}

	float getHumidity() {
		return humidity;
	}

private:
	// get data from sensor
	void getData() {
		// request data
		if (!writeCommand(0xE000)) {
			uint8_t buffer[6];
			Wire.requestFrom(SHT_ADDRESS, 6);
			Wire.readBytes(buffer, 6);

			// read raw values
			int rawTemp = getValue(buffer);
			int rawHum = getValue(buffer + 3);

			// convert values if they are valid
			if (rawTemp > 0) {
				temperature = 175.0f * (float) rawTemp / 65535.0f - 45.0f;
			}

			if (rawHum > 0) {
				humidity =  100.0f * rawHum / 65535.0f;
			}

		} else {
			Serial.println("Can't write to SHT sensor");
		}
	}

	// send command to sensor
	int writeCommand(int command) {
		Wire.beginTransmission(SHT_ADDRESS);
		Wire.write(command >> 8);
		Wire.write(command & 0xFF);
		return -10 * Wire.endTransmission();
	}

	// read value from sensor
	int getValue(uint8_t* data) {
		if (crc(data, 2) == data[2]) {
			return (data[0] << 8) | data[1];

		} else {
			return -1;
		}
	}

	// calculate checksum
	uint8_t crc(uint8_t* data, int size) {
		uint8_t crc = 0xFF;

		for (int i = 0; i < size; i++) {
			crc ^= data[i];

			for (int bit = 8; bit > 0; --bit) {
				crc = (crc & 0x80) ? (crc << 1) ^ 0x131 : crc << 1;
			}
		}

		return crc;
	}

	// timer to read the sensor
	Ticker readTimer;

	// sensor readings
	float temperature = 0.0;
	float humidity = 0.0;
};
