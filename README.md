# osi_idz4-Kang-BPE218
## Ваирант 13
## Кан Олеся БПИ 218
### Условие:
В гостинице 10 номеров с ценой 2000 рублей, 10 номеров с ценой 4000 рублей и 5 номеров с ценой 6000 руб. Клиент, зашедший в гостиницу, обладает некоторой (случайной) суммой и получает номер по своим финансовым возможностям, если тот свободен. При наличии денег и разных по стоимости номеров он выбирает случайный номер. Если доступных по цене свободных номеров нет, клиент уходит искать ночлег в другое место. Клиенты порождаются динамически и уничтожаются при освобождении номера или уходе из гостиницы при невозможности оплаты. Создать приложение, моделирующее работу гостиницы. Каждого клиента реализовать в виде отдельного процесса.

Результаты предоставлены в приложенных файлах (на 4-5 - client, server).

### Схема работы:
В начале запускается сервер, затем ожидается запуск и подключение пользователей (для каждого выделяется отдельный поток), которые имеют 4 варианта возможного поведения:
- Получение списка всех свободных комнат, доступных по бюджету
- Бронь свободной комнаты по номеру
- Выселение из комнаты
- Выход (отключение от сервера).

При вызове клиента или сервера через командную строку необходимо передать IP и порт клиенту и IP серверу.



#### Работа на 4-5 
##### Клиент
```c
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
```

##### Сервер
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define R_COUNT 25

pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

struct guest {
    int id;
    int budget;
    int r_num;
};

struct room {
    int id;
    int price;
    bool is_free;
};

struct room rooms[R_COUNT];

void init_rooms() {                                     // Создание списка номеров гостиницы
    for (int i = 0; i < 10; i++) {
        rooms[i].id = i + 1;
        rooms[i].price = 2000;
        rooms[i].is_free = true;
    }
    for (int i = 10; i < 20; i++) {
        rooms[i].id = i + 1;
        rooms[i].price = 4000;
        rooms[i].is_free = true;
    }
    for (int i = 20; i < R_COUNT; i++) {
        rooms[i].id = i + 1;
        rooms[i].price = 6000;
        rooms[i].is_free = true;
    }
}

void init_guest(struct guest *g, int id) {              // Создание слуайного гостя
    srand(time(NULL));
    g->id = id;
    g->budget = rand() % 10000 + 1000;
    g->r_num = 0;
}

void* handle_client(void* socket) {
    int* socket_fd = (int*)socket;
    struct guest g;
    init_guest(&g, *socket_fd);

    printf("Guest %d with budget %d is connected\n", g.id, g.budget);

    while (true) {
        char buffer[1024] = {0};
        // Обработка запросов гостя
        int msg_size = read(*socket_fd, buffer, 1024);
        if (msg_size <= 0) {
            printf("Guest %d disconnected\n", g.id);
            pthread_mutex_unlock(&room_mutex);
            break;
        }
        if (strcmp(buffer, "get_available_rooms ") == 0) {
            // Получение списка доступных номеров
            pthread_mutex_lock(&room_mutex);
            printf("Guest %d is looking for a room...\n", g.id);
            char response[1024] = {0};
            for (int i = 0; i < R_COUNT; i++) {
                if (!rooms[i].is_free) {
                    continue;
                }
                if (g.budget >= rooms[i].price) {
                    char room_str[10];
                    sprintf(room_str, "%d", rooms[i].id);
                    strcat(response, room_str);
                    strcat(response, " ");
                }
            }
            pthread_mutex_unlock(&room_mutex);
            write(*socket_fd, response, sizeof(response));
        } else if (strncmp(buffer, "book_room ", 10) == 0) {
            // Бронирование номера
            pthread_mutex_lock(&room_mutex);
            int room_id = atoi(buffer + 10);
            if ((room_id < 1) || (room_id > R_COUNT)) {
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "invalid_room", sizeof("invalid_room"));
            } else if (!rooms[room_id-1].is_free) {
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "room_not_available", sizeof("room_not_available"));
            } else if (g.budget < rooms[room_id-1].price) {
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "not_enough_money", sizeof("not_enough_money"));
            } else {
                if (g.r_num != 0) {
                    rooms[g.r_num - 1].is_free = true;
                    printf("Guest %d left room %d and booking another\n", g.r_num - 1, room_id);
                }
                rooms[room_id-1].is_free = false;
                g.budget -= rooms[room_id-1].price;
                g.r_num = room_id;
                printf("Guest %d booked room %d\n", g.id, room_id);
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "room_booked", sizeof("room_booked"));
            }
        } else if (strcmp(buffer, "leave_hotel") == 0) {
            // Освобождение номера
            pthread_mutex_lock(&room_mutex);
            int room_id = g.r_num;
            if (room_id == 0) {
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "no_room_booked", sizeof("no_room_booked"));
            } else {
                rooms[room_id-1].is_free = true;
                g.r_num = 0;
                printf("Guest %d left room %d\n", g.id, room_id);
                pthread_mutex_unlock(&room_mutex);
                write(*socket_fd, "room_cleared", sizeof("room_cleared"));
            }
        }
    }
    free(socket);  // deallocate the memory
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    init_rooms();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {               //    // Создание сокета
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {            // Установка опции переиспользования адреса
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса и порта сервера
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Ожидание соединений
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Hotel server is listening on port %d...\n", PORT);

    while (true) {
        // Accepting connection request
        int *new_socket_ptr = malloc(sizeof(int));
        if ((*new_socket_ptr = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            free(new_socket_ptr); // remember to deallocate the memory in case of failure
            exit(EXIT_FAILURE);
        }
        pthread_t pthread;
        pthread_create(&pthread, NULL, handle_client, new_socket_ptr);
    }
    close(server_fd);

    return 0;
}
```

#### Программа в действии:
```
Room 17 was booked by 0 for 4000
Room 18 was booked by 1 for 4000
Room 2 was booked by 2 for 2000
Room 10 was booked by 3 for 2000
Room 1 was booked by 4 for 2000
Room 20 was booked by 5 for 4000
Room 8 was booked by 6 for 2000
Room 15 was booked by 7 for 4000
Room 5 was booked by 8 for 2000
Room 25 was booked by 9 for 6000
Room 9 was booked by 10 for 2000
Room 3 was booked by 11 for 2000
Room 19 was booked by 12 for 4000
13 tried to book a room without money and bursted
3 has left the room 9
Room 10 was booked by 14 for 2000
Room 4 was booked by 15 for 2000
Room 13 was booked by 16 for 4000
7 has left the room 14
8 has left the room 4
Room 5 was booked by 17 for 2000
Room 15 was booked by 18 for 4000
Room 6 was booked by 19 for 2000
Room 21 was booked by 20 for 6000
Room 7 was booked by 21 for 2000
Room 12 was booked by 22 for 4000
23 tried to book a room without money and bursted
24 tried to book a room without money and bursted
Room 22 was booked by 25 for 6000
26 tried to book a room without money and bursted
Room 16 was booked by 27 for 4000
Room 14 was booked by 28 for 4000
Room 23 was booked by 29 for 6000
Room 11 was booked by 30 for 4000
Room 24 was booked by 31 for 6000

Process finished with exit code 0
```
