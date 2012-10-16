if grep -q wpa-ssid /etc/network/interfaces
then
  echo "wpa-ssid exists, modifying..."
  sed "/wpa-ssid/s/\"\([^\"]*\)\"/\"$1\"/" /etc/network/interfaces
else
  exit 0
fi
