#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <signal.h>
#include <sys/select.h>

#define BUF_SIZE 1024

// Структура для хранения данных о файле
struct FileData {
    char filename[256]; // Имя файла (максимум 256 символов)
    int filesize; // Размер файла в байтах
};

// Структура для сообщения об успехе/неудаче
struct SuccessMessage {
    bool success; // Флаг успешного завершения операции (true - успех, false - ошибка)
};

// Обработчик сигнала для завершения сервера
void signalHandler(int signum) {
    std::cout << "Получен сигнал " << signum << ". Завершение сервера..." << std::endl;
    exit(0);
}

int main(int argc, char *argv[]) {
    // Определение порта и директории для сохранения файлов 
    // (можете использовать аргументы командной строки)
    if (argc != 3) {
        std::cerr << "Неверный формат запуска: ./server <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]); // Преобразование строки в целое число
    std::string path = argv[2];
    // Установка обработчика сигнала для завершения
    signal(SIGINT, signalHandler); // Обработка сигнала Ctrl+C
    signal(SIGTERM, signalHandler); // Обработка сигнала завершения процесса

    // Создание UDP-сокета
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    // Заполнение структуры адреса
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Обнуление структуры
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Любой доступный адрес
    serverAddr.sin_port = htons(port); // Преобразование порта в сетевой порядок байтов

    // Привязка сокета к адресу
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        return 1;
    }

    std::cout << "Сервер запущен на порту " << port << std::endl;

    fd_set readfds;
    int max_sd;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        max_sd = sockfd;

        // Ждем событий на сокетах
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            std::cerr << "Ошибка select" << std::endl;
            continue;
        }

        // Проверяем, есть ли новые данные на сокете
        if (FD_ISSET(sockfd, &readfds)) {
            // Получение информации о файле от клиента
            struct FileData fileData;
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            if (recvfrom(sockfd, &fileData, sizeof(fileData), 0, (struct sockaddr*)&clientAddr, &addrLen) < 0) {
                std::cerr << "Ошибка получения информации о файле" << std::endl;
                continue;
            }

            // Извлечение имени файла без "jpeg/"
            std::string filename(fileData.filename); 
            size_t pos = filename.find("jpeg/"); // Поиск подстроки "jpeg/"
            if (pos != std::string::npos) {
                filename = filename.substr(pos + 5); // Удаление "jpeg/" из имени файла
            }

            // Создание полного пути к файлу
            std::string fullPath = path + filename;

            // Создание файла для записи
            std::ofstream file(fullPath, std::ios::binary); 
            if (!file.is_open()) {
                std::cerr << "Ошибка создания файла: " << fullPath << std::endl;
                // Отправляем сообщение об ошибке клиенту
                SuccessMessage successMsg;
                successMsg.success = false;
                sendto(sockfd, &successMsg, sizeof(successMsg), 0, (struct sockaddr*)&clientAddr, addrLen);
                continue; // Переход к следующему файлу
            }

            // Получение данных файла от клиента
            char buffer[BUF_SIZE];
            int bytesReceived = 0;
            while (bytesReceived < fileData.filesize) {
                int bytesRead = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen);
                if (bytesRead < 0) {
                    std::cerr << "Ошибка получения данных файла" << std::endl;
                    // Отправляем сообщение об ошибке клиенту
                    SuccessMessage successMsg;
                    successMsg.success = false;
                    sendto(sockfd, &successMsg, sizeof(successMsg), 0, (struct sockaddr*)&clientAddr, addrLen);
                    file.close(); // Закрываем файл, если произошла ошибка
                    continue; 
                }
                file.write(buffer, bytesRead); // Запись полученных данных в файл
                bytesReceived += bytesRead; // Обновление количества полученных байт
            }

            // Проверка целостности
            if (bytesReceived != fileData.filesize) {
                std::cerr << "Ошибка получения файла: получено не все данные" << std::endl;
                // Отправляем сообщение об ошибке клиенту
                SuccessMessage successMsg;
                successMsg.success = false;
                sendto(sockfd, &successMsg, sizeof(successMsg), 0, (struct sockaddr*)&clientAddr, addrLen);
                file.close(); // Закрываем файл, если произошла ошибка
                continue; // Переходим к следующему файлу
            }

            // Закрываем файл
            file.close(); 

            // Отправляем сообщение об успехе клиенту
            SuccessMessage successMsg;
            successMsg.success = true;
            sendto(sockfd, &successMsg, sizeof(successMsg), 0, (struct sockaddr*)&clientAddr, addrLen);
        }
    }

    close(sockfd);
    return 0;
}