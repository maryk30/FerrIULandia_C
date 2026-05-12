/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2c de Sistemas Operativos 2025/2026, Enunciado Versão 1+
 **
 ** Aluno: Nº: 141820       Nome: Shreya Mary Kurien
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 **
 **     Initialisation: checks that servidor.pid and pedidos.dat exist, reads the
 **     server PID, traps SIGHUP/SIGINT/SIGALRM, creates a named FIFO (<PID>.fifo),
 **     and connects to the existing IPC message queue using the student IPC key.
 **
 **     Transaction: prompts the user for material, quantity, type (B/S) and customer
 **     name. Sends the request to the server via MSG queue (type MSGTYPE_LOGIN) and
 **     notifies the server with SIGUSR1. Sets a MAX_WAIT alarm.
 **
 **     Response handling: waits for a PROPOSAL message from the dedicated server,
 **     disables the alarm, displays the proposal and asks the user to accept (Y) or
 **     reject (N). Sends the decision back to the dedicated server via MSG. Waits for
 **     TRANSACTION_CONCLUDED before cleaning up and exiting.
 **
 **     Signals: SIGHUP indicates the request was rejected by the server (no free slot).
 **     SIGINT allows the user to cancel at any time. SIGALRM fires if no response
 **     arrives within MAX_WAIT seconds. All three call c5_ShutdownClient which removes
 **     the FIFO before exiting.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

#define PHASE_2A    1                   // If this is defined, then the application will run on Fase 2a
#define PHASE_2B    2                   // If this is defined, then the application will run on Fase 2b
#define PHASE_2C    3                   // If this is defined, then the application will run on Fase 2c
#define PHASE_2D    4                   // If this is defined, then the application will run on Fase 2d
#define CURRENT_TEST_PHASE  PHASE_2C    // Current test Phase

/** GLOBAL VARIABLES **/
Transaction clientRequest;              // the Client request
pid_t serverPid = INVALID;              // the process ID (PID) of the Server process
key_t ipcKey = INVALID;                 // IPC_KEY to be used in all IPC elements
MsgContent msgClientRequest;            // message to be exchanged between Client and Server
int msgId = INVALID;                    // Message IPC Queue (MSG) ID handler

/********************************************************************
 * Template: STUDENTS SHOULD NOT CHANGE ANYTHING IN THESE FUNCTIONS *
 ********************************************************************/
/**
 * @brief  Client process
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 * @return SUCCESS(0) or ERROR (<> 0)
 */
int main() {
    so_debug("<");

    // c1_InitClient:
    c1_1_ValidateServerFiles(FILE_SERVERPID, FILE_REQUESTS);
    FILE *fServerPid = NULL;
    c1_2_OpenFileServerPid(FILE_SERVERPID, &fServerPid);
    c1_3_ReadServerPid(fServerPid, &serverPid);
    #if CURRENT_TEST_PHASE > PHASE_2A
        c1_4_TrapSignalsClient();
        c1_5_CreateClientFifo();
        #if CURRENT_TEST_PHASE > PHASE_2B
            ipcKey = so_GenerateIpcKey();
            c1_6_GetExistingMsgQueue(ipcKey, &msgId);
        #endif
    #endif

    // c2_RegisterTransaction:
    c2_1_RequestTransactionInfo(&clientRequest);
    #if CURRENT_TEST_PHASE < PHASE_2C
        FILE *fRequests = NULL;
        c2_2_OpenFileRequests(FILE_REQUESTS, &fRequests);
        c2_3_WriteRequest(fRequests, clientRequest);
        #if CURRENT_TEST_PHASE > PHASE_2A
            c2_4_AlertServer(serverPid);
        #endif
    #endif
    #if CURRENT_TEST_PHASE > PHASE_2A
        #if CURRENT_TEST_PHASE > PHASE_2B
            c2_5_SendClientRequest(msgId, clientRequest, &msgClientRequest);
        #endif
        c3_ProgramAlarm(MAX_WAIT);
        while (TRUE) {
            // c4_ReceiveResponseFromDedicatedServer:
            #if CURRENT_TEST_PHASE < PHASE_2C
                FILE *fClientFifo;
                c4_1_OpenClientFifo(&fClientFifo);
            #else   // The following is only tested if the Phase is 2c + 2d
                c4_4_ReadServerResponse(msgId, &msgClientRequest);
            #endif
            c4_2_DisableAlarm();
            #if CURRENT_TEST_PHASE < PHASE_2C
                c4_3_ReadResponse(fClientFifo, &clientRequest);
            #else   // The following is only tested if the Phase is 2c + 2d
                c4_5_ProcessServerResponse(msgId, msgClientRequest);
            #endif
            sleep(10);
        }
    #endif
    so_debug(">");
    return SUCCESS;
}

