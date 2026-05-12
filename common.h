/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2025/2026, Enunciado Versão 1+
 **
 ** Este Módulo não deverá ser alterado, e não precisa ser entregue
 ** Nome do Módulo: common.h
 ** Descrição/Explicação do Módulo:
 **     Definição das estruturas de dados comuns aos módulos servidor e cliente
 **
 ***************************************************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include "utils.h"
#include <fcntl.h>
#include <time.h>               // Header para as funções time(), localtime(), strftime()
#include <signal.h>             // Header para as constantes SIG_* e as funções signal(), sigaction(), kill()
#include <unistd.h>             // Header para as funções alarm(), pause(), sleep(), fork(), exec*() e get*pid()
#include <sys/wait.h>           // Header para a função wait()
#include <sys/stat.h>           // Header para as constantes S_ISFIFO e as funções stat() e mkfifo()
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    char material[50];          // Material a vender/comprar
    int quantity;               // Quantidade de material, em kg
    char type;                  // Tipo de transação, Buy 'B' ou Sell 'S'
    char customerName[80];      // Nome do utilizador (comprador ou vendedor)
    float amount;               // Valor total a pagar ou receber (as vendas (Sell) têm margem de 30%)
    int pidCliente;             // PID do processo Cliente
    int pidServidorDedicado;    // PID do processo Servidor Dedicado
} Transaction;

typedef struct {
    long msgType;               // Tipo da Mensagem
    struct {
        int status;             // Status
        Transaction t;          // Informação da ordem de transação
    } msgData;                  // Dados da Mensagem
} MsgContent;

typedef struct {
    Transaction transaction;    // Ordem de transação
    char dateTransaction[17];   // Timestamp da transação, no formato AAAA-MM-DDTHHhmm
} SessionRequest;

typedef struct {
    char material[50];          // Material a vender/comprar
    float price;                // Preço base por kg (no caso das vendas (Sell) acrescenta margem de 30%)
    int stock;                  // Quantidade do material em stock, em kg
    int maxStock;               // Limite máximo do material em stock, em kg
} StockItem;

#define MAX_WAIT         10                         // Número máximo de segundos de espera de um pedido
#define MAX_REQUESTS     10                         // Número máximo de sessões abertas simultaneamente
#define FILE_SERVERPID   "servidor.pid"             // Nome do ficheiro onde o Servidor guarda o valor do seu PID (formato texto)
#define FILE_USERS       "_etc_passwd"              // Nome do Ficheiro onde validar os utilizadores do servidor Tigre (formato texto)
#define FILE_MATERIALS   "materiais.txt"            // Nome do ficheiro que tem a informação dos materiais permitidos (formato texto)
#define FILE_REQUESTS    "pedidos.dat"              // Nome do ficheiro que serve para os Clientes fazerem os pedidos ao Servidor (formato binário Transacao)
#define FILE_LOGFILE     "transactions.dat"         // Nome do Ficheiro para realizar os logs de transacoes (formato binário LogItem)
#define INVALID          -1                         // Utilitário que significa valor inválido

#define MSGTYPE_LOGIN           1   // MsgType do Servidor Principal
#define SEM_MUTEX_INITIAL_VALUE 1   // Valor inicial dos semáforos do tipo MUTEX
#define SEM_MUTEX_BD            0   // Índice do semáforo no Grupo de Semáforos, relativo ao MUTEX para controlar o acesso à BD
#define SEM_MUTEX_LOGFILE       1   // Índice do semáforo no Grupo de Semáforos, relativo ao MUTEX para controlar o acesso ao ficheiro FILE_LOGFILE
#define SEM_SRV_DEDICADOS       2   // Índice do semáforo no Grupo de Semáforos, relativo à espera do fim dos Servidores Dedicados
#define SEM_LUGARES_PARQUE      3   // Índice do semáforo no Grupo de Semáforos, relativo ao número de lugares do parque
#define SEM_MUTEX_FACE          0   // Índice do semáforo no Grupo de Semáforos FACE, relativo ao MUTEX para controlar o acesso à SHM com a variável tarifaAtual
#define DONT_CARE               0   // Utilitário
#define IPC_GET                 0   // Utilitário
#define ACCESS_PERMS         0600   // Permissões para a criação de elementos IPC
#define NEW_ADDRESS          NULL   // Utilitário
#define NO_FLAGS                0   // Utilitário
#define SHMAT_ERROR   (void *) -1   // Utilitário
#define SEM_DOWN               -1   // Utilitário
#define SEM_UP                  1   // Utilitário
#define _PROCESSO_FILHO         0   // Utilitário

