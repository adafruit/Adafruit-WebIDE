cd /home/pi/Adafruit-WebIDE
git checkout alpha
git pull origin alpha
tar -zcvf editor.tar.gz * --exclude .git --exclude .gitignore

cd /home/pi/Adafruit-WebIDE-Update
git checkout alpha
git pull origin alpha
tar -zcvf editor-update.tar.gz * --exclude .git --exclude .gitignore