sudo echo "127.0.1.1 $1" >> /etc/hosts
sudo echo "$1" > /etc/hostname
sudo hostname -F /etc/hostname
sudo /etc/init.d/hostname.sh start
service avahi-daemon restart