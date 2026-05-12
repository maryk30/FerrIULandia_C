/****************************************************************************************
 ** Descrição/Explicação do Módulo:
 **     This module implements the Client process for the FerrIULândia recycling platform.
 **     The client begins by validating that the server is running (checking servidor.pid
 **     and pedidos.dat exist), reading the server PID, arming signal handlers, and
 **     creating a named FIFO (<PID>.fifo) to receive the server's response.
 **
 **     The client then prompts the user for transaction details (material, quantity,
 **     transaction type, and customer name), writes the request as a binary Transaction
 **     record to pedidos.dat, and notifies the server via SIGUSR1.
 **
 **     After submitting the request, the client sets a MAX_WAIT second alarm and waits
 **     for a response from a Dedicated Server via its named FIFO. On receiving a response,
 **     it prints the transaction proposal, removes its FIFO, and exits cleanly.
 **
 **     Signal handling covers: SIGHUP to handle server rejection of the request, SIGINT
 **     for user-initiated cancellation, and SIGALRM for timeout if no response is
 **     received within MAX_WAIT seconds. All three signals trigger clean shutdown via
 **     cb5_ShutdownClient, which removes the FIFO before exiting.
 **
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/** GLOBAL VARIABLES **/
Transaction clientRequest;              // Pedido enviado do Cliente para o Servidor
pid_t serverPid;

/**
 * @brief  Processamento do processo Cliente.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main() {
    so_debug("<");

    // c1_InitClient:
    c1_1_ValidateServerFiles(FILE_SERVERPID, FILE_REQUESTS);
    FILE *fServerPid = NULL;
    c1_2_OpenFileServerPid(FILE_SERVERPID, &fServerPid);
    c1_3_ReadServerPid(fServerPid);
    cb1_4_TrapSignalsClient();
    cb1_5_CreateClientFifo();

    // c2_RegisterTransaction:
    c2_1_RequestTransactionInfo(&clientRequest);
    FILE *fRequests = NULL;
    c2_2_OpenFileRequests(FILE_REQUESTS, &fRequests);
    c2_3_WriteRequest(fRequests, clientRequest);

    cb3_ProgramAlarm(MAX_WAIT);

    while (TRUE) {
        // cb4_ReceiveResponseFromDedicatedServer:
        FILE *fClientFifo;
        cb4_1_OpenClientFifo(&fClientFifo);
        cb4_2_DisableAlarm();
        cb4_3_ReadResponse(fClientFifo, &clientRequest);
    }

    so_debug(">");
    return SUCCESS;
}

/**
 * @brief  c1_1_ValidateServerFiles Ler a descrição da tarefa C1.1 no enunciado
 * @param  filenameServerPid (I) O nome do ficheiro que tem o PID do servidor (i.e., FILE_SERVERPID)
 * @param  filenameRequests (I) O nome do ficheiro de pedidos (i.e., FILE_REQUESTS)
 */
void c1_1_ValidateServerFiles(char *filenameServerPid, char *filenameRequests) {
    so_debug("< [@param(I) filenameServerPid:%s, filenameRequests:%s]",
             filenameServerPid, filenameRequests);

    if (filenameServerPid == NULL || filenameRequests == NULL) {
        so_error("C1.1", "Invalid arguments");
        exit(ERROR);
    }

    FILE *fp1 = fopen(filenameServerPid, "r");
    FILE *fp2 = fopen(filenameRequests, "rb");

    if (fp1 == NULL || fp2 == NULL) {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);

        so_error("C1.1", "Invalid number of arguments");
        exit(ERROR);
    }

    fclose(fp1);
    fclose(fp2);

    so_success("C1.1", "");

    so_debug(">");
}

/**
 * @brief  c1_2_OpenFileServerPid Ler a descrição da tarefa C1.2 no enunciado
 * @param  filenameServerPid (I) O nome do ficheiro que vai ter o PID do servidor (i.e., FILE_SERVERPID)
 * @param  pfServerPid (O) descritor aberto do ficheiro que tem o PID do servidor
 */
void c1_2_OpenFileServerPid(char *filenameServerPid, FILE **pfServerPid) {
    so_debug("< [@param(I) filenameServerPid:%s]", filenameServerPid);

    // Open file
    *pfServerPid = fopen(filenameServerPid, "r");

    // Error handling
    if (*pfServerPid == NULL) {
        so_error("C1.2", "");
        exit(ERROR);
    }

    // Success
    so_success("C1.2", "");

    so_debug("> [[@param(O) *pfServerPid:%p]", *pfServerPid);
}

/**
 * @brief  c1_3_ReadServerPid Ler a descrição da tarefa C1.3 no enunciado
 * @param  fServerPid (I) descritor aberto do ficheiro que tem o PID do servidor
 */
