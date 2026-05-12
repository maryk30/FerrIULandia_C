/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2c de Sistemas Operativos 2025/2026, Enunciado Versão 1+
 **
 ** Aluno: Nº: 141820       Nome: Shreya Mary Kurien
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **      Initialisation: validates the command line argument (maxCapacity), creates
 **     servidor.pid, allocates and initialises the requests database (MAX_REQUESTS
 **     slots), traps SIGUSR1/SIGINT/SIGCHLD, and creates an IPC message queue using
 **     the student IPC key (removing any existing queue first).
 **
 **     Main loop: on each iteration, reloads daily stock limits from materiais.txt if
 **     the date has changed, then blocks on the MSG queue waiting for a client login
 **     request (type MSGTYPE_LOGIN). Each valid request is processed: a free slot is
 **     reserved in the requests database, stock and amount are calculated, the database
 **     entry is updated, and a Dedicated Server process is forked.
 **
 **     Dedicated Server: after a random sleep (1 to MAX_WAIT seconds), sends a PROPOSAL
 **     message to the client via MSG (type = client PID). Waits for the client response
 **     (type = SD PID). If accepted, logs the transaction to transactions.dat. If
 **     rejected, reverts the stock change. Sends TRANSACTION_CONCLUDED to the client,
 **     frees its database slot, and exits.
 **
 **     Signals: SIGUSR1 wakes the server immediately when a client sends a request.
 **     SIGINT triggers graceful shutdown: sends SIGUSR2 to all active dedicated servers,
 **     removes servidor.pid and the MSG queue, then exits. SIGCHLD reaps finished
 **     dedicated server children and clears their slots in the requests database.
 **     SIGUSR2 in dedicated servers sets a flag checked after sleep to trigger early
 **     shutdown.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"
#include "utils.h"

#define PHASE_2A    1                   // If this is defined, then the application will run on Fase 2a
#define PHASE_2B    2                   // If this is defined, then the application will run on Fase 2b
#define PHASE_2C    3                   // If this is defined, then the application will run on Fase 2c
#define PHASE_2D    4                   // If this is defined, then the application will run on Fase 2d
#define CURRENT_TEST_PHASE  PHASE_2A    // Current test Phase

/** GLOBAL VARIABLES **/
int maxCapacity;                        // the value of the maximum capacity of the warehouse, as specified by the user
StockItem *stocks = NULL;               // the array of dairy material stocks
int sizeStocks = 0;                     // the number of elements of the array of dairy material stocks
Transaction clientRequest;              // the Client request
SessionRequest *requests = NULL;        // the array of requests to be handled by the application (with fixed size MAX_REQUESTS)
int indexClientDB;                      // current index of the array of requests to be handled by the application
FILE *fRequests = NULL;                 // handler of the file that holds the Client requests
key_t ipcKey = INVALID;                 // IPC_KEY to be used in all IPC elements
MsgContent msgClientRequest;            // message to be exchanged between Client and Server
int msgId = INVALID;                    // Message IPC Queue (MSG) ID handler
volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sdb_shutdown_requested = 0;

/********************************************************************
 * Template: STUDENTS SHOULD NOT CHANGE ANYTHING IN THESE FUNCTIONS *
 ********************************************************************/
/**
 * @brief  Server process
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 * @param  argc (I) number of Strings of the array argv
 * @param  argv (I) array of Strings with the arguments passed as commandline when invoking the program
 * @return SUCCESS(0) or ERROR (<> 0)
 */
int main(int argc, char *argv[]) {
    so_debug("< [@param(I) argc:%d, argv:%p]", argc, argv);
    s1_InitServer(argc, argv, &maxCapacity);
    s2_MainServer();
    so_error("Servidor", "Your program should NEVER have reached this point!");
    so_debug(">");
    return ERROR;
}

/**
 * @brief  s1_InitServer Read the description of task S1 on the assignment script.
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 * @param  argc (I) number of Strings of the array argv
 * @param  argv (I) array of Strings with the arguments passed as commandline when invoking the program
 * @param  pmaxCapacity (O) the value of the maximum capacity of the warehouse, as specified by the user
 */
void s1_InitServer(int argc, char *argv[], int *pmaxCapacity) {
    so_debug("< [@param(I) argc:%d, argv:%p]", argc, argv);
    s1_1_GetMaxCapacity(argc, argv, pmaxCapacity);
    FILE *fServerPid = NULL;
    s1_2_CreateFileServerPid(FILE_SERVERPID, &fServerPid);
    s1_3_WriteServerPid(fServerPid);
    #if CURRENT_TEST_PHASE > PHASE_2A
        s1_4_CreateRequestsDB(&requests, MAX_REQUESTS);
        s1_5_TrapSignalsServer();
        #if CURRENT_TEST_PHASE > PHASE_2B
            ipcKey = so_GenerateIpcKey();
            s1_6_CreateMsgQueue(ipcKey, &msgId);
        #endif
    #endif
    so_debug("> [@param(O) *pmaxCapacity:%d]", *pmaxCapacity);
}

/**
 * @brief  s2_MainServer Read the description of task S2 on the assignment script.
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 */
void s2_MainServer() {
    so_debug("<");
    int dayLastIteration = -1;  // This holds the day of the month of the last iteration
    while (TRUE) {              // Ciclo 1
        s2_1_InitDailyStocks(&dayLastIteration, &stocks, &sizeStocks);
        #if CURRENT_TEST_PHASE < PHASE_2C
            s2_2_OpenFileRequests(FILE_REQUESTS, &fRequests);
            s2_3_ReadRequests(fRequests, stocks, sizeStocks);
        #endif
        #if CURRENT_TEST_PHASE > PHASE_2B
            s2_4_ReadClientRequest(msgId, &msgClientRequest);
            if (s2_3_2_IsValidRequest(msgClientRequest.msgData.t))
                s2_3_3_ProcessRequest(msgClientRequest.msgData.t, stocks, sizeStocks, requests, MAX_REQUESTS);
        #endif
        sleep(10);              // Sleep for 10 seconds before doing the processing again
    }
    so_debug(">");
}

/**
 * @brief  s2_1_InitDailyStocks Read the description of task S2.1 on the assignment script.
 * @param  pdayLastIteration (I/O) the date (day of month) of the last 10 min iteration
 * @param  pstocks (O) the new array of dairy material stocks, in case the date has changed
 * @param  psizeStocks (O) the new number of elements of the new array of dairy material stocks, in case the date has changed
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION
 */
