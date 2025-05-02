// Пины для кнопок 
const uint8_t VOL_UP_PIN = 32;    // Кнопка увеличения громкости
const uint8_t VOL_DOWN_PIN = 33;  // Кнопка уменьшения громкости
const uint8_t NEXT_PIN = 12;     // Кнопка следующего трека
const uint8_t PREV_PIN = 13;     // Кнопка предыдущего трека
const uint8_t PLAY_PAUSE_PIN = 15; // Кнопка воспроизведения/паузы


// Константы для управления громкостью
const float VOLUME_STEP = 0.05;   // Шаг изменения громкости
const float MAX_VOLUME = 0.9;     // Максимальная громкость
const float MIN_VOLUME = 0.0;     // Минимальная громкость
// URLs для управления воспроизведением
char* nextURL = (char*)"http://192.168.1.17:5000/control/next";
char* prevURL = (char*)"http://192.168.1.17:5000/control/prev";
char* toggleURL = (char*)"http://192.168.1.17:5000/control/toggle";

// Обновите константы в buttons.ino для использования динамических URL
void updateControlURLs() {
  // Освобождаем старые URL если они были динамически выделены
  if (nextURL != NULL && nextURL[0] != 'h') free(nextURL);
  if (prevURL != NULL && prevURL[0] != 'h') free(prevURL);
  if (toggleURL != NULL && toggleURL[0] != 'h') free(toggleURL);
  
  // Выделяем память для новых URL
  String nextString = serverBaseURL + "/control/next";
  String prevString = serverBaseURL + "/control/prev";
  String toggleString = serverBaseURL + "/control/toggle";
  
  nextURL = strdup(nextString.c_str());
  prevURL = strdup(prevString.c_str());
  toggleURL = strdup(toggleString.c_str());
  
  Serial.println("URL для кнопок обновлены");
}


// Переменные для отслеживания состояния кнопок
bool lastUpState = HIGH;
bool lastDownState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 100;  // Задержка для подавления дребезга
// Добавляем состояния для кнопок
bool lastNextState = HIGH;
bool lastPrevState = HIGH;
bool lastPlayPauseState = HIGH;


// Переменная для хранения текущей громкости
float currentVolume = 0.2;  // Начальное значение

extern AudioOutputI2S *out;  // Внешняя ссылка на аудио выход
TaskHandle_t buttonTask;     // Handle для задачи обработки кнопок

void setupButtons() {
    // Существующая настройка кнопок громкости
    pinMode(VOL_UP_PIN, INPUT_PULLUP);
    pinMode(VOL_DOWN_PIN, INPUT_PULLUP);
    
    // Настройка новых кнопок
    pinMode(NEXT_PIN, INPUT_PULLUP);
    pinMode(PREV_PIN, INPUT_PULLUP);
    pinMode(PLAY_PAUSE_PIN, INPUT_PULLUP);
    
    // Установка начальной громкости
    out->SetGain(currentVolume);

    // Создание задачи для обработки кнопок
    xTaskCreatePinnedToCore(
        buttonLoop,
        "ButtonTask",
        4096,            // Увеличенный размер стека для HTTP запросов
        NULL,
        1,
        &buttonTask,
        1
    );
}

void buttonLoop(void * parameter) {
    while(true) {
        handleButtons();
        vTaskDelay(pdMS_TO_TICKS(10)); // Небольшая задержка для стабильности
    }
}

void handleButtons() {
    // Чтение текущего состояния кнопок
    bool currentUpState = digitalRead(VOL_UP_PIN);
    bool currentDownState = digitalRead(VOL_DOWN_PIN);
     // Новые состояния кнопок
    bool currentNextState = digitalRead(NEXT_PIN);
    bool currentPrevState = digitalRead(PREV_PIN);
    bool currentPlayPauseState = digitalRead(PLAY_PAUSE_PIN);   

    // Проверка на дребезг контактов
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // Обработка кнопки увеличения громкости
        if (lastUpState == HIGH && currentUpState == LOW) {
            if (currentVolume < MAX_VOLUME) {
                currentVolume += VOLUME_STEP;
                if(currentVolume > MAX_VOLUME) currentVolume = MAX_VOLUME;
                out->SetGain(currentVolume);
                Serial.printf("Volume up: %.2f\n", currentVolume);
            }
            lastDebounceTime = millis();
        }
        
        // Обработка кнопки уменьшения громкости
        if (lastDownState == HIGH && currentDownState == LOW) {
            if (currentVolume > MIN_VOLUME) {
                currentVolume -= VOLUME_STEP;
                if(currentVolume < MIN_VOLUME) currentVolume = MIN_VOLUME;
                out->SetGain(currentVolume);
                Serial.printf("Volume down: %.2f\n", currentVolume);
            }
            lastDebounceTime = millis();
        }
        // Обработка новых кнопок
        if (lastNextState == HIGH && currentNextState == LOW) {
            Serial.println("Next track");
            sendControlRequest(nextURL);
            lastDebounceTime = millis();
        }
        
        if (lastPrevState == HIGH && currentPrevState == LOW) {
            Serial.println("Previous track");
            sendControlRequest(prevURL);
            lastDebounceTime = millis();
        }
        
        if (lastPlayPauseState == HIGH && currentPlayPauseState == LOW) {
            Serial.println("Toggle playback");
            sendControlRequest(toggleURL);
            lastDebounceTime = millis();
        }
    }
    
    // Обновление состояний всех кнопок
    lastUpState = currentUpState;
    lastDownState = currentDownState;
    lastNextState = currentNextState;
    lastPrevState = currentPrevState;
    lastPlayPauseState = currentPlayPauseState;
}

// Функция для отправки HTTP запроса
void sendControlRequest(const char* url) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(url);
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            String payload = http.getString();
            Serial.println(payload);
        } else {
            Serial.printf("HTTP Error: %d\n", httpCode);
        }
        http.end();
    }
}




