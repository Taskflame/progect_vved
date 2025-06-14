# Используем официальный образ Ubuntu
FROM ubuntu:22.04

# Устанавливаем зависимости
RUN apt-get update && \
    apt-get install -y g++ make cmake build-essential net-tools iputils-ping && \
    apt-get clean

# Устанавливаем рабочую директорию
WORKDIR /app

# Копируем все файлы проекта
COPY . /app

# Собираем сервер и клиент
RUN g++ -std=c++17 -pthread server.cpp main_server.cpp -o server && \
    g++ -std=c++17 client.cpp -o client

# По умолчанию запускается bash — поведение определяет docker-compose
CMD ["/bin/bash"]