void c1_3_ReadServerPid(FILE *fServerPid) {
    so_debug("< [@param(I) fServerPid:%p]", fServerPid);

    int pid;

    // Read PID
    if (fscanf(fServerPid, "%d", &pid) != 1) {
        so_error("C1.3", "");
        fclose(fServerPid);
        exit(ERROR);
    }

    serverPid = pid;

    so_success("C1.3", "%d", pid);

    fclose(fServerPid);

    so_debug(">");
}

/**
 * @brief  cb1_4_TrapSignalsClient Ler a descrição da tarefa CB1.4 no enunciado
 */
void cb1_4_TrapSignalsClient() {
    so_debug("<");

    signal(SIGHUP, cb6_HandleSighup);   // server -> request failed
    signal(SIGINT, cb7_HandleCtrlC);    // CTRL+C
    signal(SIGALRM, cb8_HandleAlarm);   // timeout

    so_success("CB1.4", "Signals trapped");

    so_debug(">");
}

/**
 * @brief  cb1_5_CreateClientFifo Ler a descrição da tarefa CB1.5 no enunciado
 */
void cb1_5_CreateClientFifo() {
    so_debug("<");

    char fifoName[64];
    pid_t pid = getpid();

    snprintf(fifoName, sizeof(fifoName), "%d.fifo", pid);

    // Remove if it already exists
    unlink(fifoName);

    if (mkfifo(fifoName, 0600) < 0) {
        so_error("CB1.5", "Error creating FIFO");
        exit(ERROR);
    }

    so_success("CB1.5", "%s", fifoName);

    so_debug(">");
}

/**
 * @brief  c2_1_RequestTransactionInfo Ler a descrição da tarefa C2.1 no enunciado
 * @param  pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor (o campo amount não é preenchido pelo Cliente)
 */
