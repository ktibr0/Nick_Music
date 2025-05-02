#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <esp_system.h>  // Для управления частотой процессора ESP32
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include <ESPmDNS.h>





TaskHandle_t audioTask;


const char* ssid = "SID_WIFI";
const char* password = "PASSWORD";

String serverIP = ""; // IP-адрес сервера будет определяться динамически
String streamPort = "8000"; // Порт музыкального потока
String controlPort = "5000"; // Порт API управления
String URLs = ""; // Полный URL для музыкального потока
String serverBaseURL = ""; // Базовый URL для API


AudioGeneratorMP3 *mp3;
AudioFileSource *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;


const uint8_t sda = 21;
const uint8_t scl = 22;

Adafruit_PN532 nfc(sda, scl); 

// Модифицируйте функцию restartStream
void restartStream() {
  if (mp3->isRunning()) {
    mp3->stop();
  }

  delete buff;
  delete file;

  // Проверяем, что URL установлен
  if (URLs.length() == 0) {
    Serial.println("ОШИБКА: URL для аудио не установлен при перезапуске!");
    return;
  }

  file = new AudioFileSourceHTTPStream(URLs.c_str());
  buff = new AudioFileSourceBuffer(file, 32768);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  mp3->begin(buff, out);
  Serial.println("Stream restarted.");
}

bool findMusicServer() {
  Serial.println("Начинаем поиск сервера musicbox.local...");
  
  // Инициализируем mDNS
  if (!MDNS.begin("esp32player")) {
    Serial.println("Ошибка запуска mDNS!");
    return false;
  }
  
  // Ищем musicbox.local
  IPAddress serverAddr = MDNS.queryHost("musicbox");
  
  // Если не найден, выводим сообщение и возвращаем false
  if (serverAddr.toString() == "0.0.0.0") {
    Serial.println("Сервер musicbox.local не найден!");
    return false;
  }
  
  // Если нашли, сохраняем IP и формируем URL'ы
  serverIP = serverAddr.toString();
  URLs = "http://" + serverIP + ":" + streamPort;
  serverBaseURL = "http://" + serverIP + ":" + controlPort;
  
  Serial.print("Найден сервер musicbox.local по адресу: ");
  Serial.println(serverIP);
  Serial.print("URL для потока: ");
  Serial.println(URLs);
  Serial.print("URL для API: ");
  Serial.println(serverBaseURL);
  
  return true;
}





void setup() {
  Serial.begin(115200);

    
  // Увеличиваем частоту CPU для лучшей производительности
  setCpuFrequencyMhz(240);
  Wire.begin(sda, scl);
  
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Подключение к Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключен!");
  
  // Находим сервер через mDNS
  if (!findMusicServer()) {
    Serial.println("Не удалось найти музыкальный сервер. Перезагрузите устройство или проверьте подключение.");
    // Здесь можно добавить код для индикации ошибки (светодиод/звук)
  } else {
    updateControlURLs(); // Обновляем URL'ы для кнопок управления
  }





  Serial.println("PN532 Test");
  nfc.begin(); // Инициализируем PN532
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // Stop forever
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  nfc.SAMConfig();
  
  AUDIO_init();
  setupButtons();
  
  xTaskCreatePinnedToCore(
    audioLoop,        // Функция задачи
    "AudioTask",      // Имя задачи
    16000,           // Размер стека
    NULL,            // Параметры задачи
    2,               // Приоритет
    &audioTask,      // Хендл задачи
    0                // Ядро CPU (0 или 1)
  );
}

void audioLoop(void * parameter) {
  while(true) {
    handleAudio();
    vTaskDelay(1); // Даем другим задачам шанс выполниться
  }
}

// Модифицированная функция loop в main_1.ino для обработки RFID-меток
void loop() {
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the UID
  uint8_t uidLength; // Length of the UID (4, 7 or 10 bytes)

  uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.print("Found an RFID tag with UID: ");
    String rfidUID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      rfidUID += String(uid[i], HEX);
    }
    Serial.print("UID метки: ");
    Serial.println(rfidUID);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = serverBaseURL + "/play/" + rfidUID;
      http.begin(url);
      int httpCode = http.GET();

      if (httpCode == 200) {
        String payload = http.getString();
        Serial.println(payload);
        delay(1000); // Задержка перед перезапуском
        restartStream();
      } else if (httpCode == 404) {
        Serial.println("Метка не привязана. Создаётся файл на сервере...");
      } else {
        Serial.print("Ошибка сервера: ");
        Serial.println(httpCode);
      }
      http.end();
    } else {
      Serial.println("Wi-Fi не подключён!");
    }
  } else {
    Serial.println("Нет метки или ошибка чтения.");
  }

  delay(1000);
}
