cd ~
git clone git://github.com/adafruit/Adafruit-WebIDE.git
cd Adafruit-WebIDE
chmod +x bin/node_hf/node
sudo ln -s /home/pi/Adafruit-WebIDE/bin/node_hf/node /usr/bin/node
sudo ln -s /home/pi/Adafruit-WebIDE/bin/node_hf/node /usr/lib/node
curl -L https://npmjs.org/install.sh | sudo sh
npm install
cd ..
cp -r Adafruit-WebIDE Adafruit-WebIDE-Update
cd Adafruit-WebIDE-Update/node_modules
