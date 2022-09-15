#!/bin/bash
#
#	houseIoT
#	Copyright (c) 2022 Johan A. Goossens. All rights reserved.
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

# add tools
apt-get install -y vim dialog

# install mDNS
#
apt-get install -y avahi-daemon

# install packages to allow apt to use a repository over HTTPS
apt-get install -y ca-certificates curl gnupg lsb-release
mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# setup docker repository
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# install docker engine, containerd, and docker compose
apt-get update
apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# add docker permissions
usermod -aG docker ${SUDO_USER}

# automatically restart server on power loss
cat >/etc/systemd/system/reboot.service <<END
[Unit]
Description=Reboot after power failure

[Service]
Type=oneshot
ExecStart=sudo setpci -s 0:1f.0 0xa4.b=0

[Install]
WantedBy=sysinit.target
END

systemctl enable --now reboot
