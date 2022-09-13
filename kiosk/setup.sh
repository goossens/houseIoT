#!/bin/bash

# ensure we are running as root
if [ "${EUID}" -ne 0 ]
then
	echo "$0: This script must be run as [root]"
	exit 1
fi

# update software
apt-get update
apt-get upgrade -y

# ensure boot process waits for network connection
raspi-config nonint do_boot_wait 0

# enable auto login
raspi-config nonint do_boot_behaviour B2

# disable overscan
raspi-config nonint do_overscan 1

# install X Windows
apt-get install -y --no-install-recommends xserver-xorg x11-xserver-utils xinit openbox

# install Chromium
apt-get install -y chromium-browser

# create startup script
cat >/etc/xdg/openbox/autostart << "END"
#!/bin/sh
xset -dpms     # disable DPMS (Energy Star) features.
xset s off     # disable screen saver
xset s noblank # don't blank the video device

setxkbmap -option terminate:ctrl_alt_bksp

sed -i 's/"exited_cleanly":false/"exited_cleanly":true/' ~/.config/chromium/'Local State'
sed -i 's/"exited_cleanly":false/"exited_cleanly":true/; s/"exit_type":"[^"]\+"/"exit_type":"Normal"/' ~/.config/chromium/Default/Preferences

chromium-browser \
	--start-fullscreen \
	--kiosk \
	--incognito \
	--noerrdialogs \
	--disable-translate \
	--no-first-run \
	--fast \
	--fast-start \
	--disable-infobars \
	--disable-features=TranslateUI \
	--disk-cache-dir=/dev/null  \
	--password-store=basic \
	--disable-pinch \
	--overscroll-history-navigation=disabled \
	--disable-features=TouchpadOverscrollHistoryNavigation \
	--enable-features=OverlayScrollbar \
	http://iot.local
END

# Add startup script to user's boot script
cat > /home/${SUDO_USER}/.bash_profile << "END"
[[ -z $DISPLAY && $XDG_VTNR -eq 1 ]] && startx -- -nocursor
END

chmod 555 /home/${SUDO_USER}/.bash_profile