void c2_1_RequestTransactionInfo(Transaction *pclientRequest) {
    so_debug("<");

    char buffer[256];

    // MATERIAL
    printf("Introduza o material: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    if (strlen(buffer) == 0) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    strcpy(pclientRequest->material, buffer);

    // QUANTITY
    printf("Introduza a quantidade em kg: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        so_error("C2.1", "");
        exit(ERROR);
    }

    int quantity;
    if (sscanf(buffer, "%d", &quantity) != 1) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    pclientRequest->quantity = quantity;

    // TYPE
    printf("Tipo de transação (C)ompra/(V)enda: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        so_error("C2.1", "");
        exit(ERROR);
    }

    char type;
    if (sscanf(buffer, " %c", &type) != 1) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    pclientRequest->type = type;

    // CUSTOMER NAME
    printf("Introduza o nome do cliente: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    if (strlen(buffer) == 0) {
        so_error("C2.1", "");
        exit(ERROR);
    }
    strcpy(pclientRequest->customerName, buffer);

    // OTHER FIELDS
    pclientRequest->pidCliente = getpid();
    pclientRequest->pidServidorDedicado = -1;

    // SUCCESS
    so_success("C2.1", "%s %d %c %s %d %d", pclientRequest->material, pclientRequest->quantity, pclientRequest->type, pclientRequest->customerName, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado
    );

    so_debug("> [@param(O) *pclientRequest:[%s:%d:%c:%s:%f:%d:%d]]", pclientRequest->material, pclientRequest->quantity, pclientRequest->type, pclientRequest->customerName, pclientRequest->amount, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

/**
 * @brief  c2_2_OpenFileRequests Ler a descrição da tarefa C2.2 no enunciado
 * @param  filenameRequests (I) O nome do ficheiro de pedidos (i.e., FILE_REQUESTS)
 * @param  pfRequests (O) descritor aberto do ficheiro de pedidos
 */
void c2_2_OpenFileRequests(char *filenameRequests, FILE **pfRequests) {
    so_debug("< [@param filenameRequests:%s]", filenameRequests);

    if (filenameRequests == NULL || pfRequests == NULL) {
        so_error("C2.2", "Invalid arguments");
        exit(ERROR);
    }

    // Try to open file in append+binary mode (DO NOT overwrite)
    *pfRequests = fopen(filenameRequests, "ab");

    if (*pfRequests == NULL) {
        so_error("C2.2", "Error opening/creating pedidos.dat");
        exit(ERROR);
    }

    so_success("C2.2", "pedidos.dat opened");

    so_debug("> [*pfRequests:%p]", *pfRequests);
}

/**
 * @brief  c2_3_WriteRequest Ler a descrição da tarefa C2.3 no enunciado
 * @param  fRequests (I) descritor aberto do ficheiro de pedidos
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor (o campo amount não é preenchido pelo Cliente)
 */
void c2_3_WriteRequest(FILE *fRequests, Transaction clientRequest) {
    so_debug("< [@param fRequests:%p, clientRequest:[%s:%d:%c:%s:%f:%d:%d]]", fRequests, clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (fRequests == NULL) {
        so_error("C2.3", "Invalid file pointer");
        exit(ERROR);
    }

    size_t written = fwrite(&clientRequest, sizeof(Transaction), 1, fRequests);

    if (written != 1) {
        so_error("C2.3", "Error writing transaction to file");
        exit(ERROR);
    }

    fflush(fRequests);
    kill(serverPid, SIGUSR1);

    so_success("C2.3", "");
    so_debug(">");
} 

/**
 * @brief  cb3_ProgramAlarm Ler a descrição da tarefa CB3 no enunciado
 * @param  seconds (I) número de segundos a programar no alarme
 */
void cb3_ProgramAlarm(int seconds) {
    so_debug("< [@param(I) seconds:%d]", seconds);

    alarm(seconds);

    so_success("CB3", "Espera resposta em %d segundos", seconds);

    so_debug(">");
}

/**
 * @brief  cb4_1_OpenClientFifo Ler a descrição da tarefa CB4.1 no enunciado
 * @param  pfClientFifo (O) descritor aberto do ficheiro do FIFO do Cliente
 */
void cb4_1_OpenClientFifo(FILE **pfClientFifo) {
    so_debug("<");

    char fifoName[64];
    pid_t pid = getpid();

    snprintf(fifoName, sizeof(fifoName), "%d.fifo", pid);

    FILE *f = NULL;

    while (TRUE) {
        f = fopen(fifoName, "rb");

        if (f != NULL) {
            break;  // success
        }

        if (errno == EINTR) {
            continue;
        }

        so_error("CB4.1", "Error opening client FIFO");
        exit(ERROR);
    }

    *pfClientFifo = f;

    so_success("CB4.1", "%s", fifoName);

    so_debug("> [@param(O) *pfClientFifo:%p]", *pfClientFifo);
}

/**
 * @brief  cb4_2_DisableAlarm Ler a descrição da tarefa C4.1 no enunciado
 */
void cb4_2_DisableAlarm() {
    so_debug("<");

    alarm(0);  

    so_success("CB4.2", "Desliguei alarme");

    so_debug(">");
}

/**
 * @brief  cb4_3_ReadResponse Ler a descrição da tarefa CB4.2 no enunciado
 * @param  fClientFifo (I) descritor aberto do ficheiro do FIFO do Cliente
 * @param  pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void cb4_3_ReadResponse(FILE *fClientFifo, Transaction *pclientRequest) {
    so_debug("< [@param(I) fClientFifo:%p]", fClientFifo);

    if (fClientFifo == NULL || pclientRequest == NULL) {
        so_error("CB4.3", "Invalid arguments");
        exit(ERROR);
    }

    size_t n = fread(pclientRequest, sizeof(Transaction), 1, fClientFifo);

    if (n != 1) {
        so_error("CB4.3", "Error reading from FIFO");
        fclose(fClientFifo);
        exit(ERROR);
    }

    fclose(fClientFifo);

    printf("Proposta FerrIULândia:\n");
    printf("%s %s %dkg de %s, num valor total de %.2f\n", pclientRequest->customerName, (pclientRequest->type == 'V') ? "vende" : "compra", pclientRequest->quantity, pclientRequest->material, pclientRequest->amount);

    so_success("CB4.3", "Li Pedido do FIFO");

    cb5_ShutdownClient();

    so_debug("> [@param(O) *pclientRequest:[%s:%d:%c:%s:%f:%d:%d]]", pclientRequest->material, pclientRequest->quantity, pclientRequest->type, pclientRequest->customerName, pclientRequest->amount, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

void cb5_ShutdownClient() {
    so_debug("<");

    char fifoName[64];
    pid_t pid = getpid();

    snprintf(fifoName, sizeof(fifoName), "%d.fifo", pid);

    if (unlink(fifoName) < 0) {
        so_error("CB5", "Error removing FIFO");
        exit(1);
    }

    so_success("CB5", "Cliente terminado");

    so_debug(">");

    exit(0);
}

/**
 * @brief  cb6_HandleSighup Ler a descrição da tarefa CB6 no enunciado
 * @param  receivedSignal (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void cb6_HandleSighup(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGHUP) {
        so_success("CB6", "Pedido sem sucesso");

        cb5_ShutdownClient(); 
    }

    so_debug(">");
}


/**
 * @brief  cb7_HandleCtrlC Ler a descrição da tarefa CB7 no enunciado
 * @param  receivedSignal (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void cb7_HandleCtrlC(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGINT) {
        so_success("CB7", "Cliente: Shutdown");

        cb5_ShutdownClient();  // go to shutdown
    }

    so_debug(">");
}

/**
 * @brief  cb8_HandleAlarm Ler a descrição da tarefa c9 no enunciado
 * @param  receivedSignal (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void cb8_HandleAlarm(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);

    so_error("CB8", "Cliente: Timeout");

    cb5_ShutdownClient();   // IMPORTANT: cleanup step

    exit(ERROR);            // prevent further FIFO operations

    so_debug(">");
}
