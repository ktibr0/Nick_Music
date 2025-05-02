import os
import logging

MUSIC_DIR = "/mnt/usb/multimedia/music"
PLAYLIST_DIR = "/mnt/usb/multimedia/playlists"

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def create_playlists():
    if not os.path.exists(MUSIC_DIR):
        logging.error(f"Папка с музыкой не найдена: {MUSIC_DIR}")
        return

    if not os.path.exists(PLAYLIST_DIR):
        os.makedirs(PLAYLIST_DIR)
        logging.info(f"Создана папка для плейлистов: {PLAYLIST_DIR}")

    for folder_name in os.listdir(MUSIC_DIR):
        folder_path = os.path.join(MUSIC_DIR, folder_name)
        if os.path.isdir(folder_path):
            playlist_path = os.path.join(PLAYLIST_DIR, f"{folder_name}.m3u")
            try:
                with open(playlist_path, 'w') as playlist_file:
                    for root, dirs, files in os.walk(folder_path):
                        files.sort()
                        for file_name in files:
                            if file_name.lower().endswith(('.mp3', '.wav', '.flac', '.ogg', '.m4a', '.aac')):
                                file_path = os.path.join(root, file_name)
                                new_path=os.path.relpath(file_path, MUSIC_DIR)
                                relative_path = f"music/{new_path}" 
                                playlist_file.write(relative_path + '\n')
                logging.info(f"Создан плейлист: {playlist_path}")
            except Exception as e:
                logging.error(f"Ошибка при создании плейлиста {playlist_path}: {e}")

if __name__ == '__main__':
    create_playlists()