void s2_1_InitDailyStocks(int *pdayLastIteration, StockItem **pstocks, int *psizeStocks) {
    so_debug("< [@param(I) *pdayLastIteration:%d]", *pdayLastIteration);
    if (s2_1_HasDateChangedSinceLastIteration(pdayLastIteration)) {
        FILE *fMaterials = NULL;
        s2_1_1_OpenFileMaterials(FILE_MATERIALS, &fMaterials);
        s2_1_2_ValidateMaxCapacity(fMaterials, maxCapacity, psizeStocks);
        s2_1_3_CreateStocks(sizeStocks, pstocks);
        s2_1_4_PopulateStocks(fMaterials, *pstocks, *psizeStocks);
    }
    so_debug("> [@param(O) *pdayLastIteration:%d, *pstocks:%p, *psizeStocks:%d]", *pdayLastIteration, *pstocks, *psizeStocks);
}

/**
 * @brief  s2_3_ReadRequests Read the description of task S2.3 on the assignment script.
 * @param  fRequests (I) the opened handler of the file that holds the Client requests
 * @param  stocks (I) the array of dairy material stocks
 * @param  sizeStocks (I) the number of elements of the array of dairy material stocks
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION
 */
void s2_3_ReadRequests(FILE *fRequests, StockItem *stocks, int sizeStocks) {
    so_debug("< [@param(I) fRequests:%p, stocks:%p, sizeStocks:%d]", fRequests, stocks, sizeStocks);
    while (s2_3_1_ReadRequest(fRequests, &clientRequest))   // Ciclo 2
        if (s2_3_2_IsValidRequest(clientRequest))
            s2_3_3_ProcessRequest(clientRequest, stocks, sizeStocks, requests, MAX_REQUESTS);
    s2_3_4_CleanRequests(fRequests, FILE_REQUESTS);
    so_debug(">");
}

/**
 * @brief  s2_3_3_ProcessRequest Read the description of task S2.3.3 on the assignment script.
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
 * @param  stocks (I) the array of dairy material stocks
 * @param  sizeStocks (I) the number of elements of the array of dairy material stocks
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION
 */
void s2_3_3_ProcessRequest(Transaction clientRequest, StockItem *stocks, int sizeStocks, SessionRequest *requests, int sizeRequests) {
    so_debug("< [@param(I) clientRequest:[%s:%d:%c:%s:%f:%d:%d], stocks:%p, sizeStocks:%d, requests:%p, sizeRequests:%d]", clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado, stocks, sizeStocks, requests, sizeRequests);
    #if CURRENT_TEST_PHASE == PHASE_2A
        int quantity;
        float amount;
        s2_3_3_2_ProcessRequest(clientRequest, stocks, sizeStocks, &quantity, &amount);
    #else
        indexClientDB = s2_3_3_1_GetClientDBEntry(clientRequest, requests, sizeRequests);
        if (ERROR != indexClientDB) {
            int quantity;
            float amount;
            s2_3_3_2_ProcessRequest(clientRequest, stocks, sizeStocks, &quantity, &amount);
            s2_3_3_3_UpdateClientDB(quantity, amount, requests, indexClientDB);
            s2_3_3_4_CreateDedicatedServer(requests, indexClientDB);
        }
    #endif
    so_debug(">");
}

/**
 * @brief  s5_ShutdownServer Read the description of task S5 on the assignment script.
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 */
void s5_ShutdownServer() {
    so_debug("<");
    s5_1_RemoveFileServerPid(FILE_SERVERPID);
    s5_2_TerminateAllDedicatedServers(requests, MAX_REQUESTS);
    s5_4_RemoveMsgQueue(msgId);
    exit(SUCCESS);
    so_debug(">");
}

/**
 * @brief  sd8_MainDedicatedServer Read the description of task SD8 on the assignment script.
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 */
void sd8_MainDedicatedServer() {
    so_debug("<");
    // sd8_InitDedicatedServer:
    #if CURRENT_TEST_PHASE < PHASE_2C
        sd8_1_ValidateClientFifo(clientRequest);             // Removed on Fase 2c
    #endif
    sd8_2_TrapSignalsDedicatedServer();
    // sd9_ClientProcess:
    sd9_1_SleepRandomTime(1, MAX_WAIT);
    #if CURRENT_TEST_PHASE < PHASE_2C
        FILE *fClientFifo;                                   // Removed on Fase 2c
        sd9_2_OpenClientFifo(clientRequest, &fClientFifo);   // Removed on Fase 2c
        sd9_3_WriteResponse(fClientFifo, clientRequest);     // Removed on Fase 2c
    #else
        sd9_4_SendProposal(msgId, requests, indexClientDB);
        sd9_5_ReceiveClientApproval(msgId, &msgClientRequest);
        sd9_6_CheckResponseAccepted(msgClientRequest, FILE_LOGFILE);
        sd9_7_CheckResponseRejected(msgClientRequest, stocks, sizeStocks);
    #endif
    sd10_ShutdownDedicatedServer();
    so_error("Servidor Dedicado", "Your program should NEVER have reached this point!");
    so_debug(">");
}

/**
 * @brief  sd10_ShutdownDedicatedServer Read the description of task SD10 on the assignment script.
 *         STUDENTS SHOULD NOT CHANGE ANYTHING IN THIS FUNCTION.
 */