/*************************************
 *             UTILITIES             *
 *************************************/
int so_MsgCreateExclusive(key_t ipcKey) {
    #define ACCESS_PERMS 0600
    int msgId = msgget(ipcKey, IPC_CREAT | IPC_EXCL | ACCESS_PERMS);
    if (msgId > 0)
        so_debug("I created exclusively the MSG{key:0x%x, id:%d}", ipcKey, msgId);
    return msgId;
}

int so_MsgGetExisting(key_t ipcKey) {
    #define IPC_GET 0
    int msgId = msgget(ipcKey, IPC_GET);
    if (msgId > 0)
        so_debug("I'm using the MSG{key:0x%x, id=%d} already created", ipcKey, msgId);
    return msgId;
}

int so_MsgRemove(int msgId) {
    int status = msgctl(msgId, IPC_RMID, NULL);
    if (!status)
        so_debug("I removed the MSG{id:%d}", msgId);
    return status;
}

int so_MsgSend(int msgId, MsgContent *msg, long msgType) {
    msg->msgType = msgType; // THIS IS VERY IMPORTANT!!!
    int status = msgsnd(msgId, msg, sizeof(msg->msgData), 0);
    if (!status)
        so_debug("Sent to MSG{id:%d} a message with {type:%ld, size:%ld}", msgId, msg->msgType, sizeof(msg->msgData));
    return status;
}

int so_MsgReceive(int msgId, MsgContent *msg, long msgType) {
    int nrReceivedBytes = msgrcv(msgId, msg, sizeof(msg->msgData), msgType, 0);
    if (nrReceivedBytes >= 0)
        so_debug("Received from MSG{id:%d} a message with {type:%ld, size:%ld}", msgId, msgType, nrReceivedBytes);
    return nrReceivedBytes;
}

/**
 * @brief  Generates an IPC Key based on the student number (axxxx)
 * @return the generated key, or a default key if the username is not convertible
 */
key_t so_GenerateIpcKey() {
    char *userKey = getenv("USER_IPC_KEY"); // The user may define a variable USER_IPC_KEY (in hexadecimal format)
    if (NULL == userKey)                    // in the file .bashrc or using the command "export USER_IPC_KEY=<key>"
        userKey = getenv("USER");           // Otherwise, this will use the userKey (e.g., "a123456") as IPC key
    key_t ipcKey = so_strtol(userKey, 16);  // Convert string in hexadecimal format to number (long)
    if (!ipcKey && errno) {
        so_error("", "userKey is not convertible");
        ipcKey = 0xa000000;                 // If no key was available, then return a "default IPC key"
        so_debug("Using default IPC Key: 0x%x", ipcKey);
    } else {
        so_debug("IPC Key generated from userKey: 0x%x", ipcKey);
    }
    return ipcKey;
}

/*************************************
 *              FASE 2A              *
 *************************************/
/**
 * @brief  c1_1_ValidateServerFiles Read the description of task C1.1 on the assignment script.
 * @param  filenameServerPid (I) the name of the file that holds the PID of the Server (i.e., FILE_SERVERPID)
 * @param  filenameRequests (I) the name of the file that holds the Client requests (i.e., FILE_REQUESTS)
 */
void c1_1_ValidateServerFiles(char *filenameServerPid, char *filenameRequests) {
    so_debug("< [@param(I) filenameServerPid:%s, filenameRequests:%s]",
             filenameServerPid, filenameRequests);

    // replace

    so_debug(">");
}

/**
 * @brief  c1_2_OpenFileServerPid Read the description of task C1.2 on the assignment script.
 * @param  filenameServerPid (I) the name of the file that holds the PID of the Server (i.e., FILE_SERVERPID)
 * @param  pfServerPid (O) the opened handler of the file that holds the PID of the Server
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
 * @brief  c1_3_ReadServerPid Read the description of task C1.3 on the assignment script.
 * @param  fServerPid (I) the opened handler of the file that holds the PID of the Server
 * @param  pserverPid (O) the Server Process ID (PID)
 */
