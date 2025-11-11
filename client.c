#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <conio.h> // Para getch()
#include <ctype.h> // Para toupper

#pragma comment(lib, "ws2_32.lib")

#define TAB_SIZE 5

// VARIVEIS GLOBAIS
int pos_P1_x = -1, pos_P1_y = -1;
int pos_P2_x = -1, pos_P2_y = -1;
int vez = 1; 


void exibir_mapa() {
    system("cls");
    printf("--- HOT HUNT - ENCONTRE O TESOURO ---\n");
    printf("  0 1 2 3 4 (Y)\n");
    printf("  -----------\n");
    
    for (int i = 0; i < TAB_SIZE; i++) {
        printf("%d|", i); 
        for (int j = 0; j < TAB_SIZE; j++) {
            char mark = ' ';
            if (i == pos_P1_x && j == pos_P1_y) {
                mark = '1';
            } else if (i == pos_P2_x && j == pos_P2_y) {
                mark = '2';
            }
            printf("%c|", mark);
        }
        
        if (i == 0) printf(" J1 = Adversario");
        if (i == 1) printf(" J2 = Voce");
        if (i == 2) printf(" QUENTE: 1 casa do tesouro");
        if (i == 3) printf(" MORNO:  2 casas do tesouro");
        if (i == 4) printf(" FRIO:   Mais de 2 casas");
        printf("\n");
    }
    printf("  -----------\n");
    printf("Turno atual: Jogador %d\n", vez);
}

// FUNCOES DE COMUNICAO

#define MSG_LEN 8 

// A funcao de deserializacao agora imprime o feedback
void deserializa_estado(const char *buffer) {
    int nova_vez, p1x, p1y, p2x, p2y;
    char tipo_msg[2], status_msg[3];
    
    if (sscanf(buffer, "%1s%1d%1d%1d%1d%1d%2s", 
           tipo_msg, &nova_vez, &p1x, &p1y, &p2x, &p2y, status_msg) != 7) { 
        printf("ERRO DE PROTOCOLO: %s\n", buffer);
        return;
    }

    vez = nova_vez;
    pos_P1_x = p1x;
    pos_P1_y = p1y;
    pos_P2_x = p2x;
    pos_P2_y = p2y;
    
    // Interpreta o status do J1 (Adversario)
    if (strcmp(status_msg, "Q1") == 0) {
        printf("\nADVERSARIO (J1) ESTA QUENTE!\n");
    } else if (strcmp(status_msg, "M1") == 0) {
        printf("\nADVERSARIO (J1) ESTA MORNO!\n");
    } else if (strcmp(status_msg, "F1") == 0) {
        printf("\nADVERSARIO (J1) ESTA FRIO!\n");
    
    // Interpreta o status do J2 (Voce)
    } else if (strcmp(status_msg, "Q2") == 0) {
        printf("\nVOCE (J2) ESTA QUENTE!\n");
    } else if (strcmp(status_msg, "M2") == 0) {
        printf("\nVOCE (J2) ESTA MORNO!\n");
    } else if (strcmp(status_msg, "F2") == 0) {
        printf("\nVOCE (J2) ESTA FRIO!\n");

    // Interpreta Fim de Jogo
    } else if (strcmp(status_msg, "V1") == 0) {
        printf("\n----------------------------------\n");
        printf("VOCE PERDEU! JOGADOR 1 ENCONTROU O TESOURO!\n");
        printf("----------------------------------\n");
        vez = 0; 
    } else if (strcmp(status_msg, "V2") == 0) {
        printf("\n----------------------------------\n");
        printf("VOCE GANHOU! ENCONTROU O TESOURO!\n");
        printf("----------------------------------\n");
        vez = 0; 
    
    // Interpreta Erro
    } else if (strcmp(status_msg, "ME") == 0) {
        printf("\nADVERSARIO (J1) TENTOU MOVIMENTO ILEGAL (fora do mapa).\n");
    }
}

