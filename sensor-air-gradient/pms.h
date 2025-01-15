//	houseIoT
//	Copyright (c) 2022-2025 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#include <SoftwareSerial.h>
#include <Ticker.h>


//
//	Particulate matter sensor class
//

class PmsSensor {
public:
	// initialize sensor
	void init() {
		// setup serial connection to sensor
		serial = new SoftwareSerial(D5, D6);
		serial->begin(9600);
		delay(100);

		// put sensor in passive mode
		passive();
		sleep();
		pm2 = 0;

		// read values every 3 minutes
		wakeupTimer.attach_scheduled(180, [this] {
			wakeup();

			// set timer to wait for reading
			readTimer.once_scheduled(30, [this] {
				getData();
			});
		});
	}

	// get current particulate matter level
	int getPM2() {
		return pm2;
	}

private:
	// put sensor in passive mode
	void passive() {
		uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
		Serial.println("PMS5003 passive");
		serial->write(command, sizeof(command));
		serial->flush();
	}

	// send wakeup command
	void wakeup() {
		// send wakeup command
		uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
		Serial.println("PMS5003 wakeup");
		serial->write(command, sizeof(command));
		serial->flush();
	}

	// send sleep command
	void sleep() {
		uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
		Serial.println("PMS5003 sleep");
		serial->write(command, sizeof(command));
		serial->flush();
	}

	// send request command
	void request() {
		uint8_t command[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
		Serial.println("PMS5003 request");
		serial->write(command, sizeof(command));
		serial->flush();
	}

	// get the particulate matter reading
	void getData() {
		// request the data
		request();

		// read the data
		uint32_t start = millis();
		uint8_t data[12];
		bool done = false;
		int index = 0;
		int frameSize = 0;
		int cs1 = 0;
		int cs2 = 0;

		// timeout after 3 seconds
		while ((millis() - start) <= 3000 && !done) {
			if (serial->available()) {
				uint8_t ch = serial->read();

				switch (index) {
					case 0:
						if (ch == 0x42) {
							cs1 = ch;
							index++;
						}

						break;

					case 1:
						if (ch == 0x4D) {
							cs1 += ch;
							index++;
						}

						break;

					case 2:
						cs1 += ch;
						frameSize = ch << 8;
						index++;
						break;

					case 3:
						frameSize |= ch;

						if (frameSize == 2 * 13 + 2) {
							cs1 += ch;
							index++;

						} else {
							Serial.print("Invalid frame size for PMS5003 sensor (");
							Serial.print(frameSize);
							Serial.println(")");
							index = 0;
						}

						break;

					default:
						if (index == frameSize + 2) {
							cs2 = ch << 8;
							index++;

						} else if (index == frameSize + 3) {
							cs2 |= ch;

							if (cs1 == cs2) {
								pm2 = makeWord(data[8], data[9]);
								Serial.print("PMS5003 = ");
								Serial.println(pm2);

								done = true;

							} else {
								Serial.print("Incorrect checksum from PMS5003 sensor (");
								Serial.print(cs1);
								Serial.print(" != ");
								Serial.print(cs2);
								Serial.println(")");

								index = 0;
							}

						} else {
							int dataIndex = index - 4;

							if (dataIndex < sizeof(data)) {
								data[dataIndex] = ch;
							}

							cs1 += ch;
							index++;
						}
				}
			}
		}

		if (!done) {
			Serial.print("PMS5003 timed out (");
			Serial.print(index);
			Serial.println(")");
		}
		// put sensor back to sleep
		sleep();
	}

	// serial connection
	SoftwareSerial* serial;

	// timers
	Ticker wakeupTimer;
	Ticker readTimer;

	// current particulate matter level
	int pm2;
};
