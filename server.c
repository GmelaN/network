#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <string.h>
#include <memory.h>

#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_LENGTH 11
#define MAX_PLAYER 10
#define MAX_DATA 6000
#define BUFFER_SIZE 8192

typedef struct PlayerStruct
{
    char name[MAX_LENGTH];  // 사용자 이름
    int x, y, hp, damage;   // 좌표, 체력, 데미지 값
} Player;

typedef struct MessageStruct
{
    int messageType;        // 0: NEW, 1: MOVE, 2: ATTACK, 3: UPDATE
    int dataLength;         // 데이터 영역의 크기
    char data[MAX_DATA];    // 데이터 영역
    time_t pubTime;         // 발행 시각
} Message;

static Player* players[MAX_PLAYER] = {NULL, };
static int clients[MAX_PLAYER] = {0, };
static int numPlayers = 0;

void newPlayer(char* name) {
    /*
     * 사용자 이름을 입력받아 새 Player 개체를 반환하는 함수
     */
    Player* p = (Player*) malloc(sizeof(Player));

    if(p == NULL || numPlayers >= MAX_PLAYER) {
        fprintf(stderr, "%s\n", "failed to allocate new player.");
        return;
    }

    p->hp = 500;
    p->damage = 10;
    p->x = 0;
    p->y = 0;
    strncpy(p->name, name, MAX_LENGTH);
    
    if(p == NULL) {
        fprintf(stderr, "%s\n", "failed to allocate new player.");
        return;
    }

    strcpy(p->name, name);

    players[numPlayers++] = p;
    return;
}

Player* getPlayerByName(char* name) {
    /*
     * 사용자 이름을 입력받아 해당 Player 개체를 반환하는 함수
     */
    for(int i = 0; i < numPlayers; i++) {
        if(strcmp(players[i]->name, name) == 0) {
            return players[i];
        }
    }
    return NULL;
}

int getDamageByPlayerName(char* name) {
    /*
     * 사용자 이름을 입력받아 데미지 값을 반환하는 함수
     */
    Player* p = getPlayerByName(name);
    if(p == NULL) return 0;
    return p->damage;
}

void setHpByPlayerName(char* name, int hp) {
    /*
     * 사용자 이름을 입력받아 체력 값을 업데이트하는 함수
     */
    Player* p = getPlayerByName(name);
    if(p == NULL) return;
    p->hp = hp;
    return;
}

int getHpByPlayerName(char* name) {
    /*
     * 사용자 이름을 입력받아 체력 값을 반환하는 함수
     */
    Player* p = getPlayerByName(name);
    if(p == NULL) return 0;
    return p->hp;
}

void updatePosition(char* name, int x, int y) {
    /*
     * 사용자 이름을 입력받아 좌표값을 업데이트하는 함수
     */
    Player* p = getPlayerByName(name);
    if(p == NULL) return;
    p->x = x;
    p->y = y;
    return;
}

