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