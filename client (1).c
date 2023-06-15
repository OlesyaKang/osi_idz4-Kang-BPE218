#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_IP "126.0.0.1"
#define PORT 8080

int read() {
    char buf[1024] = {0};
    fgets(buf, 1024, stdin);
    buf[strcspn(buf, "\r\n")] = 0;
    return atoi(buf);
}

void menu() {
    printf("\nWhat do you want to do?\n");
    printf("1. Check available rooms\n");
    printf("2. Book the room\n");
    printf("3. Leave the hotel\n");
    printf("4. Exit\n");
    printf("Write your choice: ");
}

void switch_response(char buf[]) {
    if (strcmp(buf, "invalid_room") == 0) {
        printf("Invalid room number\n");
    } else if (strcmp(buf, "room_not_available") == 0) {
        printf("Room is not available\n");
    } else if (strcmp(buf, "not_enough_money") == 0) {
        printf("Not enough money\n");
    } else if (strcmp(buf, "room_booked") == 0) {
        printf("Room is already booked\n");
    } else if (strcmp(buf, "no_room_booked") == 0) {
        printf("No room is booked\n");
    } else if (strcmp(buf, "room_cleared") == 0) {
        printf("Room is cleared\n");
    } else if (strlen(buf) > 0) {
        printf("Available rooms: %s\n", buf);
    } else {
        printf("No Available rooms\n");
    }
}

void connect_to_server() {
    int s = 0;
    struct sockaddr_in serv_addr;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {                // Создание сокета
        printf("\n Socket creation error \n");
        return;
    }

    // Настройка адреса сервера
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr) <= 0) {          // Преобразование IP адреса из текстового в бинарный формат
        printf("\n Invalid address/ Address not supported \n");
        return;
    }

    if (connect(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {     // Установка соединения с сервером
        printf("\n Connection Failed \n");
        return;
    }

    while (true) {          // Обработка запросов гостя
        char buf[1024] = {0};
        menu();
        int choice = read();
        if (choice == 1) {
            char msg[1024];
            sprintf(msg, "get_available_rooms ");
            send(s, msg, sizeof(msg), 0);
            read(s, buf, 1024);
            switch_response(buf);
        } else if (choice == 2) {
            printf("Enter room number: ");
            int room_number = read();
            char msg[1024];
            sprintf(msg, "book_room %d", room_number);
            send(s, msg, sizeof(msg), 0);
            read(s, buf, 1024);
            switch_response(buf);
        } else if (choice == 3) {
            send(s, "leave_hotel", sizeof("leave_hotel"), 0);
            read(s, buf, 1024);
            switch_response(buf);
        } else if (choice == 4) {
            break;
        } else {
            printf("Invalid choice\n");
        }
    }

    close(s);
}

int main(int argc, char const *argv[]) {
    connect_to_server();
    return 0;
}