// Processa a movimentcao local (apenas para atualizar a interface)
int processa_movimento(char direcao) {
    int *px = &pos_P2_x; 
    int *py = &pos_P2_y;
    int novo_x = *px, novo_y = *py;

    switch (direcao) {
        case 'C': novo_x--; break;
        case 'B': novo_x++; break;
        case 'E': novo_y--; break;
        case 'D': novo_y++; break;
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
    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    }
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(51171);
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Erro ao conectar: %d\n", WSAGetLastError());
        closesocket(clientSocket); WSACleanup(); return 1;
    }
    printf("Conectado ao servidor (Jogador 1)\n");

    char sendBuffer[MSG_LEN + 1];
    char recvBuffer[MSG_LEN + 1];
    int bytesReceived;

    // SETUP 
    exibir_mapa();
    
    // 1. Jogador 2 (Cliente) escolhe posicao inicial
    while (pos_P2_x == -1) {
        printf("JOGADOR 2 (Voce): Escolha a posicao inicial (x y, 0 a 4): ");
        if (scanf("%d %d", &pos_P2_x, &pos_P2_y) != 2 || 
            pos_P2_x < 0 || pos_P2_x >= TAB_SIZE || 
            pos_P2_y < 0 || pos_P2_y >= TAB_SIZE) {
            printf("Posicao invalida. Tente novamente.\n");
            pos_P2_x = -1; 
            while (getchar() != '\n'); 
        }
    }

    // 2. Envia a posicao inicial para o Server
    // notacao: I[2][X][Y] (e.g., I240)
    sprintf(sendBuffer, "I2%d%d", pos_P2_x, pos_P2_y);
    send(clientSocket, sendBuffer, strlen(sendBuffer), 0);
    
    // 3. Recebe o estado inicial sincronizado do Server
    printf("Aguardando inicio do jogo...\n");
    bytesReceived = recv(clientSocket, recvBuffer, MSG_LEN, 0);
    if (bytesReceived > 0) {
        recvBuffer[bytesReceived] = '\0';
        deserializa_estado(recvBuffer); 
    } else {
        printf("Erro ao receber estado inicial.\n");
        vez = 0;
    }

    // LOOP PRINCIPAL DO JOGO
    while (vez != 0) {
        exibir_mapa();
        
        if (vez == 2) { // Vez do Jogador 2 (Cliente)
            char acao;
            char dir_ou_confirma[2];
            int jogada_valida = 0;
            
            while (!jogada_valida) {
                printf("JOGADOR 2 (SUA VEZ) - Acao (M: Mover): ");
                scanf(" %c", &acao);
                acao = toupper(acao);
                
                if (acao == 'M') {
                    printf("Mover (C: Cima, B: Baixo, E: Esquerda, D: Direita): ");
                    scanf(" %1s", dir_ou_confirma);
                    dir_ou_confirma[0] = toupper(dir_ou_confirma[0]);
                    
                    if (processa_movimento(dir_ou_confirma[0])) {
                        jogada_valida = 1;
                        sprintf(sendBuffer, "M2%c", dir_ou_confirma[0]);
                    } else {
                        printf("Movimento invalido. Fora do mapa.\n");
                    }
                } else {
                    printf("Acao invalida. Digite 'M'.\n");
                }
            }
            
            // Envia a acao para o Servidor (J1)
            send(clientSocket, sendBuffer, strlen(sendBuffer), 0);
            
            // Aguarda a resposta de sincronizao (com o feedback)
            printf("Aguardando status do movimento...\n");
            bytesReceived = recv(clientSocket, recvBuffer, MSG_LEN, 0);
            if (bytesReceived > 0) {
                recvBuffer[bytesReceived] = '\0';
                deserializa_estado(recvBuffer); // Atualiza e imprime o status
                getch(); // Pausa para o jogador ler o status (enter retoma)
            } else {
                printf("Conexao fechada pelo servidor.\n");
                vez = 0;
            }
            
        } else { // Vez do Jogador 1 (Server) - Aguarda
            printf("Aguardando jogada do adversario (J1)...\n");
            
            bytesReceived = recv(clientSocket, recvBuffer, MSG_LEN, 0);
            if (bytesReceived <= 0) {
                 printf("Conexao fechada.\n");
                 vez = 0;
            } else {
                recvBuffer[bytesReceived] = '\0';
                deserializa_estado(recvBuffer); // Atualiza e imprime o status do J1
                getch(); // Pausa para o jogador ler o status(enter retoam)
            }
        }
    }
    
    exibir_mapa();
    closesocket(clientSocket);
    WSACleanup();
    printf("\nJogo encerrado. Pressione qualquer tecla para sair...\n");
    getch();
    return 0;
}