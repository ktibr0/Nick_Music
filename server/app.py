from flask import Flask, jsonify, request
import os
import json
import subprocess

app = Flask(__name__)

# Пути к папкам
MUSIC_DIR = "/mnt/usb/multimedia"
PLAYLIST_DIR = "/mnt/usb/multimedia/playlists"
METADATA_FILE = os.path.join(PLAYLIST_DIR, "list.json")

# Команда MPD для воспроизведения плейлиста
MPC_COMMAND = "mpc"
subprocess.run([MPC_COMMAND, "update"], check=True)
subprocess.run([MPC_COMMAND, "random", "on"], check=True)
# Загрузка JSON-файла с привязками
def load_metadata():
    if not os.path.exists(METADATA_FILE):
        return {}  # Если файла нет, возвращаем пустой словарь
    with open(METADATA_FILE, 'r') as f:
        return json.load(f)

# Сохранение JSON-файла
def save_metadata(metadata):
    with open(METADATA_FILE, 'w') as f:
        json.dump(metadata, f, indent=4)

@app.route('/play/<uid>', methods=['GET'])
def play_playlist(uid):
    metadata = load_metadata()
    if uid in metadata:
        playlist_name = metadata[uid]
        if playlist_name == "unsigned":
            return jsonify({"status": "error", "message": "UID not assigned"}), 404
        try:
            # Очистка текущего плейлиста
            subprocess.run([MPC_COMMAND, "clear"], check=True)
            
            # Загрузка нового плейлиста
            subprocess.run([MPC_COMMAND, "load", playlist_name], check=True)
            
            # Задержка для завершения загрузки плейлиста
            import time
            time.sleep(1)  # Задержка в 1 секунду
            
            # Запуск воспроизведения
            subprocess.run([MPC_COMMAND, "play", "1"], check=True)
            
            # Проверка статуса MPD
            status = subprocess.run([MPC_COMMAND, "status"], capture_output=True, text=True)
            if "playing" in status.stdout:
                return jsonify({"status": "ok", "message": "Playlist loaded and playing"})
            else:
                raise RuntimeError("MPD did not start playback.")

        except subprocess.CalledProcessError as e:
            return jsonify({"status": "error", "message": str(e)}), 500
        except RuntimeError as e:
            return jsonify({"status": "error", "message": str(e)}), 500
    else:
        # Если UID не найден, добавляем его в JSON-файл
        metadata[uid] = "unsigned"
        save_metadata(metadata)
        return jsonify({"status": "error", "message": "UID not assigned", "uid": uid}), 404




# Добавить новые endpoint'ы для управления воспроизведением
@app.route('/control/next', methods=['GET'])
def next_track():
    try:
        subprocess.run([MPC_COMMAND, "next"], check=True)
        status = subprocess.run([MPC_COMMAND, "status"], capture_output=True, text=True)
        return jsonify({"status": "ok", "message": "Next track"})
    except subprocess.CalledProcessError as e:
        return jsonify({"status": "error", "message": str(e)}), 500

@app.route('/control/prev', methods=['GET'])
def prev_track():
    try:
        subprocess.run([MPC_COMMAND, "prev"], check=True)
        status = subprocess.run([MPC_COMMAND, "status"], capture_output=True, text=True)
        return jsonify({"status": "ok", "message": "Previous track"})
    except subprocess.CalledProcessError as e:
        return jsonify({"status": "error", "message": str(e)}), 500

@app.route('/control/toggle', methods=['GET'])
def toggle_playback():
    try:
        subprocess.run([MPC_COMMAND, "toggle"], check=True)
        status = subprocess.run([MPC_COMMAND, "status"], capture_output=True, text=True)
        state = "playing" if "playing" in status.stdout else "paused"
        return jsonify({"status": "ok", "message": f"Playback {state}"})
    except subprocess.CalledProcessError as e:
        return jsonify({"status": "error", "message": str(e)}), 500
        
        
        
        
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
