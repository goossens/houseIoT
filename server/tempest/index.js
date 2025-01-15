//	houseIoT
//	Copyright (c) 2022-2025 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


const dgram = require("dgram");
const http = require("http");

// current weather observation
var wind_avg;
var wind_gust;
var wind_direction;

var pressure;
var temperature;
var humidity;

var illuminance;
var uv;

var rain = 0;

var lightning_distance;
var lightning_count = 0;

var battery;

// derived values
var wind_chill;
var heat_index;
var feels_like;

// convert weather observation to text
function toText() {
	return `\
wind_avg: ${wind_avg}
wind_gust: ${wind_gust}
wind_direction: ${wind_direction}
pressure: ${pressure}
temperature: ${temperature}
humidity: ${humidity}
illuminance: ${illuminance}
uv: ${uv}
rain: ${rain}
lightning_distance: ${lightning_distance}
lightning_count: ${lightning_count}
battery: ${battery}

wind_chill: ${wind_chill}
heat_index: ${heat_index}
feels_like: ${feels_like}`;
}

// convert weather observation to JSON
function toJSON() {
	return JSON.stringify({
		"wind_avg": wind_avg,
		"wind_gust": wind_gust,
		"wind_direction": wind_direction,

		"pressure": pressure,
		"temperature": temperature,
		"humidity": humidity,

		"illuminance": illuminance,
		"uv": uv,

		"rain": rain,

		"lightning_distance": lightning_distance,
		"lightning_count": lightning_count,

		"battery": battery,

		"wind_chill": wind_chill,
		"heat_index": heat_index,
		"feels_like": feels_like
	});
}

// convert weather observation to prometheus format
function toPrometheus() {
	return `\
# HELP wind_avg Average wind speed in miles per hour
# TYPE wind_avg gauge
wind_avg{id="tempest"}${wind_avg}

# HELP wind_gust Wind gust in miles per hour
# TYPE wind_gust gauge
wind_gust{id="tempest"}${wind_gust}

# HELP wind_direction Wind Direction in degrees
# TYPE wind_direction gauge
wind_direction{id="tempest"}${wind_direction}

# HELP pressure Air pressure in millibars
# TYPE pressure gauge
pressure{id="tempest"}${pressure}

# HELP temperature Temperature in degrees Fahrenheit
# TYPE temperature gauge
temperature{id="tempest"}${temperature}

# HELP humidity Relative humidity in percent
# TYPE humidity gauge
humidity{id="tempest"}${humidity}

# HELP illuminance Brightness in lux
# TYPE illuminance gauge
illuminance{id="tempest"}${illuminance}

# HELP uv UV index
# TYPE uv gauge
uv{id="tempest"}${uv}

# HELP rain Rainfall during last minute in inches
# TYPE rain gauge
rain{id="tempest"}${rain}

# HELP rain_total Total amount of rain
# TYPE rain_total counter
rain_total{id="tempest"}${rain}

# HELP lightning_distance Lighting distance in miles
# TYPE lightning_distance gauge
lightning_distance{id="tempest"}${lightning_distance}

# HELP lightning_count Lighting strike count during last minute
# TYPE lightning_count gauge
lightning_count{id="tempest"}${lightning_count}

# HELP lightning_count_total total lighting strikes
# TYPE lightning_count_total counter
lightning_count_total{id="tempest"}${lightning_count}

# HELP battery Battery strength in volts
# TYPE battery gauge
battery{id="tempest"}${battery}

# HELP wind_chill Wind chill in degrees Fahrenheit
# TYPE wind_chill gauge
wind_chill{id="tempest"}${wind_chill}

# HELP heat_index Heat index in degrees Fahrenheit
# TYPE heat_index gauge
heat_index{id="tempest"}${heat_index}

# HELP feels_like Feel-like temperature in degrees Fahrenheit
# TYPE feels_like gauge
feels_like{id="tempest"}${feels_like}`;
}

// start listening to weather events
const weather = dgram.createSocket("udp4");
weather.bind(50222);

weather.on("message", function(msg, rinfo) {
	msg = JSON.parse(msg.toString());

	if (msg.type == "obs_st") {
		// decode weather observation
		wind_avg = (msg.obs[0][2] *  2.2369).toFixed(2);
		wind_gust = (msg.obs[0][3] *  2.2369).toFixed(2);
		wind_direction = msg.obs[0][4];

		pressure = msg.obs[0][6];
		temperature = (msg.obs[0][7] * 9.0 / 5.0 + 32.0).toFixed(2);
		humidity = msg.obs[0][8];

		illuminance = msg.obs[0][9];
		uv = msg.obs[0][10];

		rain += (msg.obs[0][12] / 25.4).toFixed(4);

		lightning_distance = (msg.obs[0][14] *  0.6214).toFixed(2);
		lightning_count += msg.obs[0][15];

		battery = msg.obs[0][16];

		// calculate derived values
		if (temperature > 50 || wind_gust < 1) {
			wind_chill = temperature;

		} else {
			wind_chill = (35.74 + (0.6215 * temperature) + ((0.4275 * temperature - 35.75) * Math.pow(wind_gust * 1.2, 0.16))).toFixed(2);
		}

		if (temperature < 80 || humidity < 40) {
			heat_index = temperature;

		} else {
			heat_index = (
				-42.379 +
				2.04901523 * temperature +
				10.1433127 * humidity +
				-0.22475541 * temperature * humidity +
				-6.83783e-3 * Math.pow(temperature, 2) +
				-5.481717e-2 * Math.pow(humidity, 2) +
				1.22874e-3 * Math.pow(temperature, 2) * humidity +
				8.5282e-4 * temperature * Math.pow(humidity, 2) +
				-1.99e-6 * Math.pow(temperature, 2) * Math.pow(humidity, 2)).toFixed(2);
		}

		feels_like = (temperature < 65) ? wind_chill : heat_index;
	}
});

// start HTTP server
const server = http.createServer(function (req, res) {
	switch (req.url) {
		case "/":
			res.writeHead(200);
			res.end(toText());
			break;

		case "/json":
			res.setHeader("Content-Type", "application/json");
			res.writeHead(200);
			res.end(toJSON());
			break;

		case "/metrics":
			res.writeHead(200);
			res.end(toPrometheus());
			rain = 0;
			lightning_count = 0;
			break;

		default:
			res.writeHead(404);
			res.end(JSON.stringify({error:"Resource not found"}));
			break;
	}
});

server.listen(9999, "0.0.0.0");
