#!/usr/bin/env bash

ping -q -w 1 -c 1 `ip r | grep default | cut -d ' ' -f 3` > /dev/null && echo "internet ok" || echo "internet error"

/etc/init.d/ntp force-reload

command="/etc/init.d/ntp force-reload"
$command 2>/dev/null

if (( $? == 0 )); then
    echo "success"
else
    current_datetime="$(curl http://time.mknetworks.co.uk/)"
    date -s "$current_datetime"
fi