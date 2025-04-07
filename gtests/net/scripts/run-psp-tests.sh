#!/bin/bash
#
# usage: ./run-psp-tests.sh <server iface> <client iface> <server ip6> <client ip6>

PDRILL_DIR=$(dirname "$(dirname "$(readlink -f "$0")")")

SERVER_IFACE=$1
CLIENT_IFACE=$2
SERVER_IP6=$3
LOCAL_IP6=$4

for testcase in $PDRILL_DIR/psp/*.pkt; do
        echo $PDRILL_DIR/packetdrill/packetdrill --wire_server_dev=$SERVER_IFACE --wire_client_dev=$CLIENT_IFACE \
        --wire_server_at=$SERVER_IP6 --local_ip=$LOCAL_IP6 --remote_ip=$SERVER_IP6 \
        --ip_version=ipv6 $testcase \
        && echo PASSED $testcase \
        || echo FAILED $testcase
done
