# RFID Музыкальный плеер для слабовидящих

## Введение

Проект представляет собой специализированный музыкальный плеер, созданный для людей с нарушениями зрения. Основная идея заключается в использовании RFID-меток для выбора музыкальных плейлистов - пользователю достаточно приложить метку к считывателю, чтобы начать прослушивание соответствующей музыкальной коллекции.

### Основные возможности
- Управление воспроизведением музыки с помощью RFID-меток
- Физические кнопки для управления громкостью, переключения треков и паузы/воспроизведения
- Автоматическое обнаружение сервера по локальной сети
- Поддержка различных аудиоформатов (MP3, WAV, FLAC и др.)
- Создание и управление плейлистами

### Для кого предназначен проект
Проект будет полезен людям с нарушениями зрения, пожилым людям или всем, кому необходим простой физический интерфейс для управления музыкальной библиотекой без использования экранов или сложных меню.

## Архитектура системы

Система состоит из двух основных компонентов:

1. **Сервер на Raspberry Pi**:
   - Хранение и управление музыкальной библиотекой
   - Запуск MPD (Music Player Daemon) для воспроизведения музыки
   - API на Flask для взаимодействия с клиентской частью
   - Хранение привязок RFID-меток к плейлистам

2. **Клиент на ESP32**:
   - Считывание RFID-меток с помощью модуля PN532
   - Кнопки для управления воспроизведением
   - Вывод звука через I2S DAC (например, MAX98357A)
   - Взаимодействие с сервером через WiFi

### Схема взаимодействия

```
[RFID метка] → [ESP32 + PN532] → [WiFi] → [Raspberry Pi + API] → [MPD] → [Музыка]
                     ↑                                            ↓
              [Кнопки управления]                         [Аудио поток]
                                                                ↓
                                                        [ESP32 + I2S DAC]
                                                                ↓
                                                           [Динамик]
```

## Установка серверной части (Raspberry Pi)

### Требования к оборудованию
- Raspberry Pi (рекомендуется Raspberry Pi 3 или новее)
- microSD карта (минимум 8 GB)
- USB-накопитель для хранения музыки
- Питание для Raspberry Pi

### Настройка чистого Raspbian

1. Скачайте и установите актуальную версию Raspbian OS:
   ```bash
   # Обновление системы
   sudo apt update
   sudo apt upgrade -y
   ```

2. Настройка сетевого имени:
   ```bash
   sudo hostnamectl set-hostname musicbox
   ```

3. Установка необходимых пакетов:
   ```bash
   sudo apt install -y python3-pip mpd mpc python3-flask avahi-daemon
   pip3 install flask transliterate
   ```

### Настройка автомонтирования USB-накопителя

Сначала определите, как система видит ваш USB-диск:

```bash
sudo fdisk -l
```

или

```bash
lsblk
```

Создайте точку монтирования:

```bash
sudo mkdir -p /mnt/usb
```

Создайте файл systemd для монтирования:

```bash
sudo nano /etc/systemd/system/mnt-usb.mount
```

Содержимое файла:
```
[Unit]
Description=Mount USB Drive
DefaultDependencies=no
Before=local-fs.target

[Mount]
What=/dev/sda1  # Замените на ваше устройство
Where=/mnt/usb
Type=auto
Options=defaults,nofail

[Install]
WantedBy=multi-user.target
```

Включите автозапуск монтирования:

```bash
sudo systemctl enable mnt-usb.mount
```

Создайте структуру директорий:
   ```bash
   sudo mkdir -p /mnt/usb/multimedia/music
   sudo mkdir -p /mnt/usb/multimedia/playlists
   sudo chown -R pi:pi /mnt/usb/multimedia
   ```

### Настройка MPD

Отредактируйте конфигурационный файл:
   ```bash
   sudo nano /etc/mpd.conf
   ```

Настройте следующие параметры:
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


