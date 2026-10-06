#!/bin/bash

# Скрипт автоматизации настройки окружения и сборки Horizon-OC
# Основано на предоставленных инструкциях для DferTune v5.0.0

set -e

echo "--- Запуск настройки окружения для сборки Horizon-OC ---"

# 1. Проверка наличия python3
if ! command -v python3 &> /dev/null; then
    echo "Ошибка: python3 не найден. Пожалуйста, установите Python 3."
    exit 1
fi

# 2. Клонирование необходимых репозиториев
echo "Клонирование Atmosphere..."
git clone https://github.com/Atmosphere-NX/Atmosphere.git --depth 1

echo "Клонирование Horizon-OC..."
git clone https://github.com/Horizon-OC/Horizon-OC.git --recurse-submodules --depth 1

# 3. Подготовка структуры сборки
echo "Подготовка директории сборки..."
mkdir -p Horizon-OC/build

# 4. Копирование файлов Atmosphere в директорию сборки
echo "Интеграция файлов Atmosphere..."
cp -r Atmosphere/* Horizon-OC/build/

# 5. Патчинг исходников (ldr_process_creation.cpp)
# Предполагается, что в DferTune есть папка Source с необходимым патчем
if [ -d "DferTune/source" ]; then
    echo "Применение патча ldr_process_creation.cpp..."
    # Путь в инструкциях: Source/Atmosphere/stratosphere/loader/source/ldr_process_creation.cpp
    # Мы адаптируем это под структуру нашего репозитория, если патч там присутствует
    # В данном случае, мы просто создаем структуру, если патча нет, выводим предупреждение
    PATCH_PATH="DferTune/source/Atmosphere/stratosphere/loader/source/ldr_process_creation.cpp"
    if [ -f "$PATCH_PATH" ]; then
        cp "$PATCH_PATH" Horizon-OC/build/stratosphere/loader/source/ldr_process_creation.cpp
    else
        echo "Предупреждение: Файл патча $PATCH_PATH не найден. Пропуск патчинга."
    fi
fi

# 6. Запуск сборки
echo "Запуск финального скрипта сборки..."
cd Horizon-OC
if [ -f "./build.sh" ]; then
    chmod +x ./build.sh
    ./build.sh
else
    echo "Ошибка: build.sh не найден в репозитории Horizon-OC."
    exit 1
fi

echo "--- Настройка и сборка завершены успешно ---"