#define PROPOSAL                1   // Status que indica que esta mensagem é uma proposta da FerrIULândia
#define ACCEPT                  2   // Status que indica que o Cliente aceitou a proposta da FerrIULândia
#define REJECT                  3   // Status que indica que o Cliente NÃO aceitou a proposta da FerrIULândia
#define TRANSACTION_CONCLUDED   4   // Status que indica que o Servidor (Dedicado) concluiu a transação

/** FUNCTION HEADERS **/
void s1_InitServer(int argc, char *argv[], int *pmaxCapacity);
void s1_1_GetMaxCapacity(int argc, char *argv[], int *pmaxCapacity);
void s1_2_CreateFileServerPid(char *filenameServerPid, FILE **pfServerPid);
void s1_3_WriteServerPid(FILE *fServerPid);
void s1_4_CreateRequestsDB(SessionRequest **prequests, int sizeRequests);
void s1_5_TrapSignalsServer();
key_t so_GenerateIpcKey();
void s1_6_CreateMsgQueue(key_t ipcKey, int *pmsgId);
void s2_MainServer();
void s2_1_InitDailyStocks(int *pdayLastIteration, StockItem **pstocks, int *psizeStocks);
int  s2_1_HasDateChangedSinceLastIteration(int *pdayLastIteration);
void s2_1_1_OpenFileMaterials(char *filenameMaterials, FILE **pfMaterials);
void s2_1_2_ValidateMaxCapacity(FILE *fMaterials, int maxCapacity, int *pnumberOfMaterials);
void s2_1_3_CreateStocks(int numberOfMaterials, StockItem **pstocks);
void s2_1_4_PopulateStocks(FILE *fMaterials, StockItem *stocks, int numberOfMaterials);
void s2_2_OpenFileRequests(char *filenameRequests, FILE **pfrequests);
void s2_3_ReadRequests(FILE *fRequests, StockItem *stocks, int sizeStocks);
int  s2_3_1_ReadRequest(FILE *fRequests, Transaction *pclientRequest);
int  s2_3_2_IsValidRequest(Transaction clientRequest);
void s2_3_3_ProcessRequest(Transaction clientRequest, StockItem *stocks, int sizeStocks, SessionRequest *requests, int sizeRequests);
int  s2_3_3_1_GetClientDBEntry(Transaction clientRequest, SessionRequest *requests, int sizeRequests);
void s2_3_3_2_ProcessRequest(Transaction clientRequest, StockItem *stocks, int sizeStocks, int *pquantity, float *pamount);
void s2_3_3_3_UpdateClientDB(int quantity, float amount, SessionRequest *requests, int indexClientDB);
void s2_3_3_4_CreateDedicatedServer(SessionRequest *requests, int indexClientDB);
void s2_3_4_CleanRequests(FILE *fRequests, char *filenameRequests);
void s2_4_ReadClientRequest(int msgId, MsgContent *pmsgClientRequest);
void s3_HandleSigUsr1(int receivedSignal);
void s4_HandleCtrlC(int receivedSignal);
void s5_ShutdownServer();
void s5_1_RemoveFileServerPid(char *filenameServerPid);
void s5_2_TerminateAllDedicatedServers(SessionRequest *requests, int sizeRequests);
void s5_4_RemoveMsgQueue(int msgId);
void s6_HandleFinishedDedicatedServer(int receivedSignal);
void s7_SolveDataProblem(int pidServidorDedicado, SessionRequest *requests, int sizeRequests);
void sd8_MainDedicatedServer();
void sd8_1_ValidateClientFifo(Transaction clientRequest);
void sd8_2_TrapSignalsDedicatedServer();
void sd9_1_SleepRandomTime(int minTime, int maxTime);
void sd9_2_OpenClientFifo(Transaction clientRequest, FILE **pfClientFifo);
void sd9_3_WriteResponse(FILE *fClientFifo, Transaction clientRequest);
void sd9_4_SendProposal(int msgId, SessionRequest *requests, int indexClientDB);
void sd9_5_ReceiveClientApproval(int msgId, MsgContent *pmsgClientRequest);
void sd9_6_CheckResponseAccepted(MsgContent msgClientRequest, char *filenameLogfile);
void sd9_7_CheckResponseRejected(MsgContent msgClientRequest, StockItem *stocks, int sizeStocks);
void sd10_ShutdownDedicatedServer();
void sd10_1_FreeClientDBEntry(SessionRequest *requests, int indexClientDB);
void sd10_2_SendTransactionConcluded(int msgId, MsgContent msgClientRequest);
void sd11_HandleSigusr2(int receivedSignal);

