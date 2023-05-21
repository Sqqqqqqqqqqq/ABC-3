#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 20

typedef struct {
    int socket;
    char name[MAX_NAME_LENGTH];
} Client;

Client admin_client;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

Client clients[MAX_CLIENTS];
int num_clients = 0;
int current_client = -1;

void *barber_thread(void *arg) {
    while (1) {
        //pthread_mutex_lock(&queue_mutex);

        while (num_clients == 0) {
            printf("I'm sleeping\n");
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        current_client = 0;
        Client client = clients[current_client];
        //printf("Barber is serving client: %s\n", client.name);

        // Симуляция обслуживания клиента
        char cutting_message[] = "Barber is cutting your hair";
        printf("I'm cutting your hair, %s\n", client.name);
        send(client.socket, cutting_message, sizeof(cutting_message), 0);
        sleep(3);

        // Отправка уведомления клиенту о завершении стрижки
        char message[] = "Your haircut is done. Have a nice day!";
        send(client.socket, message, sizeof(message), 0);
	printf("I served the client: %s\n", client.name);
        // Удаление клиента из очереди
        for (int i = 0; i < num_clients - 1; i++) {
            clients[i] = clients[i + 1];
        }
        num_clients--;

        current_client = -1;

        //pthread_mutex_unlock(&queue_mutex);
    }
}

void *client_thread(void *arg) {
    int client_socket = *(int *)arg;

    // Получение имени клиента
    char client_name[MAX_NAME_LENGTH];
    recv(client_socket, client_name, MAX_NAME_LENGTH, 0);

    //pthread_mutex_lock(&queue_mutex);

    if (num_clients < MAX_CLIENTS) {
        // Добавление клиента в очередь
        clients[num_clients].socket = client_socket;
        strcpy(clients[num_clients].name, client_name);
        num_clients++;

        //printf("Client connected: %s\n", client_name);
        
        /*if (num_clients == 1) {
            // Первый клиент - админ, отправка уведомления
            admin_client = clients[current_client];
            
            char message[] = "Admin panel";
            send(client_socket, message, sizeof(message), 0);
	} else*/ if (current_client == -1) {
            // Если парикмахер свободен, текущий клиент становится текущим клиентом парикмахера
            char message[] = "You are in the queue.";
            send(client_socket, message, sizeof(message), 0);
            current_client = 0;
            pthread_cond_signal(&queue_cond);
        } else {
            // Ожидание в очереди
            char message[] = "You are in the queue.";
            send(client_socket, message, sizeof(message), 0);
        }
    } else {
        // Отклонение подключения, если достигнуто максимальное количество клиентов
        char message[] = "The barbershop is full. Try again later.";
        send(client_socket, message, sizeof(message), 0);
        close(client_socket);
    }

    //pthread_mutex_unlock(&queue_mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Создание серверного сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    // Привязка серверного сокета к адресу
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind");
        return 1;
    }

    // Слушаем подключения
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen");
        return 1;
    }

    // Создание потока для парикмахера
    pthread_t barber_tid;
    if (pthread_create(&barber_tid, NULL, barber_thread, NULL) != 0) {
        perror("Failed to create barber thread");
        return 1;
    }

    printf("Barbershop server started. Waiting for connections...\n");

    while (1) {
        // Принятие входящих подключений
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Failed to accept");
            return 1;
        }

        // Создание потока для клиента
        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, client_thread, &client_socket) != 0) {
            perror("Failed to create client thread");
            return 1;
        }

        pthread_detach(client_tid);
    }

    // Закрытие серверного сокета
    close(server_socket);

    return 0;
}

