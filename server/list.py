import os
import json

# Путь к папке с плейлистами
playlists_dir = '/mnt/usb/multimedia/playlists'

# Список для хранения данных
playlists_data = {}

# Начинаем с индекса 00001
index = 1

# Сканируем папку
for filename in os.listdir(playlists_dir):
    if filename.endswith('.m3u'):
        # Форматируем индекс с ведущими нулями
        formatted_index = f"{index:05d}"
        
        # Убираем расширение .m3u из имени файла
        playlist_name = os.path.splitext(filename)[0]
        
        # Добавляем данные в словарь
        playlists_data[formatted_index] = playlist_name
        
        # Увеличиваем индекс
        index += 1

# Записываем данные в JSON-файл
output_file = 'list.json'
with open(output_file, 'w', encoding='utf-8') as f:
    json.dump(playlists_data, f, ensure_ascii=False, indent=4)

print(f"Файл {output_file} успешно создан.")