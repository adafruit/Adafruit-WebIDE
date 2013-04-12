cp -r /home/pi/Adafruit-WebIDE/scripts/deb /tmp/deb
cp -r /home/pi/Adafruit-WebIDE /tmp/deb/webide
cd /tmp/deb
cmake .
make package
