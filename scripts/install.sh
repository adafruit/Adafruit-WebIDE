#!/usr/bin/env bash

# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh
# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/test/scripts/install.sh | sudo sh
# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/offline/scripts/install.sh | sudo sh -s - --offline

#tar -zcvf editor.tar.gz * --exclude .git --exclude .gitignore
#tar -zcvf editor-update.tar.gz * --exclude .git --exclude .gitignore
#scp pi@raspberrypi.local:/home/pi/Adafruit-WebIDE/editor.tar.gz editor-0.2.4.tar.gz
#scp pi@raspberrypi.local:/home/pi/Adafruit-WebIDE-Update/editor-update.tar.gz editor-0.2.4-update.tar.gz
#sudo -u webide -g webide node server

#http://stackoverflow.com/a/6946864/629189
# translate long options to short
OFFLINE=false
GITHUB=false

for arg
do
    delim=""
    case "$arg" in
       --offline) args="${args}-o ";;
       --github) args="${args}-g ";;
       --help) args="${args}-h ";;
       #--config) args="${args}-c ";;
       # pass through anything else
       *) [[ "${arg:0:1}" == "-" ]] || delim="\""
           args="${args}${delim}${arg}${delim} ";;
    esac
done
# reset the translated args
eval set -- $args
# now we can process with getopt
while getopts ":hogvc:" opt; do
    case $opt in
        h)  usage ;;
        o)  OFFLINE=true ;;
        g)  GITHUB=true ;;
        #c)  source $OPTARG ;;
        #\?) usage ;;
        :)
        echo "option -$OPTARG requires an argument"
        usage
        ;;
    esac
done

echo "$OFFLINE"

set -e
WEBIDE_ROOT="/usr/share/adafruit/webide"

#needed for SSH key and config access at this point.
WEBIDE_HOME="/home/webide"

mkdir -p "$WEBIDE_ROOT"
mkdir -p "$WEBIDE_HOME/tmp"
cd "$WEBIDE_ROOT"

echo "**** Downloading the latest version of the WebIDE ****"
curl -L https://github.com/adafruit/Adafruit-WebIDE/archive/0.8.0.tar.gz | tar xzf - --strip-components=1

echo "**** Installing required libraries ****"
echo "**** (nodejs-legacy npm git i2c-tools python-smbus ntp libkrb5-dev) ****"
curl -sL https://deb.nodesource.com/setup_8.x | sudo -E bash -
apt-get update
apt-get install nodejs git i2c-tools python-smbus ntp libkrb5-dev -y
npm install -g npm

echo "**** Create webide user and group ****"
groupadd webide || true
useradd -g webide webide || true
usermod -a -G i2c,sudo webide || true
chsh -s /bin/bash webide

echo "**** Adding webide user to sudoers ****"
if [ -f "/etc/sudoers.tmp" ]; then
    rm /etc/sudoers.tmp
fi
cp /etc/sudoers /etc/sudoers.tmp
echo "webide ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers.tmp
visudo -c -f /etc/sudoers.tmp
if [ "$?" -eq "0" ]; then
    cp /etc/sudoers.tmp /etc/sudoers
fi
rm /etc/sudoers.tmp

chown -R webide:webide "$WEBIDE_HOME"
chown -R webide:webide "$WEBIDE_ROOT"
chmod 775 "$WEBIDE_ROOT"

echo "**** Installing webide dependencies ****"
sudo -u webide npm install

echo "**** Adding default .bashrc file for webide user ****"
cp "$WEBIDE_ROOT/scripts/.bashrc" "$WEBIDE_HOME"

echo "Attempting to force reload date and time from ntp server"
/etc/init.d/ntp force-reload

# echo "**** Installing the WebIDE as a service ****"
# echo "**** (to uninstall service, execute: 'sudo update-rc.d -f adafruit-webide.sh remove') ****"
cp "$WEBIDE_ROOT/scripts/adafruit-webide.service" "/etc/systemd/system/adafruit-webide.service"
systemctl enable adafruit-webide.service
systemctl start adafruit-webide.service


echo "**** Starting the server...(please wait) ****"
sleep 20s

echo "**** The Adafruit WebIDE is installed and running! ****"
echo "**** Commands: sudo systemctl {start,stop,restart} adafruit-webide ****"
echo "**** Navigate to http://$(hostname).local:8080 to use the WebIDE"
#echo "**** To run the editor: ****"
#echo "**** cd ~/Adafruit/WebIDE ****"
#echo "**** node webide ****"