void sd10_ShutdownDedicatedServer() {
    so_debug("<");
    sd10_1_FreeClientDBEntry(requests, indexClientDB);
    #if CURRENT_TEST_PHASE > PHASE_2B
        sd10_2_SendTransactionConcluded(msgId, msgClientRequest);
    #endif
    so_debug(">");
    exit(SUCCESS);
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
 * @brief  s1_1_GetMaxCapacity Read the description of task S1.1 on the assignment script.
 * @param  argc (I) number of Strings of the array argv
 * @param  argv (I) array of Strings with the arguments passed as commandline when invoking the program
 * @param  pmaxCapacity (O) the value of the maximum capacity of the warehouse, as specified by the user
 */
void s1_1_GetMaxCapacity(int argc, char *argv[], int *pmaxCapacity) {
    so_debug("< [@param(I) argc:%d, argv:%p]", argc, argv);

    // Validate number of arguments
    if(argc != 2){
        so_error("S1.1","Incorrect number of arguments");
        exit(ERROR);
    }

    // Read and validate maxCapacity
    int maxCapacity = atoi(argv[1]);

    if(maxCapacity <= 0){
        so_error("S1.1", "maxCapacity cannot be a negative number");
        exit(ERROR);
    }

    // Update pointer
    *pmaxCapacity = maxCapacity;

    so_success("S1.1", "%d", *pmaxCapacity);
    so_debug("> [@param(O) *pmaxCapacity:%d]", *pmaxCapacity);
}

/**
 * @brief  s1_2_CreateFileServerPid Read the description of task S1.2 on the assignment script.
 * @param  filenameServerPid (I) the name of the file that holds the PID of the Server (i.e., FILE_SERVERPID)
 * @param  pfServerPid (O) the opened handler of the file that holds the PID of the Server
 */
void s1_2_CreateFileServerPid(char *filenameServerPid, FILE **pfServerPid) {
    so_debug("< [@param(I) filenameServerPid:%s]", filenameServerPid);

    // Create servidor.pid. If exists, overwrite
    *pfServerPid = fopen(filenameServerPid, "w");

    if (*pfServerPid == NULL){
        so_error("1.2","Error creating file. Exiting...");
        exit(ERROR);
    }

    so_success("S1.2", "File servidor.pid created");
    so_debug("> [[@param(O) *pfServerPid:%p]", *pfServerPid);
}

/**
 * @brief  s1_3_WriteServerPid Read the description of task S1.3 on the assignment script.
 * @param  fServerPid (I) the opened handler of the file that holds the PID of the Server
 */
void s1_3_WriteServerPid(FILE *fServerPid) {
    so_debug("< [[@param(I) fServerPid:%p]", fServerPid);

    // Validate file pointer 
    if(fServerPid == NULL){
        so_error("S1.3", "Error opening file servidro.pid");
        exit(ERROR);
    }

    // Obtain pid
    pid_t pid = getpid();

    // Write pid to servidor.pid. Validate success.
    if(fprintf(fServerPid, "%d\n", pid) < 0){
        so_error("S1.3","Error printing pid to file");
        fclose(fServerPid);
        exit(ERROR);
    }

    so_success("S1.3", "%d", pid);
    so_debug(">");
}

/**
 * @brief  s2_1_HasDateChangedSinceLastIteration Read the description of task S2.1 on the assignment script.
 *         NOTE: This function does not raise so_success() nor so_error()
 * @param  pdayLastIteration (I/O) the date (day of month) of the last 10 min iteration
 *         If the current date is different than *pdayLastIteration,
 *         update this value to the current date (day of month of the current date).
 * @return TRUE the current date is different than *pdayLastIteration
 */
int s2_1_HasDateChangedSinceLastIteration(int *pdayLastIteration) {
    so_debug("< [@param(I) *pdayLastIteration:%d]", *pdayLastIteration);

    int return_value = FALSE;

    // Obtain date (day of month)
    time_t now = time(NULL);
    struct tm *current = localtime(&now);

    int today = current->tm_mday;

    // If last run was not today, update and return
    if (*pdayLastIteration != today){
        *pdayLastIteration = today;
        return_value = TRUE;
    }

    so_debug("> [@return return_value:%d], [@param(O) *pdayLastIteration:%d]", return_value, *pdayLastIteration);
    return return_value;
}

/**
 * @brief  s2_1_1_OpenFileMaterials Read the description of task S2.1.1 on the assignment script.
 * @param  filenameMaterials (I) the name of the file that holds the materials (i.e., FILE_MATERIALS)
 * @param  pfMaterials (O) the opened handler of the file that holds the materials
 */
void s2_1_1_OpenFileMaterials(char *filenameMaterials, FILE **pfMaterials) {
    so_debug("< [@param(I) filenameMaterials:%s]", filenameMaterials);

    *pfMaterials = fopen(filenameMaterials, "r");

    if(*pfMaterials == NULL){
        so_error("S2.1.1", "Error opening file materiais.txt");
        exit(ERROR);
    }

    so_success("S2.1.1", "File opened");
    so_debug("> [[@param(O) *pfMaterials:%p]", *pfMaterials);
}

/**
 * @brief  s2_1_2_ValidateMaxCapacity Read the description of task S2.1.2 on the assignment script.
 * @param  fMaterials (I) the opened handler of the file that holds the materials
 * @param  pmaxCapacity (I) the value of the maximum capacity of the warehouse, as specified by the user
 * @param  psizeStocks (O) the number of (lines in the file that holds the) materials, which will correspond to the number of elements of dairy material stocks array
 */
void s2_1_2_ValidateMaxCapacity(FILE *fMaterials, int pmaxCapacity, int *psizeStocks) {
    so_debug("< [@param(I) fMaterials:%p, pmaxCapacity:%i]", fMaterials, pmaxCapacity);

    // Ensure file exists
    if(fMaterials == NULL){
        so_error("S2.1.2", "Error opening materials file");
        exit(ERROR);
    }

    // initialise variables
    char line[256];
    int total;
    int count;

    // Moves file pointer to beginning of file
    rewind(fMaterials);

    while(fgets(line, sizeof(line), fMaterials)){
        line[strcspn(line, "\n")] = '\0';
        if(line[0] == 0) continue;

        char name[100];
        int price_per_kg;
        int daily_limit;

        //updates variables from materiais.txt
        if(sscanf(line, "%99[^;];%d;%d", name, &price_per_kg, &daily_limit) == 3){
            total += daily_limit;
            count++;
        }
    }

    if(ferror(fMaterials)){
        so_error("S2.1.2", "Error reading materials file");
        exit(ERROR);
    }

    // Validate total does not exceed maxCapacity
    if(total > pmaxCapacity){
        so_error("S2.1.2", "Total daily limit exceeds maxCapacity");
        exit(ERROR);
    }

    *psizeStocks = count;

    so_success("S2.1.2", "%d", total);
    so_debug("> [@param(O) *psizeStocks:%d]", *psizeStocks);
}

/**
 * @brief  s2_1_3_CreateStocks Read the description of task S2.1.3 on the assignment script.
 * @param  sizeStocks (I) the number of (lines in the file that holds the) materials, which will correspond to the number of elements of dairy material stocks array
 * @param  pstocks (I/O) the old/new array of dairy material stocks
 */
void s2_1_3_CreateStocks(int sizeStocks, StockItem **pstocks) {
    so_debug("< [@param(I) sizeStocks:%i, *pstocks:%p]", sizeStocks, *pstocks);

    // free previous allocation if exists
    if(*pstocks != NULL){
        free(*pstocks);
        *pstocks = NULL;
    }

    // Create array
    *pstocks = (StockItem *) malloc(sizeof(StockItem) * sizeof(sizeStocks));

    // Validate creation
    if(*pstocks == NULL){
        so_error("S2.1.3", "Error creating array");
        exit(ERROR);
    }

    so_success("S2.1.3", "Stocks created");
    so_debug("> [[@param(O) *pstocks:%p]", *pstocks);
}

/**
 * @brief  s2_1_4_PopulateStocks Read the description of task S2.1.4 on the assignment script.
 * @param  fMaterials (I) the opened handler of the file that holds the materials
 * @param  stocks (I) the array of dairy material stocks
 * @param  sizeStocks (I) the number of elements of the array of dairy material stocks
 */
void s2_1_4_PopulateStocks(FILE *fMaterials, StockItem *pstocksArg, int sizeStocksArg) {
    so_debug("< [@param(I) fMaterials:%p, pstocksArg:%p, sizeStocksArg:%d]", fMaterials, pstocksArg, sizeStocksArg);

    // Validate arguments
    if(fMaterials == NULL || pstocksArg == NULL){
        so_error("S2.1.4","Invalid Arguments");
        exit(ERROR);
    }

    rewind(fMaterials);

    char line[256];
    int i = 0;

    while(fgets(line, sizeof(line), fMaterials) && i < sizeStocksArg){
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        char name[100] = "";
        int price_per_kg = 0;
        int daily_limit = 0;

        sscanf(line, "%99[^;];%d;%d", name, &price_per_kg, &daily_limit);

        // Allocate each StockItem to array
        strncpy(pstocksArg[i].material, name, sizeof(pstocksArg[i].material) - 1);
        pstocksArg[i].material[sizeof(pstocksArg[i].material) - 1] = '\0';

        pstocksArg[i].price = price_per_kg;
        pstocksArg[i].maxStock = daily_limit;
        pstocksArg[i].stock = 0;

        i++;
    }

    fclose(fMaterials);

    so_success("S2.1.4", "");
    so_debug(">");
}

/**
 * @brief  s2_2_OpenFileRequests Read the description of task S2.2 on the assignment script.
 * @param  filenameRequests (I) the name of the file that holds the Client requests (i.e., FILE_REQUESTS)
 * @param  pfRequests (O) the opened handler of the file that holds the Client requests
 */
void s2_2_OpenFileRequests(char *filenameRequests, FILE **pfRequests) {
    so_debug("< [@param(I) filenameRequests:%s]", filenameRequests);
    
    // create file if it does not exist
    if (access(filenameRequests, F_OK) != 0){
        FILE *fCreate = fopen(filenameRequests, "wb");
        if (fCreate == NULL){
            so_error("S2.2", "Error creating pedidos.dat");
            exit(ERROR);
        }
        fclose(fCreate);
    }

    if(*pfRequests = fopen(filenameRequests, "r") == NULL){
        so_error("S2.2", "Error opening pedidos.dat");
        exit(ERROR)
    }
 
    //so_success("S2.2", "%s", filenameRequests);
    so_debug("> [[@param(O) *pfRequests:%p]", *pfRequests);
}

/**
 * @brief  s2_3_1_ReadRequest Read the description of task S2.3.1 on the assignment script.
 * @param  fRequests (I) the opened handler of the file that holds the Client requests
 * @param  pclientRequest (O) the Client request (the amount field is not filled by the Client)
 * @return TRUE if a request was read
 *         FALSE if the read action was not successful because the file has no more data (no error)
 */
int s2_3_1_ReadRequest(FILE *fRequests, Transaction *pclientRequest) {
    so_debug("< [@param(I) fRequests:%p]", fRequests);

    int return_value = FALSE;

    // replace

    so_debug("> [@return return_value:%d], [@param(O) *pclientRequest:[%s:%d:%c:%s:%f:%d:%d]]", return_value, pclientRequest->material, pclientRequest->quantity, pclientRequest->type, pclientRequest->customerName, pclientRequest->amount, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
    return return_value;
}

/**
 * @brief  s2_3_2_IsValidRequest Read the description of task S2.3.2 on the assignment script.
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
 * @return TRUE if the request passed all validations
 */
int s2_3_2_IsValidRequest(Transaction clientRequest) {
    so_debug("< [@param(I) clientRequest:[%s:%d:%c:%s:%f:%d:%d]]", clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    int return_value = FALSE;
 
    // replace
 
    //so_success("S2.3.2", "Valid request");
    return_value = TRUE;
 
    so_debug("> [@return return_value:%d]", return_value);
    return return_value;
}

/**
 * @brief  s2_3_3_2_ProcessRequest Read the description of task S2.3.3.2 on the assignment script.
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
 * @param  stocks (I) the array of dairy material stocks
 * @param  sizeStocks (I) the number of elements of the array of dairy material stocks
 * @param  pquantity (O) the maximum calculated quantity of material the Client is allowed to Buy/Sell
 * @param  pamount (O) the calculated value (amount of money) of the material the Client is allowed to Buy/Sell
 */
void s2_3_3_2_ProcessRequest(Transaction clientRequest, StockItem *stocks, int sizeStocks, int *pquantity, float *pamount) {
    so_debug("< [@param(I) clientRequest:[%s:%d:%c:%s:%f:%d:%d]], stocks:%p, sizeStocks:%d", clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado, stocks, sizeStocks);

    // replace

    //so_success("S2.3.3.2", "%d %d %.2f", item->stock, usedQty, amount);
    so_debug(">");
}

/**
 * @brief  s2_3_4_CleanRequests Read the description of task S2.3.4 on the assignment script.
 * @param  pfRequests (I) the opened handler of the file that holds the Client requests
 * @param  filenameRequests (I) the name of the file that holds the Client requests (i.e., FILE_REQUESTS)
 */
void s2_3_4_CleanRequests(FILE *fRequests, char *filenameRequests) {
    so_debug("< [@param(I) fRequests:%p, filenameRequests:%s]", fRequests, filenameRequests);

    // replace

    //so_success("S2.3.4", "Requests cleared");

    so_debug(">");
}

/*************************************
 *              FASE 2B              *
 *************************************/
/**
 * @brief  s1_4_CreateRequestsDB Read the description of task S1.4 on the assignment script.
 * @param  prequests (O) the new array of requests to be handled by the application
 * @param  sizeRequests (I) the number of elements of the new array of requests to be handled by the application (i.e., MAX_REQUESTS)
 */
void s1_4_CreateRequestsDB(SessionRequest **prequests, int sizeRequests) {
    so_debug("< [[@param(I) prequests:%p, sizeRequests:%d]", prequests, sizeRequests);

    if (prequests == NULL) {
        so_error("S1.4", "Invalid requests pointer");
        exit(ERROR);
    }

    *prequests = (SessionRequest *) malloc(sizeof(SessionRequest) * MAX_REQUESTS);

    if (*prequests == NULL) {
        so_error("S1.4", "Memory allocation failed");
        exit(ERROR);
    }

    for (int i = 0; i < MAX_REQUESTS; i++) {
        (*prequests)[i].transaction.pidCliente = INVALID;
    }

    so_success("S1.4", "Requests DB created");
    so_debug(">");
}

/**
 * @brief  s1_5_TrapSignalsServer Read the description of task S1.5 on the assignment script.
 */
void s1_5_TrapSignalsServer() {
    so_debug("<");

    signal(SIGUSR1, s3_HandleSigUsr1);
    signal(SIGINT,  s4_HandleCtrlC);
    signal(SIGCHLD, s6_HandleFinishedDedicatedServer);

    so_success("S1.5", "Signals trapped successfully");
    so_debug(">");
}

/**
 * @brief  s2_3_3_1_GetClientDBEntry Read the description of task S2.3.3.1 on the assignment script.
 * @param  clientRequest (I) the Client request (the amount field is not filled by the Client)
 * @param  requests (I) the array of requests to be handled by the application
 * @param  sizeRequests (I) the number of elements of the array of requests to be handled by the application (i.e., MAX_REQUESTS)
 * @return the index (if >= 0) of a found free entry on the requests array
 *         ERROR if no free entry was found on the requests array
 */
int s2_3_3_1_GetClientDBEntry(Transaction clientRequest, SessionRequest *requests, int sizeRequests) {
    so_debug("< [@param(I) clientRequest:[%s:%d:%c:%s:%f:%d:%d], requests:%p, sizeRequests:%d]", clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado, requests, sizeRequests);
    int return_value = ERROR;
    int pos = -1;

    // Find free slot
    for (int i = 0; i < sizeRequests; i++) {
        if (requests[i].transaction.pidCliente == INVALID) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        kill(clientRequest.pidCliente, SIGHUP);
        so_error("S2.3.3.1", "No free entry in requests DB");
        return ERROR;
    }

    // Store request in DB
    requests[pos].transaction = clientRequest;
    so_success("S2.3.3.1", "Reservei Posição: %d", pos);
    return_value = pos;
    so_debug("> [@return return_value:%d]", return_value);
    return return_value;
}

/**
 * @brief  s2_3_3_3_UpdateClientDB Read the description of task S2.3.3.3 on the assignment script.
 * @param  quantity (I) the maximum calculated quantity of material the Client is allowed to Buy/Sell.
 * @param  amount (I) the calculated value (amount of money) of the material the Client is allowed to Buy/Sell.
 * @param  requests (I) the array of requests to be handled by the application
 * @param  indexClientDB (I) the requests array (Client DB) index corresponding to the current Client request
 */
void s2_3_3_3_UpdateClientDB(int quantity, float amount, SessionRequest *requests, int indexClientDB) {
    so_debug("< [@param(I) quantity:%d, amount:%f, requests:%p, indexClientDB:%d]", quantity, amount, requests, indexClientDB);

    // Update DB entry 
    requests[indexClientDB].transaction.quantity = quantity;
    requests[indexClientDB].transaction.amount   = amount;

    so_success("S2.3.3.3", "Atualizei Posição: %d %.2f", indexClientDB, amount);
    so_debug(">");
}

/**
 * @brief  s2_3_3_4_CreateDedicatedServer Read the description of task S2.3.3.4 on the assignment script.
 * @param  requests (I) the array of requests to be handled by the application
 * @param  indexClientDB (I) the requests array (Client DB) index corresponding to the current Client request
 */
void s2_3_3_4_CreateDedicatedServer(SessionRequest *requests, int indexClientDB) {
    so_debug("< [@param(I) requests:%p, indexClientDB:%d]", requests, indexClientDB);

    int savedIndex = indexClientDB;

    pid_t pid = fork();
    if (pid < 0) {
        so_error("S2.3.3.4", "Fork failed");
        exit(ERROR);
    }
    if (pid == 0) {
        indexClientDB = savedIndex;
        requests[indexClientDB].transaction.pidServidorDedicado = getpid();
        so_success("SD8", "Nasci com PID %d", getpid());
        clientRequest = requests[indexClientDB].transaction;
        sd8_MainDedicatedServer();
        exit(SUCCESS);
    } else {
        if (savedIndex >= 0 && requests != NULL)
            requests[savedIndex].transaction.pidServidorDedicado = pid;
        so_success("Servidor", "Iniciei SD %d", pid);
    }

    so_debug(">");
}

/**
 * @brief  s3_HandleSigUsr1 Read the description of task S3 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void s3_HandleSigUsr1(int receivedSignal) {
    so_debug("< [@param(I) receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGUSR1) {
        so_success("Servidor", "Recebi Pedido");
        #if CURRENT_TEST_PHASE < PHASE_2C
            s2_3_ReadRequests(fRequests, stocks, sizeStocks);
        #endif
    }

    so_debug(">");
}

/**
 * @brief  s4_HandleCtrlC Read the description of task S4 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void s4_HandleCtrlC(int receivedSignal) {
    so_debug("< [@param(I) receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGINT) {
        so_success("Servidor", "Start Shutdown");

        if (requests != NULL) {
            for (int i = 0; i < MAX_REQUESTS; i++) {
                if (requests[i].transaction.pidCliente != INVALID &&
                    requests[i].transaction.pidServidorDedicado > 0) {
                    kill(requests[i].transaction.pidServidorDedicado, SIGUSR2);
                }
            }
        }

        s5_ShutdownServer();
    }

    so_debug(">");
}

/**
 * @brief  s5_1_RemoveFileServerPid Read the description of task S5.1 on the assignment script.
 * @param  filenameServerPid (I) the name of the file that holds the PID of the Server (i.e., FILE_SERVERPID)
 */
void s5_1_RemoveFileServerPid(char *filenameServerPid) {
    so_debug("< [@param(I) filenameServerPid:%s]", filenameServerPid);

    if (remove(filenameServerPid) != 0) {
        so_error("S5.1", "Error removing servidor.pid");
        exit(ERROR);
    }

    so_success("S5.1", "");
    so_debug(">");
}

/**
 * @brief  s6_HandleFinishedDedicatedServer Read the description of task S6 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void s6_HandleFinishedDedicatedServer(int receivedSignal) {
    so_debug("< [@param(I) receivedSignal:%d]", receivedSignal);

    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        so_success("Servidor", "Confirmo que terminou o SD %d", pid);
        for (int i = 0; i < MAX_REQUESTS; i++) {
            if (requests[i].transaction.pidServidorDedicado == pid) {
                requests[i].transaction.pidCliente = INVALID;
                break;
            }
        }
    }

    so_debug(">");
}

/**
 * @brief  s7_SolveDataProblem Read the description of task S7 on the assignment script.
 * @param  pidServidorDedicado (I) PID of a Servidor Dedicado
 * @param  requests (I) the array of requests to be handled by the application
 * @param  sizeRequests (I) the number of elements of the array of requests to be handled by the application (i.e., MAX_REQUESTS)
 */
void s7_SolveDataProblem(int pidServidorDedicado, SessionRequest *requests, int sizeRequests) {
    so_debug("< [@param(I) pidServidorDedicado:%d, requests:%p, sizeRequests:%d]",
             pidServidorDedicado, requests, sizeRequests);

    if (requests == NULL) {
        so_debug(">");
        return;
    }

    for (int i = 0; i < sizeRequests; i++) {

        if (requests[i].transaction.pidServidorDedicado == pidServidorDedicado) {

            requests[i].transaction.pidCliente = INVALID;
            requests[i].transaction.pidServidorDedicado = 0;
            requests[i].transaction.quantity = 0;
            requests[i].transaction.amount = 0.0f;
            requests[i].transaction.material[0] = '\0';
            requests[i].transaction.customerName[0] = '\0';

            break;
        }
    }

    so_debug(">");
}

/**
 * @brief  sd8_1_ValidateClientFifo Read the description of task SD8.1 on the assignment script.
 * @param  clientRequest (I) the Client request which holds the information about the Client PID which will be part of the Client FIFO filename.
 */
void sd8_1_ValidateClientFifo(Transaction clientRequest) {
    so_debug("< [@param(I) pidCliente:%d]", clientRequest.pidCliente);

    char fifoName[64];
    sprintf(fifoName, "%d.fifo", clientRequest.pidCliente);

    if (access(fifoName, F_OK) != 0) {
        so_error("SD8.1", "Client FIFO does not exist");
        exit(ERROR);
    }

    so_success("SD8.1", "Client FIFO validated");
    so_debug(">");
}

/**
 * @brief  sd8_2_TrapSignalsDedicatedServer Read the description of task SD8.2 on the assignment script.
 */
void sd8_2_TrapSignalsDedicatedServer() {
    so_debug("<");

    signal(SIGINT, SIG_IGN);

    signal(SIGUSR2, sd11_HandleSigusr2);

    so_debug(">");
}

/**
 * @brief  sd9_1_SleepRandomTime Read the description of task SD9.1 on the assignment script.
 * @param  minTime (I) Minimum wait time
 * @param  maxTime (I) Maximum wait time
 */
void sd9_1_SleepRandomTime(int minTime, int maxTime) {
    so_debug("< [@param(I) minTime:%d, maxTime:%d]", minTime, maxTime);

    srand(getpid() ^ time(NULL));

    int t = (rand() % (maxTime - minTime + 1)) + minTime;

    so_success("SD9.1", "%d", t);

    sleep(t);

    if (sdb_shutdown_requested) {
        sd10_ShutdownDedicatedServer();
    }
    
    so_debug(">");
}

/**
 * @brief  sd9_2_OpenClientFifo Read the description of task SD9.2 on the assignment script.
 * @param  clientRequest (I) the Client request which holds the information about the Client PID which will be part of the Client FIFO filename.
 * @param  pfClientFifo (O) the opened handler of the Client FIFO
 */
void sd9_2_OpenClientFifo(Transaction clientRequest, FILE **pfClientFifo) {
    so_debug("< [@param(I) pidCliente:%d]", clientRequest.pidCliente);

    char fifoName[64];
    sprintf(fifoName, "%d.fifo", clientRequest.pidCliente);

    int fd = open(fifoName, O_WRONLY);
    if (fd < 0) {
        so_error("SD9.2", "Error opening client FIFO");
        exit(ERROR);
    }

    *pfClientFifo = fdopen(fd, "w");
    if (*pfClientFifo == NULL) {
        so_error("SD9.2", "fdopen failed");
        close(fd);
        exit(ERROR);
    }

    so_success("SD9.2", "Client FIFO opened");

    so_debug("> [*pfClientFifo:%p]", *pfClientFifo);
}

/**
 * @brief  sd9_3_WriteResponse Read the description of task SD9.3 on the assignment script.
 * @param  fClientFifo (I) the opened handler of the Client FIFO
 * @param  clientRequest (I) the proposal to be sent by this dedicated server to the Client
 */
void sd9_3_WriteResponse(FILE *fClientFifo, Transaction clientRequest) {
    so_debug("< [@param fClientFifo:%p, clientRequest:[%s:%d:%c:%s:%f:%d:%d]]", fClientFifo, clientRequest.material, clientRequest.quantity, clientRequest.type, clientRequest.customerName, clientRequest.amount, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (fClientFifo == NULL) {
        so_error("SD9.3", "Invalid FIFO");
        exit(ERROR);
    }

    size_t written = fwrite(&clientRequest, sizeof(Transaction), 1, fClientFifo);

    if (written != 1) {
        so_error("SD9.3", "Error writing to client FIFO");
        fclose(fClientFifo);
        exit(ERROR);
    }

    fflush(fClientFifo);
    fclose(fClientFifo);

    so_success("SD9.3", "SD: Enviei proposta ao Cliente com PID %d",
               clientRequest.pidCliente);

    so_debug(">");
}

/**
 * @brief  sd10_1_FreeClientDBEntry Read the description of task SD10.1 on the assignment script.
 * @param  requests (I) the array of requests to be handled by the application
 * @param  indexClientDB (I) the requests array (Client DB) index corresponding to the current Client request
 */
void sd10_1_FreeClientDBEntry(SessionRequest *requests, int indexClientDB) {
    so_debug("<");

    if (requests == NULL || indexClientDB < 0) {
        so_error("SD10.1", "Invalid requests database or index");
        so_debug(">");
        return;
    }

    requests[indexClientDB].transaction.pidCliente = INVALID;
    requests[indexClientDB].transaction.pidServidorDedicado = 0;

    so_success("SD10.1", "Libertei Posição: %d", indexClientDB);
    so_debug(">");
}

/**
 * @brief  sd11_HandleSigusr2 Read the description of task SD11 on the assignment script.
 * @param  receivedSignal (I) ID (number) of the signal that triggered this function call (sent by the OS)
 */
void sd11_HandleSigusr2(int receivedSignal) {
    so_debug("< [@param(I) receivedSignal:%d]", receivedSignal);

    if (receivedSignal == SIGUSR2) {
        so_success("SD", "Recebi pedido do Servidor para terminar");
        sdb_shutdown_requested = 1;
    }

    so_debug(">");
}

/*************************************
 *              FASE 2C              *
 *************************************/
/**
 * @brief  s1_6_CreateMsgQueue Read the description of task S1.6 on the assignment script.
 * @param  ipcKey (I) IPC_KEY to be used in the project
 * @param  pmsgId (O) opened IPC Message Queue (MSG) ID handler
 */
void s1_6_CreateMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param(I) ipcKey:0x0%x]", ipcKey);

    // Try to access existing queue
    int msgId = msgget(ipcKey, 0666);

    // If it exists, remove it
    if (msgId != -1) {
        if (msgctl(msgId, IPC_RMID, NULL) == -1) {
            so_error("S1.6", "Error removing existing message queue");
            exit(ERROR);
        }
    }

    // Create new message queue
    *pmsgId = msgget(ipcKey, IPC_CREAT | 0666);

    if (*pmsgId == -1) {
        so_error("S1.6", "Error creating message queue");
        exit(ERROR);
    }

    so_success("S1.6", "Message queue created");
    so_debug("> [@param(O) *pmsgId:%d]", *pmsgId);
}

/**
 * @brief  s2_4_ReadClientRequest Read the description of task S2.4 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  pmsgClientRequest (O) the message sent via MSG by a Client
 */
void s2_4_ReadClientRequest(int msgId, MsgContent *pmsgClientRequest) {
    so_debug("< [@param(I) msgId:%d]", msgId);

    if (pmsgClientRequest == NULL) {
        so_error("S2.4", "Invalid message pointer");
        exit(ERROR);
    }

    int received = -1;
    // Keep trying until successful read
    while (received == -1) {
        received = so_MsgReceive(msgId, pmsgClientRequest, MSGTYPE_LOGIN);
        if (received == -1 && errno == EINTR) {
            // interrupted by signal → continue waiting
            continue;
        }

        if (received == -1) {
            so_error("S2.4", "Error receiving message");
            exit(ERROR);
        }
    }

    so_success("S2.4", "%d", pmsgClientRequest->msgData.t.pidCliente);
    so_debug("> [@param(O) *pmsgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d]]", pmsgClientRequest->msgType, pmsgClientRequest->msgData.status, pmsgClientRequest->msgData.t.material, pmsgClientRequest->msgData.t.quantity, pmsgClientRequest->msgData.t.type, pmsgClientRequest->msgData.t.customerName, pmsgClientRequest->msgData.t.amount, pmsgClientRequest->msgData.t.pidCliente, pmsgClientRequest->msgData.t.pidServidorDedicado);
}

/**
 * @brief  s5_2_TerminateAllDedicatedServers Read the description of task S5.2 on the assignment script.
 * @param  requests (I) the array of requests to be handled by the application
 * @param  sizeRequests (I) the number of elements of the array of requests to be handled by the application (i.e., MAX_REQUESTS)
 */
void s5_2_TerminateAllDedicatedServers(SessionRequest *requests, int sizeRequests) {
    so_debug("< [@param(I) requests:%p, sizeRequests:%d]", requests, sizeRequests);

    if (requests == NULL) {
        so_error("S5.2", "Invalid requests database");
        so_debug(">");
        return;
    }

    for (int i = 0; i < sizeRequests; i++) {
        if (requests[i].transaction.pidCliente != INVALID &&
            requests[i].transaction.pidServidorDedicado > 0) {
            kill(requests[i].transaction.pidServidorDedicado, SIGUSR2);
        }
    }

    so_success("S5.2", "Servers terminated");
    so_debug(">");
}

/**
 * @brief  s5_4_RemoveMsgQueue Read the description of task S5.4 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 */
void s5_4_RemoveMsgQueue(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);

    if (msgId < 0) {
        so_error("S5.4", "Invalid msgId");
        exit(ERROR);
    }

    if (so_MsgRemove(msgId) < 0) {
        so_error("S5.4", "Error removing message queue");
        exit(ERROR);
    }

    so_success("S5.4", "Message queue removed");
    so_debug(">");
}

/**
 * @brief  sd9_4_SendProposal Read the description of task SD9.4 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  requests (I) the array of requests to be handled by the application
 * @param  indexClientDB (I) the requests array (Client DB) index corresponding to the current Client request
 */
void sd9_4_SendProposal(int msgId, SessionRequest *requests, int indexClientDB) {
    so_debug("< [@param(I) msgId:%d, requests:%p, indexClientDB:%d]", msgId, requests, indexClientDB);

    if (requests == NULL || indexClientDB < 0) {
        so_error("SD9.4", "Invalid arguments");
        exit(ERROR);
    }

    MsgContent msg;
    msg.msgData.status = PROPOSAL;
    msg.msgData.t = requests[indexClientDB].transaction;

    long targetType = (long) requests[indexClientDB].transaction.pidCliente;

    if (so_MsgSend(msgId, &msg, targetType) < 0) {
        so_error("SD9.4", "Error sending proposal");
        exit(ERROR);
    }

    so_success("SD9.4", "SD: Enviei proposta ao Cliente com PID %d", msg.msgData.t.pidCliente);
    so_debug(">");
}

/**
 * @brief  sd9_5_ReceiveClientApproval Read the description of task SD9.5 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  pmsgClientRequest (O) the message sent via MSG by a Client
 */

void sd9_5_ReceiveClientApproval(int msgId, MsgContent *pmsgClientRequest) {
    so_debug("< [@param(I) msgId:%d]", msgId);

    if (pmsgClientRequest == NULL) {
        so_error("SD9.5", "Invalid message pointer");
        exit(ERROR);
    }

    int received = -1;
    long myType = (long) getpid();

    while (received == -1) {
        received = so_MsgReceive(msgId, pmsgClientRequest, myType);
        if (received == -1 && errno == EINTR) continue;
        if (received == -1) {
            so_error("SD9.5", "Error receiving client approval");
            exit(ERROR);
        }
    }

    const char *decision = (pmsgClientRequest->msgData.status == ACCEPT) ? "aceitou" : "recusou";

    so_success("SD", "o Cliente com PID %d %s a proposta", pmsgClientRequest->msgData.t.pidCliente, decision);
    so_debug("> [@param(O) *pmsgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d]]", pmsgClientRequest->msgType, pmsgClientRequest->msgData.status, pmsgClientRequest->msgData.t.material, pmsgClientRequest->msgData.t.quantity, pmsgClientRequest->msgData.t.type, pmsgClientRequest->msgData.t.customerName, pmsgClientRequest->msgData.t.amount, pmsgClientRequest->msgData.t.pidCliente, pmsgClientRequest->msgData.t.pidServidorDedicado);
}

/**
 * @brief  sd9_6_CheckResponseAccepted Read the description of task SD9.5 on the assignment script.
 * @param  msgClientRequest (I) the message sent via MSG by a Client
 * @param  filenameLogfile (I) the name of the file that will store the logs of the FerrIULândia transactions (i.e., FILE_LOGFILE)
 */
void sd9_6_CheckResponseAccepted(MsgContent msgClientRequest, char *filenameLogfile) {
    so_debug("< [@param(I) msgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d], filenameLogfile:%s]", msgClientRequest.msgType, msgClientRequest.msgData.status, msgClientRequest.msgData.t.material, msgClientRequest.msgData.t.quantity, msgClientRequest.msgData.t.type, msgClientRequest.msgData.t.customerName, msgClientRequest.msgData.t.amount, msgClientRequest.msgData.t.pidCliente, msgClientRequest.msgData.t.pidServidorDedicado, filenameLogfile);

    if (msgClientRequest.msgData.status != ACCEPT) {
        so_debug(">");
        return;
    }

    FILE *fp = fopen(filenameLogfile, "ab");
    if (fp == NULL) {
        so_error("SD9.6", "Error opening logfile");
        return;
    }

    size_t written = fwrite(&msgClientRequest.msgData.t, sizeof(Transaction), 1, fp);
    if (written != 1) {
        fclose(fp);
        so_error("SD9.6", "Error writing logfile");
        return;
    }

    fclose(fp);
    so_success("SD9.6", "Transaction recorded");
    so_debug(">");
}

/**
 * @brief  sd9_7_CheckResponseRejected Read the description of task SD9.5 on the assignment script.
 * @param  msgClientRequest (I) the message sent via MSG by a Client
 * @param  stocks (I) the array of dairy material stocks
 * @param  sizeStocks (I) the number of elements of the array of dairy material stocks
 */
void sd9_7_CheckResponseRejected(MsgContent msgClientRequest, StockItem *stocks, int sizeStocks) {
    so_debug("< [@param(I) msgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d], stocks:%p, sizeStocks:%d]", msgClientRequest.msgType, msgClientRequest.msgData.status, msgClientRequest.msgData.t.material, msgClientRequest.msgData.t.quantity, msgClientRequest.msgData.t.type, msgClientRequest.msgData.t.customerName, msgClientRequest.msgData.t.amount, msgClientRequest.msgData.t.pidCliente, msgClientRequest.msgData.t.pidServidorDedicado, stocks, sizeStocks);

    if (msgClientRequest.msgData.status != REJECT) {
        so_debug(">");
        return;
    }

    Transaction t = msgClientRequest.msgData.t;

    for (int i = 0; i < sizeStocks; i++) {
        if (strcmp(stocks[i].material, t.material) == 0) {
            if (t.type == 'B') {
                stocks[i].stock += t.quantity;
            } else if (t.type == 'S') {
                stocks[i].stock -= t.quantity;
            }
            break;
        }
    }

    so_success("SD9.7", "Stock reverted");
    so_debug(">");
}

/**
 * @brief  sd10_2_SendTransactionConcluded Read the description of task SD10.2 on the assignment script.
 * @param  msgId (I) opened IPC Message Queue (MSG) ID handler
 * @param  msgClientRequest (I) message to send to the Client to conclude the business.
 */
void sd10_2_SendTransactionConcluded(int msgId, MsgContent msgClientRequest) {
    so_debug("< [@param(I) msgId:%d, msgClientRequest:[%ld:%d:%s:%d:%c:%s:%f:%d:%d]]", msgId, msgClientRequest.msgType, msgClientRequest.msgData.status, msgClientRequest.msgData.t.material, msgClientRequest.msgData.t.quantity, msgClientRequest.msgData.t.type, msgClientRequest.msgData.t.customerName, msgClientRequest.msgData.t.amount, msgClientRequest.msgData.t.pidCliente, msgClientRequest.msgData.t.pidServidorDedicado);

    MsgContent msg;
    msg.msgData.status = TRANSACTION_CONCLUDED;
    msg.msgData.t = msgClientRequest.msgData.t;

    long targetType = (long) msgClientRequest.msgData.t.pidCliente;

    if (so_MsgSend(msgId, &msg, targetType) < 0) {
        so_error("SD10.2", "Error sending transaction concluded");
        return;
    }

    so_success("SD10.2", "Transaction concluded");
    so_debug(">");
}