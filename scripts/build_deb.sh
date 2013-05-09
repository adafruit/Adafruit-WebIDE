cp -r /home/pi/Adafruit-WebIDE/scripts/deb /tmp
cp -r /home/pi/Adafruit-WebIDE /tmp/deb/webide
cd /tmp/deb
cmake .
make package
