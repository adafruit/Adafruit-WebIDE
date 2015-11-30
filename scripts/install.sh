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

#NODE_PATH="/usr/local/lib/node"

#if [ ! -d "$NODE_PATH" ]; then
#  mkdir -p "$NODE_PATH"
#    # Control will enter here if $DIRECTORY doesn't exist.
#fi

mkdir -p "$WEBIDE_ROOT"
mkdir -p "$WEBIDE_HOME"
cd "$WEBIDE_ROOT"

echo "**** Downloading the latest version of the WebIDE ****"
curl -L http://adafruit-download.s3.amazonaws.com/webide-0.3.11.tar.gz | tar xzf -

echo "**** Installing required libraries ****"
echo "**** (redis-server git restartd libcap2-bin avahi-daemon i2c-tools python-smbus) ****"
apt-get update
apt-get install nodejs nodejs-legacy redis-server git restartd libcap2-bin avahi-daemon i2c-tools python-smbus ntp -y

echo "**** Create webide user and group ****"
groupadd webide || true
useradd -g webide webide || true
usermod -a -G i2c,sudo webide || true

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

echo "**** Adding default .bashrc file for webide user ****"
cp "$WEBIDE_ROOT/scripts/.bashrc" "$WEBIDE_HOME"

echo "**** Installing the WebIDE as a service ****"
echo "**** (to uninstall service, execute: 'sudo update-rc.d -f adafruit-webide.sh remove') ****"
cp "$WEBIDE_ROOT/scripts/adafruit-webide.sh" "/etc/init.d"
cd /etc/init.d
chmod 755 adafruit-webide.sh

NODE_PATH=""
ARCH=$(dpkg --print-architecture)
if [ $ARCH = armhf ]; then
  NODE_PATH="\/usr\/share\/adafruit\/webide\/bin\/node_hf\/node"
  chmod +x "$WEBIDE_ROOT/bin/node_hf/node"
else
  NODE_PATH="\/usr\/share\/adafruit\/webide\/bin\/node_sf\/node"
  chmod +x "$WEBIDE_ROOT/bin/node_sf/node"
fi
sed -i "s/NODE_PATH/$NODE_PATH/g" adafruit-webide.sh

update-rc.d adafruit-webide.sh defaults

#set binaries as executable

echo "Attempting to force reload date and time from ntp server"
/etc/init.d/ntp force-reload

#Check if port 80 is in use, use 8080 if so.
PORT_USED=""
if netstat -lnt | awk '$6 == "LISTEN" && $4 ~ ".80"' | grep -q "LISTEN"
then
  redis-cli HMSET server port 8090
  PORT_USED=":8090"
  echo "**** WARNING: PORT 80 IN USE. FALLING BACK TO 8090. ****"
  echo "**** TO CHOOSE A DIFFERENT PORT USE THE FOLLOWING COMMAND: ****"
  echo "**** redis-cli HMSET server port 8090 ****"
  echo "**** AND RESTART THE SERVER ****"
fi

if $OFFLINE
then
  redis-cli HMSET server offline 1
fi

if $GITHUB
then
  redis-cli HMSET server github 1
fi

service adafruit-webide.sh start

if grep -q adafruit-webide.sh /etc/restartd.conf
then
  echo "restartd already configured"
else
  echo 'webide "node" "service adafruit-webide.sh restart" ""' >> /etc/restartd.conf
fi

#kill all restartd processes, and restart one
pkill -f restartd || true
sleep 5s
restartd

echo "**** Starting the server...(please wait) ****"
sleep 20s

echo "**** The Adafruit WebIDE is installed and running! ****"
echo "**** Commands: sudo service adafruit-webide.sh {start,stop,restart} ****"
echo "**** Navigate to http://$(hostname).local$PORT_USED to use the WebIDE"
#echo "**** To run the editor: ****"
#echo "**** cd ~/Adafruit/WebIDE ****"
#echo "**** node webide ****"