void c1_3_ReadServerPid(FILE *fServerPid, pid_t *pserverPid) {
    so_debug("< [@param(I) fServerPid:%p]", fServerPid);

    if (fServerPid == NULL) {
        so_error("C1.3", "");
        exit(ERROR);
    }

    if (fscanf(fServerPid, "%d", pserverPid) != 1) {
        so_error("C1.3", "");
        fclose(fServerPid);
        exit(ERROR);
    }

    so_success("C1.3", "%d", *pserverPid);
    fclose(fServerPid);
    so_debug(">");
}

/**
 * @brief  c2_1_RequestTransactionInfo Read the description of task C2.1 on the assignment script.
 * @param  pclientRequest (O) the Client request (the amount field is not filled by the Client)
 */
void c2_1_RequestTransactionInfo(Transaction *pclientRequest) {
    so_debug("<");

    char buffer[256];

    // Helper: check empty or only spaces
    auto int isEmptyOrSpaces(char *s) {
        while (*s) {
            if (!isspace((unsigned char)*s)) return 0;
            s++;
        }
        return 1;
    }

    // Helper: trim leading spaces
    char *trim(char *s) {
        while (isspace((unsigned char)*s)) s++;
        return s;
    }

    // MATERIAL
    printf("Introduza o material: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        so_error("C2.1", "");
        exit(ERROR);
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    if (isEmptyOrSpaces(buffer)) {
        so_error("C2.1", "");
        exit(ERROR);
    }

    strcpy(pclientRequest->material, trim(buffer));

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
    printf("Tipo de transação (B)uy/(S)ell: ");
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

    if (isEmptyOrSpaces(buffer)) {
        so_error("C2.1", "");
        exit(ERROR);
    }

    strcpy(pclientRequest->customerName, trim(buffer));

    pclientRequest->pidCliente = getpid();
    pclientRequest->pidServidorDedicado = -1;

    so_success("C2.1", "%s %d %c %s %d %d",pclientRequest->material,pclientRequest->quantity,pclientRequest->type,pclientRequest->customerName,pclientRequest->pidCliente,pclientRequest->pidServidorDedicado);
    so_debug(">");
}

/**
 * @brief  c2_2_OpenFileRequests Read the description of task C2.2 on the assignment script.
 * @param  filenameRequests (I) the name of the file that holds the Client requests (i.e., FILE_REQUESTS)
 * @param  pfRequests (O) the opened handler of the file that holds the Client requests
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
 * @brief  c2_3_WriteRequest Read the description of task C2.3 on the assignment script.
 * @param  fRequests (I) the opened handler of the file that holds the Client requests
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
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

    so_success("C2.3", "");
    so_debug(">");
}

/*************************************
 *              FASE 2B              *
 *************************************/
/**
 * @brief  c1_4_TrapSignalsClient Read the description of task C1.4 on the assignment script.
 */
void c1_4_TrapSignalsClient() {
    so_debug("<");

    signal(SIGHUP, c6_HandleSighup);   // server -> request failed
    signal(SIGINT, c7_HandleCtrlC);    // CTRL+C
    signal(SIGALRM, c8_HandleAlarm);   // timeout

    so_success("C1.4", "Signals trapped");
    so_debug(">");
}

/**
 * @brief  c1_5_CreateClientFifo Read the description of task C1.5 on the assignment script.
 */
void c1_5_CreateClientFifo() {
    so_debug("<");

    char fifoName[64];
    pid_t pid = getpid();

    snprintf(fifoName, sizeof(fifoName), "%d.fifo", pid);

    // Remove if it already exists
    unlink(fifoName);

    // Create fifo, validate whther cretaed succesfully
    if (mkfifo(fifoName, 0600) < 0) {
        so_error("C1.5", "Error creating FIFO");
        exit(ERROR);
    }

    so_success("C1.5", "%s", fifoName);
    so_debug(">");
}

/**
 * @brief  c3_ProgramAlarm Read the description of task C3 on the assignment script.
 * @param  seconds (I) number of seconds to program the alarm
 */
void c3_ProgramAlarm(int seconds) {
    so_debug("< [@param(I) seconds:%d]", seconds);

    alarm(seconds);

    so_success("C3", "Espera resposta em %d segundos", seconds);
    so_debug(">");
}

/**
 * @brief  c4_1_OpenClientFifo Read the description of task C4.1 on the assignment script.
 * @param  pfClientFifo (O) the opened handler of the Client FIFO
 */
void c4_1_OpenClientFifo(FILE **pfClientFifo) {
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

        so_error("C4.1", "Error opening client FIFO");
        exit(ERROR);
    }

    *pfClientFifo = f;

    so_success("C4.1", "%s", fifoName);
    so_debug("> [@param(O) *pfClientFifo:%p]", *pfClientFifo);
}

