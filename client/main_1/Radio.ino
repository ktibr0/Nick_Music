// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}




// Модифицируйте функцию AUDIO_init в Radio.ino
void AUDIO_init(void) {
    // Проверяем, что URL получен
    if (URLs.length() == 0) {
        Serial.println("ОШИБКА: URL для аудио не установлен!");
        return;
    }
    const int BUFFER_SIZE = 32768;    
    // Используем AudioFileSourceHTTPStream вместо ICYStream
    file = new AudioFileSourceHTTPStream(URLs.c_str());

    // Отключаем обработку метаданных
    file->RegisterMetadataCB(nullptr, nullptr);

    // Инициализация буфера
    buff = new AudioFileSourceBuffer(file, 8192);
    buff->RegisterStatusCB(StatusCallback, (void*)"buffer");

    // Настройка I2S для MAX98357A
    out = new AudioOutputI2S();
    out->SetPinout(26, 25, 27); // Пины для BCK, LRC, DIN
    out->SetGain(0.1); // Настройка уровня громкости

    // Инициализация MP3-декодера
    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
    mp3->begin(buff, out);

    delay(300); // Пауза для стабильной инициализации
}





void  handleAudio() {
   static int lastms = 0;

  if (mp3->isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
     }
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
  }
}




