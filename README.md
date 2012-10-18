Adafruit WebIDE
================
This is a simple editor designed to help learn the Raspberry PI components, and more.

Easiest Installation
============

On the Raspberry PI:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh

Note: As part of the installation process, the 'webide' user is given access to sudo and sudoers, 
similar to the 'pi' user.  This is needed in order to easily access GPIO pins from the Editor.  
If you don't need these features, feel free to manually install the editor below.

Manual Installation (without process monitor)
============

On the Raspberry PI:

    git clone git://github.com/adafruit/Adafruit-WebIDE.git
    cd Adafruit-WebIDE
    sudo apt-get install nodejs npm redis-server git -y
    mkdir tmp
    npm config set tmp tmp
    npm install
    vim editor/config/config.js (change port 80 to your port of choice)
    node server.js

You can look at the install.sh script if you'd like a process monitor, and to install it
as a daemon.

Uninstallation
============

On the Raspberry PI:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/uninstall.sh | sudo sh

Manual Uninstallation
============

On the Raspberry PI:

    rm -r Adafruit-WebIDE
    rm ~/.ssh/id_rsa_bitbucket*

Running the Editor
============

Using Firefox or Chrome (and likely any other webkit browser) on any computer in your internal network:

    http://raspberrypi.local

Restart the Editor
============

If for any reason you need to restart the editor, you can execute the following commands in order
    
    sudo service adafruit-webide.sh stop
    sudo service adafruit-webide.sh start

Sudo is required to restart due to the editor running as the 'webide' user.

Offline Mode
============

Yup, there is basic support for offline mode.  Just switch the 'offline' flag in the editor/config/config.js file to true when you're coding on a boat, in a submarine, or on your bicycle (not recommended).

License
============

The editor is licensed with AGPL Version 3.
http://www.gnu.org/licenses/agpl-3.0.html

SCREENSHOTS
===========
![ScreenShot](http://www.adafruit.com/adablog/wp-content/uploads/2012/10/WebIDE_Alpha.jpg)