void c1_1_ValidateServerFiles(char *filenameServerPid, char *filenameRequests);
void c1_2_OpenFileServerPid(char *filenameServerPid, FILE **pfServerPid);
void c1_3_ReadServerPid(FILE *fServerPid, pid_t *pserverPid);
void c1_4_TrapSignalsClient();
void c1_5_CreateClientFifo();
void c1_6_GetExistingMsgQueue(key_t ipcKey, int *pmsgId);
void c2_1_RequestTransactionInfo(Transaction *pclientRequest);
void c2_2_OpenFileRequests(char *filenameRequests, FILE **pfRequests);
void c2_3_WriteRequest(FILE *fRequests, Transaction clientRequest);
void c2_4_AlertServer(pid_t serverPid);
void c2_5_SendClientRequest(int msgId, Transaction clientRequest, MsgContent *pmsgClientRequest);
void c3_ProgramAlarm(int seconds);
void c4_1_OpenClientFifo(FILE **pfClientFifo);
void c4_2_DisableAlarm();
void c4_3_ReadResponse(FILE *fClientFifo, Transaction *pclientRequest);
void c4_4_ReadServerResponse(int msgId, MsgContent *pmsgClientRequest);
void c4_5_ProcessServerResponse(int msgId, MsgContent msgClientRequest);
void c5_ShutdownClient();
void c6_HandleSighup(int receivedSignal);
void c7_HandleCtrlC(int receivedSignal);
void c8_HandleAlarm(int receivedSignal);

key_t so_GenerateIpcKey();
int so_MsgCreateExclusive(key_t ipcKey);
int so_MsgGetExisting(key_t ipcKey);
int so_MsgRemove(int msgId);
int so_MsgSend(int msgId, MsgContent *msg, long msgType);
int so_MsgReceive(int msgId, MsgContent *msg, long msgType);

int so_SemCreateExclusive(key_t ipcKey, int nrSems);
int so_SemGetExisting(key_t ipcKey);
int so_SemRemove(int semId);
int so_SemGetValue(int semId, int idxSem);
int so_SemSetValue(int semId, int idxSem, int semValue);
int so_SemAddValue(int semId, int idxSem, int addValue);
int so_SemUp(int semId, int idxSem, int incrementValue);
int so_SemDown(int semId, int idxSem, int decrementValue);

int so_ShmCreateExclusive(key_t ipcKey, int sizeInBytes);
int so_ShmGetExisting(key_t ipcKey);
int so_ShmRemove(int shmId);
void *so_ShmAttach(int shmId);

#endif  // __COMMON_H__