void updateStatus() {
    /*
     * 모든 클라이언트에게 업데이트된 정보를 브로드캐스트
     * Message 패킷을 만들고, 상태 정보를 모두 담은 후
     * 모든 클라이언트에게 send
     */
    for(int i = 0; i < numPlayers; i++) {
        Message m;

        Player temp[MAX_PLAYER];
        for(int i = 0; i < numPlayers; i++) {
            memcpy(&temp[i], players[i], sizeof(Player) * MAX_PLAYER);
        }
        for(int i = numPlayers; i < MAX_PLAYER; i++) {
            memset(&temp[i], 0, sizeof(Player));
            strncpy(temp[i].name, "EMPTY", sizeof("EMPTY"));
        }

        memcpy(m.data, temp, sizeof(Player) * MAX_PLAYER);
        // memcpy(m.data, players, sizeof(Player) * MAX_PLAYER);
        m.dataLength = sizeof(Player) * MAX_PLAYER;
        m.messageType = 3;
        m.pubTime = 0;
        send(clients[i], &m, sizeof(Message), 0);

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
    // 모든 클라이언트에게 현재 Players의 상태를 broadcast
}


// 클라이언트 요청 저리 함수
void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    //클라이언트로부터 요청 받기
    int bytes_received = recv(client_socket, buffer, sizeof(Message), 0);
    
    if(bytes_received <= 0) {
        fprintf(stderr, "%s\n","Failed to receive request");
        close(client_socket);
        return;
    }

    Message* message = (Message*) buffer;
    
    // 메시지 파싱
    int type = message->messageType;
    printf("numPlayers: %d\tmessage type is: %d\n", numPlayers, type);
    char name[MAX_LENGTH];
    switch (type) {
        case 0: // 연결 요청
        {
            // 데이터 필드에서 이름 정보를 추출
            strncpy(name, message->data, MAX_LENGTH);

            // 새 Player 객체 생성
            clients[numPlayers] = client_socket;
            newPlayer(name);
            printf("created new user, name: %s\n", players[numPlayers - 1]->name);
            break;
        }
        case 1: // 위치 업데이트
        {
            // 데이터 필드에서 이름 정보를 추출
            char data[MAX_LENGTH + 5];
            char* x_c;
            char* y_c;
            int x, y;
            strncpy(data, message->data, MAX_LENGTH + 5);
            strncpy(name, strtok(data, "\t"), MAX_LENGTH);
            x = atoi(strtok(NULL, "\t"));
            y = atoi(strtok(NULL, "\t"));

            printf("MOVE\t%s\tto x:%d\ty:%d\n", name, x, y);
            updatePosition(name, x, y);
            break;
        }

        case 2: // 행동 명령
        {
            // 데이터 필드에서 이름 정보를 추출
            char names[MAX_LENGTH * 2 + 2], actorName[MAX_LENGTH], victim[MAX_LENGTH];
            strncpy(names, message->data, MAX_LENGTH * 2 + 2);
            strncpy(actorName, strtok(names, "\t"), MAX_LENGTH);
            strncpy(victim, strtok(NULL, "\t"), MAX_LENGTH);

            int damage = getDamageByPlayerName(actorName);

            printf("ATTACK\tattacker:%s\tvictim:%s\n", actorName, victim);
            setHpByPlayerName(victim, getHpByPlayerName(victim) - damage);
            break;
        }
        default:
        {
            fprintf(stderr, "%s\n","Failed to parse message");
            close(client_socket);
            break;
        }
    }

    // 모든 클라이언트에세 변경된 상태를 브로드캐스트
    updateStatus();
}

// 서버 시작 함수
void start_server(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1) {
        fprintf(stderr, "%s\n","Failed to create socket");
        return;
    }

    // 논블로킹 모드 설정
    fcntl(server_socket, F_SETFL, O_NONBLOCK);

    // 서버 주소 구조체 설정
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 소켓과 주소 바인딩
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "%s\n", "Failed to bind socket");
        close(server_socket);
        return;
    }

    // 연결 대기
    if(listen(server_socket, 5) == -1) {
        fprintf(stderr, "%s\n", "Failed to listen");
        close(server_socket);
        return;
    }

    printf("Server listening on port %d...\n", port);

    fd_set readfds;
    int max_sd, sd, activity;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while(1) {
        // 새 클라이언트 패킷 수신 대기와
        // 기존 클라이언트 수신 대기를 위한 select 구문
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        for(int i = 0; i < numPlayers; i++) {
            sd = clients[i];
            if(sd > 0) FD_SET(sd, &readfds);
            if(sd > max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if(activity < 0 && errno != EINTR) {
            // fprintf(stderr, "select error\n");
            continue;
        }

        // 새 클라이언트 연결 처리
        if(FD_ISSET(server_socket, &readfds)) {
            int new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if(new_socket < 0) {
                fprintf(stderr, "accept failed\n");
                continue;
            }

            // 클라이언트 소켓 추가
            clients[numPlayers] = new_socket;
            printf("New connection accepted\n");
            handle_client(new_socket);
            continue;
        }

        // 기존 클라이언트에서 데이터 수신
        for(int i = 0; i < numPlayers; i++) {
            sd = clients[i];

            if(FD_ISSET(sd, &readfds)) {
                handle_client(sd);
            }
        }
    }

    // 종료 시 서버 소켓 닫기
    close(server_socket);
    for(int i = 0; i < numPlayers; i++) close(clients[i]);
}



int main(void) {
    // 8081번 포트에서 서버 시작
    start_server(8081);

    // 종료 시 클라이언트 소켓 닫기
    for(int i = 0; i < numPlayers; i++) close(clients[i]);
    return 0;
}

