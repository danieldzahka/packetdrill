#!/bin/bash
#
# usage: ./psp-wireserver.sh eth0

IFACE=$1
./packetdrill/packetdrill --wire_server --ip_version=ipv6 --wire_server_dev=$IFACE
