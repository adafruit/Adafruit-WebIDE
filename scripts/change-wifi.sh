#!/usr/bin/env bash

if grep -q wpa-ssid /etc/network/interfaces
then
  echo "wpa-ssid exists, modifying..."
  sed -i.bak "/wpa-ssid/s/\"\([^\"]*\)\"/\"$1\"/" /etc/network/interfaces
else
  exit 0
fi

if grep -q wpa-psk /etc/network/interfaces
then
  echo "wpa-psk exists, modifying..."
  sed -i "/wpa-psk/s/\"\([^\"]*\)\"/\"$2\"/" /etc/network/interfaces
else
  exit 0
fi
