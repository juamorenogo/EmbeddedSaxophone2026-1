

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
```

## Permanent-WIFI-config  
  
In order to permanently associate a wireless network with the system, the previously generated `wifi.conf` file can be copied into the appropriate system configuration path:

``` bash
# Copy the generated WiFi configuration file into the WPA supplicant directory
cp wifi.conf /etc/wpa_supplicant/wpa_supplicant.conf
```

After this, the network interfaces configuration file located at `/etc/network/interfaces` must be edited and the following configuration added:

```bash
# Open the network interfaces configuration file
nano /etc/network/interfaces

# --> Add the following lines into the file:

auto wlxc8fe0fe6765a
iface wlxc8fe0fe6765a inet dhcp
    wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf
```

Finally, after rebooting the system, the wireless network configuration should be automatically loaded and the device should reconnect to the configured WiFi network.

## Time-zone-adjustment

In order to correctly configure the system time zone, synchronize the system clock, and keep the date and time automatically updated whenever internet access is available, the following commands can be executed:

```bash
# Start the D-Bus service required by timedatectl
systemctl start dbus

# Configure the system time zone
timedatectl set-timezone America/Bogota

# Install the NTP package for automatic network time synchronization
apt install ntp

# Perform an immediate date and time synchronization using an NTP server
ntpdate pool.ntp.org

# Display the current time, date, and synchronization status
timedatectl status
```

Once configured, the system will automatically maintain the correct date and time whenever an internet connection is available.
## Recommended-packages

Based on the setup process performed during this session, the following packages were most likely installed or recommended for proper system operation and debugging purposes:

```bash
apt install:
	# Wireless networking utilities
	iw
	wpa_supplicant
	
	# Time synchronization and system services
	systemd-timesyncd
	dbus
	
	# Basic terminal utilities
	nano
	htop
	less
	file
	
	# Additional development and debugging tools
	sudo
	git
	evtest
```

