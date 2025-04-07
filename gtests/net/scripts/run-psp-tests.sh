#!/bin/bash
#
# usage: ./run-psp-tests.sh

NSIM_DEV_1_ID=$((256 + RANDOM % 256))
NSIM_DEV_1_SYS=/sys/bus/netdevsim/devices/netdevsim$NSIM_DEV_1_ID
NSIM_DEV_2_ID=$((512 + RANDOM % 256))
NSIM_DEV_2_SYS=/sys/bus/netdevsim/devices/netdevsim$NSIM_DEV_2_ID

NSIM_DEV_SYS_NEW=/sys/bus/netdevsim/new_device
NSIM_DEV_SYS_DEL=/sys/bus/netdevsim/del_device
NSIM_DEV_SYS_LINK=/sys/bus/netdevsim/link_device
NSIM_DEV_SYS_UNLINK=/sys/bus/netdevsim/unlink_device

setup_ns()
{
        set -e
        ip netns add nssv
        ip netns add nscl

        devlink dev reload netdevsim/netdevsim$NSIM_DEV_1_ID netns nssv
        devlink dev reload netdevsim/netdevsim$NSIM_DEV_2_ID netns nscl

        udevadm settle

        ip netns exec nssv ip addr add '2001:db8::2/64' dev eth0
        ip netns exec nscl ip addr add '2001:db8::1/64' dev eth0

        ip netns exec nssv ip link set dev eth0 up
        ip netns exec nscl ip link set dev eth0 up
        set +e
}

cleanup()
{
	ip netns del nscl
	ip netns del nssv

        udevadm settle

        echo $NSIM_DEV_1_ID > $NSIM_DEV_SYS_DEL
        echo $NSIM_DEV_2_ID > $NSIM_DEV_SYS_DEL
}

echo $NSIM_DEV_1_ID > $NSIM_DEV_SYS_NEW
echo $NSIM_DEV_2_ID > $NSIM_DEV_SYS_NEW
udevadm settle

setup_ns
trap cleanup EXIT

NSIM_DEV_1_FD=$((256 + RANDOM % 256))
exec {NSIM_DEV_1_FD}</var/run/netns/nssv
NSIM_DEV_1_IFIDX=$(ip netns exec nssv cat /sys/class/net/eth0/ifindex)

NSIM_DEV_2_FD=$((256 + RANDOM % 256))
exec {NSIM_DEV_2_FD}</var/run/netns/nscl
NSIM_DEV_2_IFIDX=$(ip netns exec nscl cat /sys/class/net/eth0/ifindex)

echo "$NSIM_DEV_1_FD:$NSIM_DEV_1_IFIDX $NSIM_DEV_2_FD:$NSIM_DEV_2_IFIDX" > $NSIM_DEV_SYS_LINK

PDRILL_DIR=$(dirname "$(dirname "$0")")

# start the wireserver
ip netns exec nssv $PDRILL_DIR/packetdrill/packetdrill --wire_server --ip_version=ipv6 --wire_server_dev=eth0 &
PID=$!
sleep 2

for testcase in $PDRILL_DIR/psp/*.pkt; do
        ip netns exec nscl $PDRILL_DIR/packetdrill/packetdrill --wire_server_dev=eth0 --wire_client_dev=eth0 --wire_server_at=2001:db8::2 --local_ip=2001:db8::1 --remote_ip=2001:db8::2 --ip_version=ipv6  $PDRILL_DIR/psp/psp-basic.pkt \
        && echo PASSED $testcase \
        || echo FAILED $testcase
done

# stop the wireserver
kill -9 $PID > /dev/null 2>&1
