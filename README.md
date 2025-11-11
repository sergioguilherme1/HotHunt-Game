# 🔥 Hot Hunt - Jogo de Caça ao Tesouro com Sockets

**Hot Hunt** é um jogo clássico de caça ao tesouro ("Quente ou Frio") para dois jogadores, implementado em C utilizando `winsock2.h` para comunicação em rede. Este projeto demonstra o gerenciamento de estado e a exclusão mútua através de um protocolo de troca de mensagens síncrono entre um processo Servidor e um Cliente.

![Imagem de um mapa do tesouro 5x5 com pinos de jogador](https://i.imgur.com/gA9mY1j.png)

---

## 🎮 Conceito do Jogo

O jogo é disputado em um tabuleiro $5 \times 5$. Um tesouro é escondido aleatoriamente em uma das 25 casas. O objetivo é ser o primeiro jogador a mover-se para a casa exata onde o tesouro está.

* **Turnos:** O jogo é baseado em turnos alternados.
* **Feedback:** A cada movimento, o jogo informa ao jogador sua proximidade do tesouro.

### A Lógica "Quente ou Frio"

O feedback de proximidade é baseado na "Distância de Manhattan" (passos em X + passos em Y) até o tesouro:

* **QUENTE 🔥:** 1 casa de distância.
* **MORNO 🌡️:** 2 casas de distância.
* **FRIO ❄️:** Mais de 2 casas de distância.

Vence o primeiro jogador a se mover para uma casa com distância 0, encontrando o tesouro.

## 🛠️ Arquitetura Técnica

Este projeto utiliza uma arquitetura Cliente/Servidor alternada para simular o gerenciamento de memória e garantir a exclusão mútua.

* **`server.c` (Jogador 1):** Atua como o "host". Ele sorteia e armazena a localização secreta do tesouro. É a autoridade central que valida as jogadas, calcula o feedback e sincroniza o estado.
* **`client.c` (Jogador 2):** Conecta-se ao servidor. Ambos os processos mantêm uma cópia local do estado do jogo (posições dos jogadores).

### Gerenciamento de Estado e Exclusão Mútua

A exclusão mútua não é feita por `mutex` ou semáforos, mas sim pelo **próprio fluxo de comunicação síncrona** dos *sockets* TCP:

1.  O **estado do jogo** (a "memória") é controlado pela variável `vez`.
2.  Quando é a vez do J1 (Servidor) jogar, o J2 (Cliente) está obrigatoriamente "travado" na função `recv()`, aguardando o novo estado. Ele não pode agir.
3.  Quando o J1 termina, ele envia a mensagem de estado e passa a vez para o J2.
4.  Agora, o J1 fica "travado" no `recv()`, aguardando a ação do J2.

Esse bloqueio alternado garante que apenas um processo possa modificar o estado do jogo por vez.

### Protocolo de Mensagens

A sincronização é feita através de dois tipos principais de mensagens (strings formatadas):

1.  **Mensagem de Ação (Cliente $\to$ Servidor):**
    * Formato: `M[VEZ][DIREÇÃO]`
    * Exemplo: `M2D` (Jogador 2 moveu para Direita)
    * Enviada pelo Cliente para informar sua jogada.

2.  **Mensagem de Estado (Servidor $\to$ Cliente):**
    * Formato: `G[NOVA_VEZ][P1X][P1Y][P2X][P2Y][STATUS]`
    * Exemplo: `G13340F2` (A vez agora é do J1, P1 está em (3,3), P2 está em (4,0), e o status do J2 foi Frio).
    * Enviada pelo Servidor para sincronizar o estado e fornecer o feedback térmico.

## 🚀 Como Compilar e Rodar

Este projeto foi desenvolvido em C para Windows e requer a biblioteca `ws2_32`.

### Pré-requisitos

* Compilador C (como o `gcc` do MinGW)
* Git (opcional, para clonar)

### Compilação (Usando `gcc` no Windows)

Abra seu terminal na pasta do projeto e execute os seguintes comandos:

```bash
# Compilar o Servidor (Jogador 1)
gcc server.c -o server.exe -lws2_32

# Compilar o Cliente (Jogador 2)
gcc client.c -o client.exe -lws2_32