Остановите службу MPD для редактирования конфигурации:
   ```bash
   sudo systemctl stop mpd
   ```

Создайте переопределение для службы MPD, чтобы она запускалась после монтирования USB-диска:

```bash
sudo mkdir -p /etc/systemd/system/mpd.service.d/
sudo nano /etc/systemd/system/mpd.service.d/override.conf
```

Содержимое файла:
```
[Unit]
After=mnt-usb.mount
Requires=mnt-usb.mount
```

Включите автозапуск MPD:

```bash
sudo systemctl enable mpd.service
```


### Настройка MPC

Создайте сервис для запуска команд MPC после запуска MPD:

```bash
sudo nano /etc/systemd/system/mpc-startup.service
```

Содержимое файла:
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

Включите автозапуск:

```bash
sudo systemctl enable mpc-startup.service
```

### Установка и настройка Syncthing

Установка Syncthing:
   ```bash
   curl -s https://syncthing.net/release-key.txt | sudo apt-key add -
   echo "deb https://apt.syncthing.net/ syncthing stable" | sudo tee /etc/apt/sources.list.d/syncthing.list
   sudo apt update
   sudo apt install syncthing
   ```

Создайте переопределение для Syncthing, чтобы он запускался после MPD:

```bash
sudo mkdir -p /etc/systemd/system/syncthing@pi.service.d/
sudo nano /etc/systemd/system/syncthing@pi.service.d/override.conf
```

Содержимое файла (замените `pi` на имя вашего пользователя, если оно отличается):
```
[Unit]
After=mpd.service
Requires=mpd.service
```

Включите автозапуск Syncthing:

```bash
sudo systemctl enable syncthing@pi.service
```

### Запуск Python-приложения


Скопируйте файлы сервера в директорию `/mnt/usb/utils`:
   ```bash
   mkdir -p /mnt/usb/utils
   # Скопируйте файлы app.py, list.py, playlist.py и т.д.
   ```

Создайте сервис для запуска Python-приложения:

```bash
sudo nano /etc/systemd/system/my-python-app.service
```

