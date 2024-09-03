#!/bin/bash
#
#	houseIoT
#	Copyright (c) 2022-2024 Johan A. Goossens. All rights reserved.
#
#	This work is licensed under the terms of the MIT license.
#	For a copy, see <https://opensource.org/licenses/MIT>.

# ensure we are running as root
if [ "${EUID}" -ne 0 ]
then
	echo "$0: This script must be run as [root]"
	exit 1
fi


# update software
apt-get update
apt-get upgrade -y


# Add Docker's official GPG key:
apt-get update
apt-get install ca-certificates curl
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc
chmod a+r /etc/apt/keyrings/docker.asc

# setup docker repository
echo \
	"deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian \
	$(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
	sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

apt-get update

# install docker engine, containerd, and docker compose
apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# add docker permissions
usermod -aG docker ${USER}