/**
 * @brief  c4_2_DisableAlarm Read the description of task C4.2 on the assignment script.
 */
void c4_2_DisableAlarm() {
    so_debug("<");

    alarm(0);  

    so_success("C4.2", "Desliguei alarme");
    so_debug(">");
}

/**
 * @brief  c4_3_ReadResponse Read the description of task C4.3 on the assignment script.
 * @param  fClientFifo (I) the opened handler of the Client FIFO
 * @param  pclientRequest (O) the business proposal from the Dedicated Server
 */
void c4_3_ReadResponse(FILE *fClientFifo, Transaction *pclientRequest) {
    so_debug("< [@param(I) fClientFifo:%p]", fClientFifo);

    if (fClientFifo == NULL || pclientRequest == NULL) {
        so_error("C4.3", "Invalid arguments");
        exit(ERROR);
    }

    size_t n = fread(pclientRequest, sizeof(Transaction), 1, fClientFifo);

    if (n != 1) {
        so_error("C4.3", "Error reading from FIFO");
        fclose(fClientFifo);
        exit(ERROR);
    }

    fclose(fClientFifo);

    printf("Proposta FerrIULândia:\n");
    printf("%s %s %dkg de %s, num valor total de %.2f\n", pclientRequest->customerName, (pclientRequest->type == 'S') ? "vende" : "compra", pclientRequest->quantity, pclientRequest->material, pclientRequest->amount);

    so_success("C4.3", "Li Pedido do FIFO");

    c5_ShutdownClient();

    so_debug("> [@param(O) *pclientRequest:[%s:%d:%c:%s:%f:%d:%d]]", pclientRequest->material, pclientRequest->quantity, pclientRequest->type, pclientRequest->customerName, pclientRequest->amount, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

/**
 * @brief  c5_ShutdownClient Read the description of task C5 on the assignment script.
 */
void c5_ShutdownClient() {
    so_debug("<");

    char fifoName[64];
    pid_t pid = getpid();

    snprintf(fifoName, sizeof(fifoName), "%d.fifo", pid);

    if (unlink(fifoName) < 0) {
        so_error("C5", "Error removing FIFO");
        exit(1);
    }

    so_success("C5", "Cliente terminado");

    so_debug(">");

    exit(0);
}

/**
 * @brief  c6_HandleSighup Read the description of task C6 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void c6_HandleSighup(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGHUP) {
        so_success("C6", "Pedido sem sucesso");

        c5_ShutdownClient(); 
    }

    so_debug(">");
}

/**
 * @brief  c7_HandleCtrlC Read the description of task C7 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void c7_HandleCtrlC(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGINT) {
        so_success("C7", "Cliente: Shutdown");

        c5_ShutdownClient();  // go to shutdown
    }
    so_debug(">");
}

/**
 * @brief  c8_HandleAlarm Read the description of task c8 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void c8_HandleAlarm(int receivedSignal) {
    so_debug("< [@param receivedSignal:%d]", receivedSignal);


    so_error("C8", "Cliente: Timeout");

    c5_ShutdownClient();

    exit(ERROR);            // prevent further FIFO operations

    so_debug(">");
}

/*************************************
 *              FASE 2C              *
 *************************************/
/**
 * @brief  c1_6_GetExistingMsgQueue Read the description of task C1.6 on the assignment script.
 * @param  ipcKey (I) IPC_KEY to be used in the project
 * @param  pmsgId (O) opened IPC Message Queue (MSG) ID handler
 */
void c1_6_GetExistingMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param(I) ipcKey:0x0%x]", ipcKey);

    if (pmsgId == NULL) {
        so_error("C1.6", "Invalid message queue pointer");
        exit(ERROR);
    }

    *pmsgId = so_MsgGetExisting(ipcKey);

    if (*pmsgId == -1) {
        so_error("C1.6", "Failed to access existing message queue");
        exit(ERROR);
    }

    so_success("C1.6", "Opened message queue");
    so_debug("> [@param(O) *pmsgId:%d]", *pmsgId);
}

/**
 * @brief  c2_4_AlertServer Read the description of task C2.3 on the assignment script.
 * @param  serverPid (I) the Server Process ID (PID)
 */
