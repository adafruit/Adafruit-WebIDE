Adafruit webIDE
================
This is a simple editor designed to help learn the Raspberry Pi and Beaglebone components, and more. This editor is designed solely for use on your secure private network as of now.

Debian Installation (Raspberry Pi and BeagleBone Black)
============

The WebIDE installer is currently targeting Debian Stretch (latest Raspbian) installations only.

On the Raspberry PI or BeagleBone Black (after expanding the file system):

    curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/master/scripts/install.sh | sudo sh

Alternatively, you can install using the .deb file:

    curl -O https://adafruit-download.s3.amazonaws.com/adafruitwebide-0.3.12-Linux.deb
    sudo dpkg -i adafruitwebide-0.3.12-Linux.deb
    sudo apt-get -f install

Note: As part of the installation process, the 'webide' user is given access to sudo and sudoers,
similar to the 'pi' user.  This is needed in order to easily access GPIO pins from the Editor.  
If you don't need these features, feel free to manually install the editor below.

Note: This is also the default installation for any Debian or Ubuntu operating systems

Manual Installation
============

Follow along in the [installation script][1] and pick and choose
the components you'd like to install.

Uninstallation
============

Debian (Raspberry PI and BeagleBone Black):

    curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/master/scripts/uninstall.sh | sudo sh

Manual Uninstallation
============

Follow along in the [uninstallation script][2] and pick and choose
the components you'd like to remove.

Running the Editor
============

Using Firefox or Chrome (and likely any other webkit browser) on any computer in your internal network:

Raspberry Pi:

    http://raspberrypi.local:8080

BeagleBone:

    http://beaglebone.local:8080

Restart the Editor
============

If for any reason you need to restart the editor, you can execute the following commands in order

    sudo systemctl restart adafruit-webide
    sudo systemctl start adafruit-webide

Sudo is required to restart due to the editor running as the 'webide' user.

Status or Logs for the Editor
============

sudo systemctl status adafruit-webide

Logs are in syslog: /var/log/syslog

License
============

The editor is licensed with AGPL Version 3.
http://www.gnu.org/licenses/agpl-3.0.html

SCREENSHOTS
===========
![ScreenShot](http://www.adafruit.com/adablog/wp-content/uploads/2012/10/WebIDE_Alpha.jpg)

[1]: https://github.com/adafruit/Adafruit-WebIDE/blob/master/scripts/install.sh
[2]: https://github.com/adafruit/Adafruit-WebIDE/blob/master/scripts/uninstall.sh
