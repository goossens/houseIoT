//	houseIoT
//	Copyright (c) 2022-2025 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#include <SoftwareSerial.h>


//
//	Constants
//

#define MODBUS_ANY_ADDRESS                  0XFE	// S8 uses any address
#define MODBUS_FUNC_READ_HOLDING_REGISTERS  0X03	// Read holding registers (HR)
#define MODBUS_FUNC_READ_INPUT_REGISTERS    0x04	// Read input registers (IR)
#define MODBUS_FUNC_WRITE_SINGLE_REGISTER   0x06	// Write single register (SR)

#define MODBUS_IR1							0x00	// MeterStatus
#define MODBUS_IR2							0x01	// AlarmStatus
#define MODBUS_IR3							0x02	// OutputStatus
#define MODBUS_IR4							0x03	// Space CO2
#define MODBUS_IR22							0x15	// PWM Output
#define MODBUS_IR26							0x19	// Sensor Type ID High
#define MODBUS_IR27							0x1A	// Sensor Type ID Low
#define MODBUS_IR28							0x1B	// Memory Map version
#define MODBUS_IR29							0x1C	// firmware version
#define MODBUS_IR30							0x1D	// Sensor ID High
#define MODBUS_IR31							0x1E	// Sensor ID Low

#define MAX_RESULT_SIZE 20


//
//	CO2 sensor class
//

class S8Sensor {
public:
	// initialize sensor
	void init() {
		// setup serial connection to sensor
		serial = new SoftwareSerial(D4, D3);
		serial->begin(9600);
	}

	// get sensor ID
	int32_t getID() {
		int32_t sensorID = 0;

		// get sensor ID
		if (command(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR30, 1, 7)) {
			sensorID |= ((int32_t) result[3]) << 24;
			sensorID |= ((int32_t) result[4]) << 16;

			if (command(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR31, 1, 7)) {
				sensorID |= ((int32_t) result[3]) << 8;
				sensorID |= (int32_t) result[4];
			}
		}

		return sensorID;
	}

	// get sensor firmware version
	const char* getFirmwareVersion() {
		static char version[10];

		// ask sensor for firmware version
		if (command(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR29, 1, 7)) {
			snprintf(version, 10, "%0u.%0u", result[3], result[4]);

		} else {
			strcpy(version, "Unknown");
		}

		return version;
	}

	// get the current C02 level
	int16_t getCO2() {
		static int16_t co2 = 0;

		// ask sensor for current reading
		if (command(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR4, 1, 7)) {
			co2 = (result[3] << 8) | result[4];
		}

		return co2;
	}

private:
	// send command to sensor and wait for response
	bool command(uint8_t function, uint16_t reg, uint16_t value, uint8_t size) {
		// build command
		uint8_t command[8];
		command[0] = MODBUS_ANY_ADDRESS;
        command[1] = function;
        command[2] = (reg >> 8) & 0xFF;
        command[3] = reg & 0xFF;
        command[4] = (value >> 8) & 0xFF;
        command[5] = value & 0xFF;
        uint16_t crc16 = crc(command, 6);
        command[6] = crc16 & 0xFF;
        command[7] = (crc16 >> 8) & 0xFF;

		// send command to sensor
		serial->write(command, 8);
		serial->flush();
		delay(100);

		// read results
		memset(result, 0, MAX_RESULT_SIZE);
		uint32_t start = millis();
		int read = 0;

		// timeout after 3 seconds
		while ((millis() - start) <= 3000 && read < size) {
			// read results
			read += serial->readBytes(result + read, size - read);
		}

		// see if result is complete
		if (read < size) {
			Serial.println("Incomplete reading from S8 sensor");
			return false;
		}

		// validate answer
		if (result[0] != MODBUS_ANY_ADDRESS || result[1] != function || result[2] != size - 5) {
			Serial.println("Unexpected response from S8 sensor");
			return false;
		}

		// validate checksum
		crc16 = crc(result, size - 2);

		if (result[size - 2] != (crc16 & 0xFF) || result[size - 1] != ((crc16 >> 8) & 0xFF)) {
			Serial.println("Incorrect checksum from S8 sensor");
			return false;
		}

		// we have a valid result
	   return true;
	}

	// calculate modbus crc value
	uint16_t crc(uint8_t *message, uint16_t len) {
		// crc values for msb
		static uint8_t msbCRC[] = {
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
			0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
			0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
			0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
			0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
			0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
			0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
			0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
			0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
			0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
			0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
			0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
			0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
			0x40
		};

		// crc values for lsb
		static uint8_t lsbCRC[] = {
			0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
			0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
			0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
			0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
			0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
			0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
			0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
			0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
			0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
			0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
			0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
			0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
			0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
			0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
			0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
			0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
			0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
			0x40
		};

		uint8_t msb = 0xFF;
		uint8_t lsb = 0xFF;

		while (len--) {
			uint16_t index = lsb ^ *message++;
			lsb = msb ^ msbCRC[index];
			msb = lsbCRC[index];
		}

		return (msb << 8 | lsb);
	}

	// serial connection
	SoftwareSerial* serial;

	// sensor query result
	uint8_t result[MAX_RESULT_SIZE];
};
