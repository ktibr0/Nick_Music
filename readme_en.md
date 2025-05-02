# RFID Music Player for the Visually Impaired

## Introduction

This project is a specialized music player designed for people with visual impairments. The core concept uses RFID tags to select music playlists - users simply place a tag on the reader to start playing the corresponding music collection.

### Key Features
- Control music playback using RFID tags
- Physical buttons for volume control, track navigation, and play/pause
- Automatic server discovery on the local network
- Support for various audio formats (MP3, WAV, FLAC, etc.)
- Playlist creation and management

### Who This Project Is For
This project will benefit people with visual impairments, elderly individuals, or anyone who needs a simple physical interface to manage a music library without screens or complex menus.

## System Architecture

The system consists of two main components:

1. **Raspberry Pi Server**:
   - Stores and manages the music library
   - Runs MPD (Music Player Daemon) for music playback
   - Flask API for interacting with the client
   - Stores RFID tag to playlist mappings

2. **ESP32 Client**:
   - Reads RFID tags using a PN532 module
   - Provides control buttons for playback
   - Outputs audio through an I2S DAC (e.g., MAX98357A)
   - Communicates with the server via WiFi

### Interaction Diagram

```
[RFID tag] → [ESP32 + PN532] → [WiFi] → [Raspberry Pi + API] → [MPD] → [Music]
                 ↑                                             ↓
          [Control buttons]                              [Audio stream]
                                                             ↓
                                                     [ESP32 + I2S DAC]
                                                             ↓
                                                         [Speaker]
```

## Server Installation (Raspberry Pi)

### Hardware Requirements
- Raspberry Pi (Raspberry Pi 3 or newer recommended)
- microSD card (minimum 8 GB)
- USB drive for music storage
- Power supply for Raspberry Pi

### Clean Raspbian Setup

1. Download and install the latest version of Raspbian OS:
   ```bash
   # Update system
   sudo apt update
   sudo apt upgrade -y
   ```

2. Configure network hostname:
   ```bash
   sudo hostnamectl set-hostname musicbox
   ```

3. Install required packages:
   ```bash
   sudo apt install -y python3-pip mpd mpc python3-flask avahi-daemon
   pip3 install flask transliterate
   ```

### USB Drive Auto-mount Setup

First, identify how your system sees the USB drive:

```bash
sudo fdisk -l
```

or

```bash
lsblk
```

Create a mount point:

```bash
sudo mkdir -p /mnt/usb
```

Create a systemd file for mounting:

```bash
sudo nano /etc/systemd/system/mnt-usb.mount
```

File contents:
```
[Unit]
Description=Mount USB Drive
DefaultDependencies=no
Before=local-fs.target

[Mount]
What=/dev/sda1  # Replace with your device
Where=/mnt/usb
Type=auto
Options=defaults,nofail

[Install]
WantedBy=multi-user.target
```

Enable auto-mount at startup:

```bash
sudo systemctl enable mnt-usb.mount
```

Create directory structure:
   ```bash
   sudo mkdir -p /mnt/usb/multimedia/music
   sudo mkdir -p /mnt/usb/multimedia/playlists
   sudo chown -R pi:pi /mnt/usb/multimedia
   ```

### MPD Configuration

Edit the configuration file:
   ```bash
   sudo nano /etc/mpd.conf
   ```

Configure the following parameters:
   ```
	music_directory "/mnt/usb/multimedia"
	playlist_directory "/mnt/usb/multimedia/playlists"
	db_file "/mnt/usb/spec/tag_cache"
	log_file "/mnt/usb/spec/mpd.log"
	pid_file "/run/mpd/pid"
	state_file "/mnt/usb/spec/state"
	sticker_file "/mnt/usb/spec/sticker.sql"
	user "mpd"
	bind_to_address "0.0.0.0"
	port "6600"
	audio_output {
		type "httpd"
		name "Stream"
		encoder "lame"
		port "8000"
		bitrate "128"
		format "44100:16:2"
   ```

Stop the MPD service to edit the configuration:
   ```bash
   sudo systemctl stop mpd
   ```

Create an override for the MPD service so it starts after the USB drive is mounted:

```bash
sudo mkdir -p /etc/systemd/system/mpd.service.d/
sudo nano /etc/systemd/system/mpd.service.d/override.conf
```

File contents:
```
[Unit]
After=mnt-usb.mount
Requires=mnt-usb.mount
```

Enable MPD autostart:

```bash
sudo systemctl enable mpd.service
```

### MPC Configuration

Create a service to run MPC commands after MPD starts:

```bash
sudo nano /etc/systemd/system/mpc-startup.service
```

File contents:
```
[Unit]
Description=MPC Startup Commands
After=mpd.service
Requires=mpd.service

[Service]
Type=oneshot
ExecStart=/usr/bin/mpc clear
ExecStart=/usr/bin/mpc update
ExecStart=/usr/bin/mpc random on
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

Enable autostart:

```bash
sudo systemctl enable mpc-startup.service
```

### Syncthing Installation and Configuration

Install Syncthing:
   ```bash
   curl -s https://syncthing.net/release-key.txt | sudo apt-key add -
   echo "deb https://apt.syncthing.net/ syncthing stable" | sudo tee /etc/apt/sources.list.d/syncthing.list
   sudo apt update
   sudo apt install syncthing
   ```

Create an override for Syncthing to start after MPD:

```bash
sudo mkdir -p /etc/systemd/system/syncthing@pi.service.d/
sudo nano /etc/systemd/system/syncthing@pi.service.d/override.conf
```

File contents (replace `pi` with your username if different):
```
[Unit]
After=mpd.service
Requires=mpd.service
```

Enable Syncthing autostart:

```bash
sudo systemctl enable syncthing@pi.service
```

### Starting the Python Application

Copy the server files to the `/mnt/usb/utils` directory:
   ```bash
   mkdir -p /mnt/usb/utils
   # Copy app.py, list.py, playlist.py and other files
   ```

Create a service to run the Python application:

```bash
sudo nano /etc/systemd/system/my-python-app.service
```

File contents:
```
[Unit]
Description=My Python Application
After=syncthing@pi.service mpc-startup.service
Requires=mnt-usb.mount
Wants=syncthing@pi.service mpc-startup.service

