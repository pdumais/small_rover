This is adapted from https://github.com/rodneybakiskan/ps5-esp32. The library was modified to work outside of Arduino and also has a few bug fixes.

The PS5 controller code runs on a ESP32. It cannot run on esp32s3 since that device only supports BLE.
This software listens for incoming dualsense events and forwards them on the i2c bus.


# Building
Everything runs in docker, no need to install ESP IDF

- Change the dualsense MAC address in app_main.c
- run ./build.sh
- change the USB port to target your esp32 device in flash.sh
- run ./flash.sh

# Pairing
On the first usage, the dualsense needs to be put in pairing mode. Then the pairing will be done automatically.


# Known issues
There is sometimes a crash:
    E (30465) BT_L2CAP: L2CAP got conn_comp in bad state: 4  status: 0x4
    I (30465) ps5_L2CAP: [ps5_l2cap_disconnect_ind_cback] l2cap_cid: 0x41 ack_needed: 0
    I (30475) ps5_L2CAP: [ps5_l2cap_disconnect_ind_cback] l2cap_cid: 0x42 ack_needed: 0
    I (30475) ps5_L2CAP: [ps5_l2cap_disconnect_ind_cback] l2cap_cid: 0x40 ack_needed: 0
    W (30485) BT_HCI: hcif conn complete: hdl 0xfff, st 0x4
    E (30495) BT_L2CAP: L2CAP - rcvd ACL for unknown handle:128 ls:0 cid:64 opcode:161 cur count:0
