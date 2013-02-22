cd /home/pi/Adafruit-WebIDE
git checkout alpha
git pull origin alpha
rm webide.tar.gz
tar -zcvf webide.tar.gz * --exclude .git --exclude .gitignore

cd /home/pi/Adafruit-WebIDE-Update
git checkout alpha
git pull origin alpha
rm webide-update.tar.gz
tar -zcvf webide-update.tar.gz * --exclude .git --exclude .gitignore