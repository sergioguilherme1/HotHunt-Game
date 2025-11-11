#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <winsock2.h>
#include <conio.h> // Para getch()
#include <ctype.h> // Para toupper

#pragma comment(lib, "ws2_32.lib") 

#define TAB_SIZE 5

// VARIAVEIS GLOBAIS
int pos_P1_x = -1, pos_P1_y = -1;
int pos_P2_x = -1, pos_P2_y = -1;
int vez = 1;

// VARIVEL SECRETA (Apenas no Servidor)
int pos_T_x, pos_T_y;

// FUNCOES DE LOGICA DO JOGO

void iniciar_jogo() {
    srand((unsigned int)time(NULL));
    pos_T_x = rand() % TAB_SIZE;
    pos_T_y = rand() % TAB_SIZE;
    printf("--- Jogo Iniciado ---\n");
    printf("Tesouro em (%d, %d)\n", pos_T_x, pos_T_y);
}

// Calcula a Distancia de Manhattan
int calculaDistancia(int px, int py, int tx, int ty) {
    return abs(px - tx) + abs(py - ty);
}

// Verifica o estado (F, M, Q, V)
char verificaEstado(int jogador) {
    int px, py;
    
    if (jogador == 1) {
        px = pos_P1_x; py = pos_P1_y;
    } else {
        px = pos_P2_x; py = pos_P2_y;
    }

    int dist = calculaDistancia(px, py, pos_T_x, pos_T_y);

    if (dist == 0) return 'V'; // Venceu
    if (dist == 1) return 'Q'; // QUENTE = 1 casa
    if (dist == 2) return 'M'; // MORNO = 2 casas
    return 'F';                // FRIO = Mais de 2 casas
}

void exibir_mapa() {
    system("cls");
    printf("--- HOT HUNT - ENCONTRE O TESOURO ---\n");
    printf("  0 1 2 3 4 (Y)\n");
    printf("  -----------\n");
    
    for (int i = 0; i < TAB_SIZE; i++) {
        printf("%d|", i); // Linha (X)
        for (int j = 0; j < TAB_SIZE; j++) {
            char mark = ' ';
            if (i == pos_P1_x && j == pos_P1_y) {
                mark = '1';
            } else if (i == pos_P2_x && j == pos_P2_y) {
                mark = '2';
            }
            printf("%c|", mark);
        }
        
        if (i == 0) printf(" J1 = Voce");
        if (i == 1) printf(" J2 = Adversario");
        if (i == 2) printf(" QUENTE: 1 casa do tesouro");
        if (i == 3) printf(" MORNO:  2 casas do tesouro");
        if (i == 4) printf(" FRIO:   Mais de 2 casas");
        printf("\n");
    }
    printf("  -----------\n");
    printf("Turno atual: Jogador %d\n", vez);
}

// FUNCOES DE COMUNICACAO E LGICA DE TURNO

// Notacao utilizada: G[NOVA_VEZ][P1X][P1Y][P2X][P2Y][STATUS_RESPOSTA]
// Tamanho fixo
#define MSG_LEN 8 

void serializa_estado(char *buffer, const char *status_msg) {
    sprintf(buffer, "G%d%d%d%d%d%s", 
            vez, 
            pos_P1_x, pos_P1_y, 
            pos_P2_x, pos_P2_y, 
            status_msg);
}

// No server, precisamos deserializar/tirar as informacoes da notacao do status do J2
void deserializa_estado(const char *buffer) {
    int nova_vez, p1x, p1y, p2x, p2y;
    char tipo_msg[2], status_msg[3];
    
    sscanf(buffer, "%1s%1d%1d%1d%1d%1d%2s", 
           tipo_msg, &nova_vez, &p1x, &p1y, &p2x, &p2y, status_msg); 

    vez = nova_vez;
    pos_P1_x = p1x;
    pos_P1_y = p1y;
    pos_P2_x = p2x;
    pos_P2_y = p2y;

    // Imprime o status do movimento anterior do J2
    if (strcmp(status_msg, "Q2") == 0) {
        printf("\nADVERSARIO (J2) ESTA QUENTE!\n");
    } else if (strcmp(status_msg, "M2") == 0) {
        printf("\nADVERSARIO (J2) ESTA MORNO!\n");
    } else if (strcmp(status_msg, "F2") == 0) {
        printf("\nADVERSARIO (J2) ESTA FRIO!\n");
    } else if (strcmp(status_msg, "V2") == 0) {
        printf("\n----------------------------------\n");
        printf("VOCE PERDEU! JOGADOR 2 ENCONTROU O TESOURO!\n");
        printf("----------------------------------\n");
        vez = 0;
    } else if (strcmp(status_msg, "ME") == 0) {
        printf("\nADVERSARIO (J2) TENTOU MOVIMENTO ILEGAL (fora do mapa).\n");
    }
}

// Processa a movimentacao do jogador
int processa_movimento(int jogador, char direcao) {
    int *px = (jogador == 1) ? &pos_P1_x : &pos_P2_x;
    int *py = (jogador == 1) ? &pos_P1_y : &pos_P2_y;
    int novo_x = *px, novo_y = *py;

    switch (direcao) {
        case 'C': novo_x--; break; // Cima
        case 'B': novo_x++; break; // Baixo
        case 'E': novo_y--; break; // Esquerda
        case 'D': novo_y++; break; // Direita
        default: return 0;
    }

    if (novo_x >= 0 && novo_x < TAB_SIZE && novo_y >= 0 && novo_y < TAB_SIZE) {
        *px = novo_x;
        *py = novo_y;
        return 1; // Movimento valido
    }
    return 0; // Movimento invalido
}