Содержимое файла:
```
[Unit]
Description=My Python Application
After=syncthing@pi.service mpc-startup.service
Requires=mnt-usb.mount
Wants=syncthing@pi.service mpc-startup.service

[Service]
Type=simple
User=pi  # Замените на имя вашего пользователя
Group=pi  # И соответствующую группу
WorkingDirectory=/mnt/usb/utils
Environment=PYTHONPATH=/mnt/usb/utils
ExecStart=/usr/bin/python3 /mnt/usb/utils/app.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Включите автозапуск:

```bash
sudo systemctl enable my-python-app.service
```

### Настройка обратного SSH-туннеля

Это позволит удаленно управлять Raspberry Pi через туннель к вашему серверу.

#### Создание SSH-ключа для беспарольного доступа

```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ""
ssh-copy-id -i ~/.ssh/id_ed25519.pub username@your-server-ip
```

#### Создание сервиса для туннеля

```bash
sudo nano /etc/systemd/system/reverse-ssh.service
```

Содержимое файла:
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

Включите автозапуск туннеля:

```bash
sudo systemctl enable reverse-ssh.service
```

#### Настройка сервера

На удаленном сервере откройте файл конфигурации SSH:

```bash
sudo nano /etc/ssh/sshd_config
```

Добавьте или измените следующие строки:
```
GatewayPorts yes
AllowTcpForwarding yes
```

Перезапустите SSH-сервер:
```bash
sudo systemctl restart sshd
```

#### Подключение к Raspberry Pi через туннель

На удаленном сервере выполните:
```bash
ssh -p 10022 pi@localhost
```






## Установка клиентской части (ESP32)

### Требования к оборудованию
- ESP32 (рекомендуется DevKit или WROOM)
- Модуль RFID PN532
- I2S DAC (например, MAX98357A)
- Кнопки управления (5 штук)
- Динамик
- Макетная плата, провода, корпус

### Установка Arduino IDE и прошивка ESP32

1. Скачайте и установите [Arduino IDE](https://www.arduino.cc/en/software)

2. Добавьте поддержку ESP32:
   - Откройте Arduino IDE
   - Перейдите в Файл -> Настройки
   - В поле "Дополнительные ссылки для Менеджера плат" добавьте:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Перейдите в Инструменты -> Плата -> Менеджер плат
   - Найдите и установите "ESP32"

3. Установите необходимые библиотеки:
   - Инструменты -> Управление библиотеками
   - Установите:
     - ESP8266Audio
     - Adafruit PN532
     - WiFi
     - HTTPClient

4. Подготовка кода:
   - Скопируйте файлы из директории `client` в ваш проект Arduino
   - Настройте параметры WiFi в файле `main_1.ino`
   - При необходимости адаптируйте пины под вашу схему подключения

5. Компиляция и загрузка:
   - Выберите нужную плату ESP32 в меню Инструменты -> Плата
   - Нажмите кнопку "Загрузка"

### Схема подключения

- **PN532 (RFID)**:
  - SDA: GPIO21
  - SCL: GPIO22
  
- **MAX98357A (I2S DAC)**:
  - BCK: GPIO26
  - LRC: GPIO25
  - DIN: GPIO27
  
- **Кнопки**:
  - Громкость + : GPIO32
  - Громкость - : GPIO33
  - Следующий трек: GPIO12
  - Предыдущий трек: GPIO13
  - Воспроизведение/Пауза: GPIO15

## Утилиты

В директории `utils` находятся вспомогательные скрипты для работы с музыкальной библиотекой:

- **playlist.py**: Создает плейлисты на основе структуры папок с музыкой
  ```bash
  python3 playlist.py
  ```

- **rename.py**: Транслитерирует кириллические названия файлов и папок, удаляет специальные символы
  ```bash
  python3 rename.py
  ```



### Исправление ошибки "unable to resolve host"

Если вы видите сообщение "sudo: unable to resolve host musicbox: Name or service not known", исправьте это, отредактировав файл `/etc/hosts`:

```bash
sudo nano /etc/hosts
```

Добавьте или измените строку:
```
127.0.1.1       musicbox
```

Файл должен выглядеть примерно так:
```
127.0.0.1       localhost
127.0.1.1       musicbox

# Остальные строки...
```



### Управление сервисами

Проверка статуса сервиса:
```bash
sudo systemctl status имя_сервиса.service
```

Запуск сервиса:
```bash
sudo systemctl start имя_сервиса.service
```

Остановка сервиса:
```bash
sudo systemctl stop имя_сервиса.service
```

Перезапуск сервиса:
```bash
sudo systemctl restart имя_сервиса.service
```

Просмотр логов сервиса:
```bash
sudo journalctl -u имя_сервиса.service
```

---



## Использование

### Подготовка музыкальной библиотеки

1. Скопируйте музыку на USB-накопитель в директорию `/multimedia/music/`
2. Организуйте музыку по папкам - каждая папка будет отдельным плейлистом
3. При необходимости используйте утилиту rename.py для транслитерации имен
4. Запустите утилиту playlist.py для создания плейлистов
5. Обновите базу данных MPD:
   ```bash
   mpc update
   ```

### Привязка RFID-меток к плейлистам

1. При первом поднесении RFID-метки к считывателю, она будет зарегистрирована в системе как "unsigned"
2. Зайдите в файл `/mnt/usb/multimedia/playlists/list.json` и замените "unsigned" на имя плейлиста (без расширения .m3u)
3. Повторное поднесение метки начнет воспроизведение привязанного плейлиста

### Управление воспроизведением

- Используйте физические кнопки на устройстве для управления воспроизведением
- Для смены плейлиста приложите соответствующую RFID-метку к считывателю





