#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 11
#define MAX_PLAYER 10
#define MAX_DATA 6000
#define BUFFER_SIZE 8192

typedef struct PlayerStruct
{
    char name[MAX_LENGTH];
    int x, y, hp, damage;
} Player;

typedef struct MessageStruct
{
    int messageType;
    int dataLength;
    char data[MAX_DATA];
    time_t pubTime;
} Message;

Player* players[MAX_PLAYER] = {NULL, };

int createConnection() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        fprintf(stderr, "%s\n","Failed to create socket");
        return -1;
    }

    // 서버 주소 실정
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "%s\n", "Failed to bind socket");
        close(client_socket);
        return -1;
    }

    return client_socket;
}
void printStatus() {
    for(int i = 0; i < MAX_PLAYER; i++) {
        // if(strcmp(players[i]->name, "EMPTY") != 0) return;

        printf(
            "[%d]\t%s\tx: %d\ty: %d\thp: %d\tdamage: %d\n",
            i,
            players[i]->name,
            players[i]->x,
            players[i]->y,
            players[i]->hp,
            players[i]->damage
        );
    }
}

void sendRequest(int socket, Message* m) {
    char buffer[BUFFER_SIZE] = {0, };

    send(socket, m, sizeof(Message), 0);
    printf("request sent.\n");
    int bytes_received = recv(socket, buffer, sizeof(Message), 0);

    m = (Message*) buffer;

    if(m->messageType != 3) {
        fprintf(stderr, "%s\n", "received invalid response.");
        return;
    }

    Player temp[MAX_PLAYER];
    memcpy(temp, m->data, sizeof(Player) * MAX_PLAYER);

    for(int i = 0; i < MAX_PLAYER; i++) {
        Player* p = (Player*) malloc(sizeof(Player));

        strncpy(p->name, temp[i].name, MAX_LENGTH);
        p->damage = temp[i].damage;
        p->hp = temp[i].hp;
        p->x = temp[i].x;
        p->y = temp[i].y;

        players[i] = p;
    }

    // memcpy(players, m->data, sizeof(Player)* MAX_PLAYER);
    printStatus();
}

int main() {
    // for(int i = 0; i < MAX_PLAYER; i++) {
    //     strncpy(players[i].name = "EMPTY", sizeof("EMPTY"));
    // }
    int socket1 = createConnection();
    if(socket1 == -1) {
        fprintf(stderr, "%s\n", "failed to connect server.");
        return 0;
    }

    Message m;
    m.messageType = 0;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "test_user");
    m.pubTime = 0;
    m.dataLength = strlen("test_user");
    sendRequest(socket1, &m);

    printf("MAKE TEST_USER: %s\n", m.data);

    m.messageType = 1;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "test_user\t1\t2");
    m.pubTime = 0;
    m.dataLength = strlen("test_user\t1\t2");
    sendRequest(socket1, &m);

    printf("MOVE TEST_USER: %s\n", m.data);

    int socket2 = createConnection();
    if(socket2 == -1) {
        fprintf(stderr, "%s\n", "failed to connect server.");
        return 0;
    }

    m.messageType = 0;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "user");
    m.pubTime = 0;
    m.dataLength = strlen("user");
    sendRequest(socket2, &m);

    printf("MAKE USER: %s\n", m.data);

    m.messageType = 1;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "user\t1\t2");
    m.pubTime = 0;
    m.dataLength = strlen("user\t1\t2");
    sendRequest(socket2, &m);

    printf("MOVE USER: %s\n", m.data);

    m.messageType = 2;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "user\ttest_user");
    m.pubTime = 0;
    m.dataLength = strlen("user\ttest_user");
    sendRequest(socket1, &m);

    printf("ATTACK USER -> TEST_USER: %s\n", m.data);

        m.messageType = 1;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "user\t1\t2");
    m.pubTime = 0;
    m.dataLength = strlen("user\t1\t2");
    sendRequest(socket2, &m);

        m.messageType = 1;
    memset(m.data, 0, MAX_DATA);
    strcpy(m.data, "user\t1\t2");
    m.pubTime = 0;
    m.dataLength = strlen("user\t1\t2");
    sendRequest(socket2, &m);

    close(socket1);
    close(socket2);
    return 0;
}