[Service]
Type=simple
User=pi  # Replace with your username
Group=pi  # And corresponding group
WorkingDirectory=/mnt/usb/utils
Environment=PYTHONPATH=/mnt/usb/utils
ExecStart=/usr/bin/python3 /mnt/usb/utils/app.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable autostart:

```bash
sudo systemctl enable my-python-app.service
```

### Reverse SSH Tunnel Setup

This allows remote management of the Raspberry Pi through a tunnel to your server.

#### Creating an SSH Key for Password-less Access

```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ""
ssh-copy-id -i ~/.ssh/id_ed25519.pub username@your-server-ip
```

#### Creating a Service for the Tunnel

```bash
sudo nano /etc/systemd/system/reverse-ssh.service
```

File contents:
```
[Unit]
Description=Reverse SSH Tunnel
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=pi
ExecStart=/usr/bin/ssh -o ServerAliveInterval=60 -o ExitOnForwardFailure=yes -N -R 0.0.0.0:10022:localhost:22 username@your-server-ip
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable tunnel autostart:

```bash
sudo systemctl enable reverse-ssh.service
```

#### Server Configuration

On the remote server, open the SSH configuration file:

```bash
sudo nano /etc/ssh/sshd_config
```

Add or modify the following lines:
```
GatewayPorts yes
AllowTcpForwarding yes
```

Restart the SSH server:
```bash
sudo systemctl restart sshd
```

#### Connecting to the Raspberry Pi through the Tunnel

On the remote server, run:
```bash
ssh -p 10022 pi@localhost
```

## Client Installation (ESP32)

### Hardware Requirements
- ESP32 (DevKit or WROOM recommended)
- RFID PN532 module
- I2S DAC (e.g., MAX98357A)
- Control buttons (5 pieces)
- Speaker
- Breadboard, wires, enclosure

### Arduino IDE Installation and ESP32 Programming

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)

2. Add ESP32 support:
   - Open Arduino IDE
   - Go to File -> Preferences
   - In the "Additional Board Manager URLs" field, add:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools -> Board -> Board Manager
   - Find and install "ESP32"

3. Install required libraries:
   - Tools -> Manage Libraries
   - Install:
     - ESP8266Audio
     - Adafruit PN532
     - WiFi
     - HTTPClient

4. Code preparation:
   - Copy files from the `client` directory to your Arduino project
   - Configure WiFi parameters in `main_1.ino`
   - Adapt pins to your connection scheme if necessary

5. Compilation and upload:
   - Select the appropriate ESP32 board in Tools -> Board
   - Click the "Upload" button

### Connection Diagram

- **PN532 (RFID)**:
  - SDA: GPIO21
  - SCL: GPIO22
  
- **MAX98357A (I2S DAC)**:
  - BCK: GPIO26
  - LRC: GPIO25
  - DIN: GPIO27
  
- **Buttons**:
  - Volume +: GPIO32
  - Volume -: GPIO33
  - Next track: GPIO12
  - Previous track: GPIO13
  - Play/Pause: GPIO15

![circuit_image](https://github.com/user-attachments/assets/6141e4ac-c05b-4bb8-a194-ce822f5e7f73)

## Utilities

The `utils` directory contains helper scripts for working with the music library:

- **playlist.py**: Creates playlists based on the folder structure of your music
  ```bash
  python3 playlist.py
  ```

- **rename.py**: Transliterates Cyrillic file and folder names, removes special characters
  ```bash
  python3 rename.py
  ```

### Fixing "unable to resolve host" Error

If you see the message "sudo: unable to resolve host musicbox: Name or service not known", fix it by editing the `/etc/hosts` file:

```bash
sudo nano /etc/hosts
```

Add or modify the line:
```
127.0.1.1       musicbox
```

The file should look something like this:
```
127.0.0.1       localhost
127.0.1.1       musicbox

# Other lines...
```

## Usage

### Preparing the Music Library

1. Copy music to the USB drive in the `/multimedia/music/` directory
2. Organize music into folders - each folder will be a separate playlist
3. Use the rename.py utility for transliteration of names if needed
4. Run the playlist.py utility to create playlists
5. Update the MPD database:
   ```bash
   mpc update
   ```

### Linking RFID Tags to Playlists

1. When an RFID tag is first brought to the reader, it will be registered in the system as "unsigned"
2. Open the file `/mnt/usb/multimedia/playlists/list.json` and replace "unsigned" with the playlist name (without the .m3u extension)
3. Bringing the tag to the reader again will start playing the associated playlist

### Playback Control

- Use the physical buttons on the device to control playback
- To change playlists, place the corresponding RFID tag on the reader
