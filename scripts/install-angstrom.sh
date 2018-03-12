#!/usr/bin/env bash

# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh
# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/test/scripts/install.sh | sudo sh
# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/offline/scripts/install.sh | sudo sh -s - --offline

#tar -zcvf editor.tar.gz * --exclude .git --exclude .gitignore
#tar -zcvf editor-update.tar.gz * --exclude .git --exclude .gitignore
#sudo -u webide -g webide node server

#curl -k https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sh

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

echo "Attempting to force reload date and time from ntp server"
/usr/bin/ntpdate -b -s -u pool.ntp.org

echo "**** Downloading the latest version of the WebIDE ****"
curl -L https://adafruit-download.s3.amazonaws.com/webide-0.3.12.tar.gz | tar xzf -

echo "**** Installing required libraries ****"
echo "**** (redis-server git avahi-daemon i2c-tools python-smbus openssh-keygen) ****"
opkg update
opkg install nodejs git avahi-daemon i2c-tools python-smbus openssh-keygen

if ! redis-cli PING
then
    curl http://download.redis.io/redis-stable.tar.gz | tar xvzf -
    cd redis-stable
    make
    cp src/redis-server /usr/bin/redis-server
    cp src/redis-cli /usr/bin/redis-cli
    mkdir -p /etc/redis
    mkdir -p /var/redis
    curl -k https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/redis/redis_6379 > /etc/init.d/redis_6379
    curl -k https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/redis/redis.conf > /etc/redis/6379.conf
    mkdir -p /var/redis/6379
    chmod +x /etc/init.d/redis_6379
    update-rc.d redis_6379 defaults
    /etc/init.d/redis_6379 start
    cd "$WEBIDE_ROOT"
    rm -rf redis-stable
fi

wget http://feeds.angstrom-distribution.org/feeds/next/ipk/eglibc/all/lsb-base_3.2-r0.9_all.ipk
opkg install lsb-base_3.2-r0.9_all.ipk
rm lsb-base_3.2-r0.9_all.ipk

wget http://feeds.angstrom-distribution.org/feeds/v2012.05/ipk/eglibc/armv7a/base/sudo_1.8.4p4-r1_armv7a.ipk
opkg install sudo_1.8.4p4-r1_armv7a.ipk
rm sudo_1.8.4p4-r1_armv7a.ipk

echo "**** Create webide user and group ****"
groupadd webide || true
useradd -g webide webide || true
usermod -a -G sudo webide || true

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


#Check if port 80 is in use, use 8080 if so.
PORT_USED=""
if netstat -lnt | awk '$6 == "LISTEN" && $4 ~ ".80"' | grep -q "LISTEN"
then
  redis-cli HMSET server port 8080
  PORT_USED=":8080"
  echo "**** WARNING: PORT 80 IN USE. FALLING BACK TO 8080. ****"
  echo "**** TO CHOOSE A DIFFERENT PORT USE THE FOLLOWING COMMAND: ****"
  echo "**** redis-cli HMSET server port 8080 ****"
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

echo "**** Setting up systemd scripts"
curl -k https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/adafruit-webide-angstrom.service > /lib/systemd/system/adafruit-webide-angstrom.service
cd /etc/systemd/system/multi-user.target.wants
ln -s /lib/systemd/system/adafruit-webide-angstrom.service adafruit-webide-angstrom.service
systemctl daemon-reload
systemctl start adafruit-webide-angstrom.service

echo "**** Starting the server...(please wait) ****"
sleep 20s

echo "**** The Adafruit webIDE is installed and running! ****"
echo "**** Navigate to http://$(hostname).local$PORT_USED to use the WebIDE"
