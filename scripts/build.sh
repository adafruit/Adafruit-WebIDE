rm editor*

git pull origin master
git pull origin alpha
git checkout alpha
git merge master
git push origin alpha
git checkout master

ssh pi@raspberrypi.local 'bash -s' < scripts/remote.sh

scp pi@raspberrypi.local:/home/pi/Adafruit-WebIDE/webide.tar.gz webide-$1.tar.gz
scp pi@raspberrypi.local:/home/pi/Adafruit-WebIDE-Update/webide-update.tar.gz webide-$1-update.tar.gz