void c2_4_AlertServer(pid_t serverPid) {
    so_debug("< [@param(I) serverPid:%d]", serverPid);

    if (serverPid <= 0) {
        so_error("C2.4", "Invalid server PID");
        exit(ERROR);
    }

    if (kill(serverPid, SIGUSR1) == -1) {
        so_error("C2.4", "Failed to send SIGUSR1 to server");
        exit(ERROR);
    }
    so_success("C2.4", "Alerted server");
    so_debug(">");
}

/**
 * @brief  c2_5_SendClientRequest Read the description of task C2.4 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
 * @param  pmsgClientRequest (O) Client request message to send to the Server to start business
 */
void c2_5_SendClientRequest(int msgId, Transaction clientRequest, MsgContent *pmsgClientRequest) {
    so_debug("< [@param(I) msgId:%d, clientRequest:[%s:%d:%c:%s:%f:%d:%d]]", msgId, clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (pmsgClientRequest == NULL) {
        so_error("C2.5", "NULL message pointer");
        exit(ERROR);
    }

    if (kill(serverPid, SIGUSR1) == -1) {
        so_error("C2.5", "Failed to send SIGUSR1 to server");
        exit(ERROR);
    }

    pmsgClientRequest->msgType = MSGTYPE_LOGIN;
    pmsgClientRequest->msgData.status = 0;
    pmsgClientRequest->msgData.t = clientRequest;

    if (so_MsgSend(msgId, pmsgClientRequest, MSGTYPE_LOGIN) == -1) {
        so_error("C2.5", "Failed to send login message");
        exit(ERROR);
    }

    so_success("C2.5", "Sent login request");
    so_debug("> [@param(O) *pmsgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d]]", pmsgClientRequest->msgType, pmsgClientRequest->msgData.status, pmsgClientRequest->msgData.t.material, pmsgClientRequest->msgData.t.quantity, pmsgClientRequest->msgData.t.type, pmsgClientRequest->msgData.t.customerName, pmsgClientRequest->msgData.t.amount, pmsgClientRequest->msgData.t.pidCliente, pmsgClientRequest->msgData.t.pidServidorDedicado);
}

/**
 * @brief  c4_4_ReadServerResponse Read the description of task C4.4 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  pmsgClientRequest (O) the message sent via MSG by the Dedicated Server
 */
void c4_4_ReadServerResponse(int msgId, MsgContent *pmsgClientRequest) {
    so_debug("< [@param(I) msgId:%d]", msgId);

    if (pmsgClientRequest == NULL) {
        so_error("C4.4", "NULL message buffer");
        exit(ERROR);
    }

    long clientType = (long)getpid();

    while (so_MsgReceive(msgId, pmsgClientRequest, clientType) == -1) {
        if (errno == EINTR) continue;
        so_error("C4.4", "Failed to receive message");
        exit(ERROR);
    }

    so_success("C4.4", "");   // this line was missing
    so_debug(">");
}

/**
 * @brief  c4_5_ProcessServerResponse Read the description of task C4.5 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  msgClientRequest (O) the message sent via MSG by the Dedicated Server
 */
void c4_5_ProcessServerResponse(int msgId, MsgContent msgClientRequest) {
    so_debug("< [@param(I) msgId:%d]", msgId);

    Transaction *t = &msgClientRequest.msgData.t;

    if (msgClientRequest.msgData.status == PROPOSAL) {

        printf("Proposta FerrIULândia: %s %s %dkg de %s, num valor total de %.2f\n\nAceita (Y/N)? ",
               t->customerName,
               (t->type == 'B') ? "compra" : "vende",
               t->quantity,
               t->material,
               t->amount);

        char answer;
        scanf(" %c", &answer);

        msgClientRequest.msgData.status =
            (answer == 'Y' || answer == 'y') ? ACCEPT : REJECT;

        // Send reply to Dedicated Server (msgType = server PID)
        if (so_MsgSend(msgId, &msgClientRequest, (long)t->pidServidorDedicado) == -1) {
            so_error("C4.5", "Failed to send response");
            exit(ERROR);
        }

        so_success("C4.5", "");
    }
    else if (msgClientRequest.msgData.status == TRANSACTION_CONCLUDED) {

        so_success("C4.5", "Negócio terminado");
        c5_ShutdownClient();
    }
    else {
        so_error("C4.5", "Invalid status");
        exit(ERROR);
    }

    so_debug(">");
}