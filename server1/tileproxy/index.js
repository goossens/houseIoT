//	houseIoT
//	Copyright (c) 2022-2025 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.

const express = require("express");
const path = require("path");
const fs = require("fs");

const cache = path.join(__dirname, "cache");
const headers = {"User-Agent": "ObjectTalk/0.4"};
const app = express();

app.get("/*path", async function(req, res) {
	const target = path.join(cache, req.path.slice(1));

	if (fs.existsSync(target)) {
		res.sendFile(target);

	} else {
		const url = "https://tile.openstreetmap.org" + req.path;
		console.log("Downloading file:", url, "to", target);

		try {
			const response = await fetch(url, {method: "GET", headers: headers});

			if (response.ok) {
				fs.mkdirSync(path.dirname(target), {"recursive": true});
				const buffer = Buffer.from(await response.arrayBuffer());
				fs.writeFileSync(target, buffer);
				res.sendFile(target);

			} else {
				res.status(response.status).send(response.statusText);
			}

		} catch(error) {
			console.log(error);
			res.status(500).send({error: error});
		}

		console.log("Downloaded file:", url, "to", target);
	}
});

app.listen(8010, function() {
	console.log("Proxy server listening on port 8010");
});
