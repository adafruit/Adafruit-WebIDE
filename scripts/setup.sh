cd ~
git clone git://github.com/adafruit/Adafruit-WebIDE.git
cd Adafruit-WebIDE
curl -L https://npmjs.org/install.sh | sudo sh
npm install
cd ..
cp -r Adafruit-WebIDE Adafruit-WebIDE-Update
cd Adafruit-WebIDE-Update/node_modules
rm -rf winston socket.io jsDAV connect-redis request connect express gitty jade redis validator