int main() {
    iniciar_jogo();

    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        printf("WSAStartup falhou: %d\n", WSAGetLastError());
        return 1;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(51171);
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Erro bind: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
    if (listen(sock, 1) == SOCKET_ERROR) {
        printf("Erro listen: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
    printf("Aguardando conexao do Jogador 2...\n");

    SOCKET clientSocket;
    clientSocket = accept(sock, NULL, NULL);
    printf("Conexao aceita com sucesso\n");


    char sendBuffer[MSG_LEN + 1];
    char recvBuffer[MSG_LEN + 1];
    int bytesReceived;
    
    // SETUP
    exibir_mapa();
    
    // 1. Jogador 1 (Server) escolhe posicao inicial
    while (pos_P1_x == -1) {
        printf("JOGADOR 1 (Voce): Escolha a posicao inicial (x y, 0 a 4): ");
        if (scanf("%d %d", &pos_P1_x, &pos_P1_y) != 2 || 
            pos_P1_x < 0 || pos_P1_x >= TAB_SIZE || 
            pos_P1_y < 0 || pos_P1_y >= TAB_SIZE) {
            printf("Posicao invalida. Tente novamente.\n");
            pos_P1_x = -1; 
            while (getchar() != '\n'); 
        }
    }
    exibir_mapa();
    printf("Aguardando posicao inicial do Jogador 2...\n");
    
    // 2. Recebe a posicao inicial do Jogador 2 (Cliente)
    bytesReceived = recv(clientSocket, recvBuffer, MSG_LEN, 0); 
    if (bytesReceived > 0) {
        recvBuffer[bytesReceived] = '\0';
        if (recvBuffer[0] == 'I' && recvBuffer[1] == '2') {
            pos_P2_x = recvBuffer[2] - '0';
            pos_P2_y = recvBuffer[3] - '0';
        }
    }
    
    // 3. Sincroniza o estado inicial
    serializa_estado(sendBuffer, "OK"); // OK = Setup concluido
    send(clientSocket, sendBuffer, MSG_LEN, 0);
    
    // LOOP PRINCIPAL DO JOGO
    while (vez != 0) {
        exibir_mapa();
        
        if (vez == 1) { // Vez do Jogador 1 (Server)
            char acao;
            char dir_ou_confirma[2];
            int jogada_valida = 0;
            
            while (!jogada_valida) {
                printf("JOGADOR 1 (SUA VEZ) - Acao (M: Mover): ");
                scanf(" %c", &acao);
                acao = toupper(acao);
                
                if (acao == 'M') {
                    printf("Mover (C: Cima, B: Baixo, E: Esquerda, D: Direita): ");
                    scanf(" %1s", dir_ou_confirma);
                    dir_ou_confirma[0] = toupper(dir_ou_confirma[0]);
                    
                    if (processa_movimento(1, dir_ou_confirma[0])) { 
                        jogada_valida = 1;
                        
                        char estado = verificaEstado(1);
                        char status_resp[3];
                        
                        if (estado == 'V') {
                            vez = 0; 
                            strcpy(status_resp, "V1");
                            printf("\nVOCE ENCONTROU O TESOURO!\n");
                        } else {
                            vez = 2; 
                            sprintf(status_resp, "%c1", estado);
                            
                            // Imprime o feedback local
                            if (estado == 'Q') printf("\nVOCE ESTA QUENTE!\n");
                            else if (estado == 'M') printf("\nVOCE ESTA MORNO!\n");
                            else printf("\nVOCE ESTA FRIO!\n");
                        }
                        serializa_estado(sendBuffer, status_resp);
                        getch(); // Pausa para o jogador ler o status (apertando ENTER ele segue)
                        
                    } else {
                        printf("Movimento invalido. Fora do mapa.\n");
                    }
                } else {
                    printf("Acao invalida. Digite 'M'.\n");
                }
            }
            send(clientSocket, sendBuffer, MSG_LEN, 0);
            
        } else { // Vez do Jogador 2 (Cliente) - Aguarda
            printf("Aguardando jogada do adversario (J2)...\n");
            
            bytesReceived = recv(clientSocket, recvBuffer, MSG_LEN, 0);
            
            if (bytesReceived <= 0) {
                 printf("Conexao fechada.\n");
                 vez = 0; 
            } else {
                recvBuffer[bytesReceived] = '\0'; 
                
                char status_resp[3] = "OK";
                int jogada_processada = 0;
                
                // Analisa a AÇÃO recebida (ex: "M2D")
                if (recvBuffer[0] == 'M') {
                    // PROCESSA O MOVIMENTO DO JOGADOR 2
                    if (processa_movimento(2, recvBuffer[2])) {
                        jogada_processada = 1;
                        
                        char estado = verificaEstado(2);
                        
                        if (estado == 'V') {
                            vez = 0; 
                            strcpy(status_resp, "V2"); 
                        } else {
                            vez = 1;
                            sprintf(status_resp, "%c2", estado);
                        }
                    } else {
                        // Movimento Ilegal
                        jogada_processada = 1; 
                        strcpy(status_resp, "ME"); 
                        vez = 1; 
                    }
                } 
                
                if (jogada_processada) {
                    // Envia o ESTADO + FEEDBACK para o Cliente (J2)
                    serializa_estado(sendBuffer, status_resp); 
                    send(clientSocket, sendBuffer, MSG_LEN, 0);
                } else {
                    printf("Erro de protocolo. Recebido: %s\n", recvBuffer);
                }
            }
        }
    }
    
    exibir_mapa();
    closesocket(clientSocket);
    closesocket(sock);
    WSACleanup();
    printf("\nJogo encerrado. Pressione qualquer tecla para sair...\n");
    getch();
    return 0;
}