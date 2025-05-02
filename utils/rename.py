import os
import re
from transliterate import translit

def sanitize_name(name):
    """
    Преобразует имя файла или папки:
    - Заменяет пробелы на _
    - Удаляет специальные символы
    - Переводит имя в транслит
    """
    # Переводим имя в транслит
    name = translit(name, 'ru', reversed=True)
    # Заменяем пробелы на _
    name = name.replace(' ', '_')
    # Удаляем все символы, кроме букв, цифр, _, -, и .
    name = re.sub(r'[^a-zA-Z0-9._-]', '', name)
    return name

def rename_files_and_folders(root_dir):
    """
    Рекурсивно переименовывает все файлы и папки в указанной директории
    """
    for root, dirs, files in os.walk(root_dir, topdown=False):
        # Сначала переименовываем файлы
        for file_name in files:
            old_path = os.path.join(root, file_name)
            new_name = sanitize_name(file_name)
            new_path = os.path.join(root, new_name)
            if old_path != new_path:
                os.rename(old_path, new_path)
                print(f'Файл: {old_path} -> {new_path}')

        # Затем переименовываем папки
        for dir_name in dirs:
            old_path = os.path.join(root, dir_name)
            new_name = sanitize_name(dir_name)
            new_path = os.path.join(root, new_name)
            if old_path != new_path:
                os.rename(old_path, new_path)
                print(f'Папка: {old_path} -> {new_path}')

if __name__ == "__main__":
    # Укажите путь к директории, которую нужно обработать
    directory = input("Введите путь к директории: ").strip()
    if os.path.exists(directory):
        rename_files_and_folders(directory)
        print("Переименование завершено!")
    else:
        print("Указанная директория не существует.")