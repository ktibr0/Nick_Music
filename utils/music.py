from flask import Flask, jsonify, send_file, request
import os
import base64
from transliterate import translit
app = Flask(__name__)

# Корневая папка для музыки
MUSIC_FOLDER = "/mnt/usb/music"
RFID_MAPPING_FILE = "rfid_mapping.json"

# Загрузка ассоциаций RFID с папками
try:
    import json
    with open(RFID_MAPPING_FILE, 'r') as f:
        rfid_mapping = json.load(f)
except FileNotFoundError:
    rfid_mapping = {}



# API: Получение списка файлов в папке
@app.route('/files/<rfid>', methods=['GET'])
def get_files(rfid):
    folder = rfid_mapping.get(rfid)
    if not folder:
        return jsonify({"error": "RFID not associated"}), 404
    folder_path = os.path.join(MUSIC_FOLDER, folder)
    if not os.path.exists(folder_path):
        return jsonify({"error": "Folder not found"}), 404
    files = [f for f in os.listdir(folder_path) if f.endswith('.mp3')]
    return jsonify(files)
    
    
    
    # API: Получение MP3-файла
@app.route('/file/<rfid>/<filename>', methods=['GET'])
def get_file(rfid, filename):
    folder = rfid_mapping.get(rfid)
    if not folder:
        return jsonify({"error": "RFID not associated"}), 404
    file_path = os.path.join(MUSIC_FOLDER, folder, filename)
    if not os.path.exists(file_path):
        return jsonify({"error": "File not found"}), 404
    return send_file(file_path, as_attachment=True)

# API: Привязка RFID к папке
@app.route('/associate', methods=['POST'])
def associate_rfid():
    data = request.json
    rfid = data.get('rfid')
    folder = data.get('folder')
    if not rfid or not folder:
        return jsonify({"error": "Invalid data"}), 400
    rfid_mapping[rfid] = folder
    with open(RFID_MAPPING_FILE, 'w') as f:
        json.dump(rfid_mapping, f)
    return jsonify({"message": "RFID associated"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
