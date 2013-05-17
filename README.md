Adafruit webIDE
================
This is a simple editor designed to help learn the Raspberry Pi and Beaglebone components, and more.

Raspberry Pi Installation
============

On the Raspberry PI (after expanding the file system):

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh

Alternatively, you can install using the .deb file:

    curl -O http://adafruit-download.s3.amazonaws.com/adafruitwebide-0.3.8-Linux.deb
    sudo dpkg -i adafruitwebide-0.3.8-Linux.deb
    sudo apt-get -f install

Note: As part of the installation process, the 'webide' user is given access to sudo and sudoers, 
similar to the 'pi' user.  This is needed in order to easily access GPIO pins from the Editor.  
If you don't need these features, feel free to manually install the editor below.

Note: This is also the default installation for any Debian or Ubuntu operating systems

Beaglebone Installation (Angstrom)
============

On the Beaglebone (as the default root user), execute each line independently:

    echo "nameserver 8.8.8.8" >> /etc/resolv.conf
    curl -k https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/install-angstrom.sh | sh

Note: The curl -k command is used due to the Beaglebone not having the github SSL certificate in the default installation.

Note: If you've replaced the default operating system (Angstrom) with Debian or Ubuntu, use the Raspberry Pi installation instructions.

Manual Installation (without process monitor)
============

On the Raspberry PI:

    sudo apt-get update && sudo apt-get -y install build-essential nodejs nodejs-legacy npm redis-server git
    git clone git://github.com/adafruit/Adafruit-WebIDE.git
    cd Adafruit-WebIDE
    mkdir tmp
    npm config set tmp tmp
    npm install
    editor config/config.js (change port 80 to your port of choice)
    nodejs server.js

You can look at the install.sh script if you'd like a process monitor, and to install it
as a daemon.

Uninstallation
============

On the Raspberry PI:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/uninstall.sh | sudo sh

Or if you installed with the .deb file:

    sudo apt-get remove adafruitwebide

On the Beaglebone (as the default root user):

    curl -k https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/uninstall-angstrom.sh | sh

Manual Uninstallation
============

On the Raspberry PI:

    rm -r Adafruit-WebIDE
    rm ~/.ssh/id_rsa_bitbucket*

Running the Editor
============

Using Firefox or Chrome (and likely any other webkit browser) on any computer in your internal network:

Raspberry Pi:

    http://raspberrypi.local

BeagleBone:

    http://beaglebone.local:8080

Restart the Editor
============

If for any reason you need to restart the editor, you can execute the following commands in order
    
    sudo service adafruit-webide.sh stop
    sudo service adafruit-webide.sh start

Sudo is required to restart due to the editor running as the 'webide' user.

Advanced Options
============

Offline Mode Installation:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh -s - --offline

Note: Offline mode does not setup git in any way other than installing it.  You'll want to git config your
email and name, and setup your ssh keys.

GitHub Mode Installation:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/alpha/scripts/install.sh | sudo sh -s - --github

Note: GitHub mode does not automatically create, and post an ssh key to your GitHub account.  It requires
a bit more manual setup at this time.

Enable support for Makefiles (execute on the Pi in the terminal, post-installation):

    redis-cli hmset editor:settings enable_make "on"

Disable:

    redis-cli hmset editor:settings enable_make "off"

License
============

The editor is licensed with AGPL Version 3.
http://www.gnu.org/licenses/agpl-3.0.html

SCREENSHOTS
===========
![ScreenShot](http://www.adafruit.com/adablog/wp-content/uploads/2012/10/WebIDE_Alpha.jpg)