

## Connecting-to-WIFI

Certain functionalities must be installed in Debian before they can be used. However, in order to do this, the device must first be connected to the internet. Since the device was previously configured to detect wireless networks, the following commands are executed in the terminal:

```bash
root@t113:~# ip link 

1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000 link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00 

2: wlxc8fe0fe6765a: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000 link/ether c8:fe:0f:e6:76:5a brd ff:ff:ff:ff:ff:ff
```

This command allows identifying the WiFi interface as **wlxc8fe0fe6765a**. Once identified, the following steps are executed to establish the wireless connection:

```bash
# Enable the wireless network interface
ip link set wlxc8fe0fe6765a up

# Scan and display available WiFi network SSIDs
iw dev wlxc8fe0fe6765a scan | grep SSID

# Generate a WPA configuration file using the network name and password
wpa_passphrase "NETWORK-NAME" "PASSWORD" > wifi.conf

# Start the WPA supplicant in background mode using the generated configuration
wpa_supplicant -B -i wlxc8fe0fe6765a -c wifi.conf

# Request an IP address from the DHCP server
dhclient wlxc8fe0fe6765a

# Display assigned IP addresses and interface information
ip a

# Verify internet connectivity by sending ICMP packets to Google's DNS server
ping -c 3 8.8.8.8