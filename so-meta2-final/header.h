#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h> 
#include <sys/stat.h>


#define BUFFER_SIZE 256
#define CODE_SIZE 10
#define PIPE_NAME "input_pipe"

#define VOO 1
#define SLOT 2 //torre de controlo responde com o nr de slot atribuido
#define READY 3
#define GO 4

extern int errno; 

/**
	Renato Santos 2015237457
	Simao Brito
**/


typedef struct{
	int nrVoos; //criados ---
	int nrVoosAterrados;
	double tempoMedioEsperaAterragem; //para alem do ETA
	int nrTotalVoosDescolados; //---
	double tempoMedioEsperaDescolagem;
	double nrMedioHoldingAterragem;
	double nrMedioHoldingUrgencia;
	int nrVoosRedirecionados;
	int nrVoosRejeitados;
}stats_struct;

// CONFIG
typedef struct configuracao{
	int ut; //ms
	int duracaoDescolagem; int intervaloDescolagem;
	int duracaoAterragem; int intervaloAterragens;
	int holdingMin; int holdingMax;
	int maxPartidas;
	int maxChegadas;
}Config_struct;

/* ----- VOOS ------ */
typedef struct departure
{
	char flight_code[CODE_SIZE];
	int initial_time;
	int takeoff_time; 
}VooPartida;

typedef struct arrival
{
	char flight_code[CODE_SIZE];
	int initial_time;
	int time_to_runway;
	int initial_fuel; 
} VooChegada;

// filas de espera
typedef struct filaPartida {
    VooPartida *partida;
    struct filaPartida *next;
    //struct filaPartida *prev;
} queuePartida;

typedef struct filaChegada {
    VooChegada *chegada;
    struct filaChegada *next;
} queueChegada;


// MQ (message queue)
typedef struct {
	long mtype;
	//int id;
	char flight_code[CODE_SIZE];
	int takeoff; //partida 
	int tchegada;
	int eta; //chegada
	int fuel; //chegada
	long slot;
	bool emergencia;
	bool readyP; //chegou o takeoff
	bool readyC; // chegou o init + eta
	bool holding;
} message;

typedef struct {
	long nr_slot;
	char flight_code[CODE_SIZE];
	int eta;
	int tchegada;
	int fuel;
	bool aterragem; //faz a operacao
	bool emergencia;
	int pista; //valor será 1 para aterrar na pista C1 e 2 para aterrar na pista C2
	//bool ready;
}slotVooC;

typedef struct {
	long nr_slot;
	char flight_code[CODE_SIZE];
	int takeoff;
	bool descolagem; //faz a operacao
	//bool ready;
	int pista; //valor será 1 para aterrar na pista D1 e 2 para aterrar na pista D2
}slotVooP;

// filas de espera para as PISTAS
typedef struct filaPartidaPISTA {
    slotVooP *partidaPISTA; // [N] --> ??
    struct filaPartidaPISTA *next;
    //struct filaPartida *prev;
} queuePartidaPISTA;

typedef struct filaChegadaPISTA {
    slotVooC *chegadaPISTA;
    struct filaChegadaPISTA *next;
} queueChegadaPISTA;

/*
bool p1 = false;
bool p2 = false;
bool c1 = false;
bool c2 = false;
*/

bool P1 = false;
bool P2 = false;
bool C1 = false;
bool C2 = false;


int adiaPartida = 0;

int tempo;
int *shm_tempo;

//stats
int contaHolding=0;
int nrPartidas = 0;
int nrAterragens = 0;
int contaHoldingEmergencia = 0;
int contaEmergencias = 0;


// Queue para voos
queuePartida *noPartida_inicio = NULL;
queuePartida *partida_atual;// = NULL;

queueChegada *noChegada_inicio = NULL;
queueChegada *chegada_atual;// = NULL;

// voos
VooPartida *partida;
VooChegada *chegada;

// SLOTS dos voos em SHM
int shm_slotC;
int shm_slotP;

slotVooC *chegadaPISTA;
slotVooP *partidaPISTA;

// queues para as PISTAS
queuePartidaPISTA *noPartida_inicioPISTA = NULL;
queuePartidaPISTA *partida_atualPISTA;// = NULL;

int shmHeadPISTA_p;
//int shmAtualPISTA_p;

queueChegadaPISTA *noChegada_inicioPISTA = NULL;
queueChegadaPISTA *chegada_atualPISTA;// = NULL;


Config_struct *config;

// semaforos
sem_t *semLOG; // LOG -> para envio sincronizado do output para ficheiro de log e ecra
sem_t *sem_tempo; //controlo do contador de tempo;
sem_t *sem_voo;


// para esperar que chegue o tempo do voo
pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
pthread_mutex_t mutexVoo = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condVoo  = PTHREAD_COND_INITIALIZER;


pthread_mutex_t * pmutex = NULL;
pthread_mutexattr_t attrmutex;
pthread_mutexattr_init(&attrmutex);
pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
pthread_mutex_init(pmutex, &attrmutex);

pthread_cond_t * pcond = NULL;
pthread_condattr_t attrcond;
*/

/* --- memoria partilhada --- */
//stats
stats_struct *stats;
int shmid_stats;

//message queue
int mqid;
long nrSlot = 0;

//threads voos
pthread_t thread_voo;
pthread_t thread_tempo;
//int id = 1;

//pipe
int readInputPipe; //ler
int fd; //escrever

// tamanho das listas de espera
int tamFilaPartidas = 0;
int tamFilaChegadas = 0;

// pistas
int tamFilaPartidasPISTA = 0;
int tamFilaChegadasPISTA = 0;

// ------------ funcoes ---------------
// ler pipe
void *readPipe();

// Threads
int criaThreadVoo();
void *gerirVooP(); //worker
void *gerirVooC(); //worker

// tempo atual do programa em UT
void *getTempo();

//voos
int criaVooPartida(char *code, int initReady, int initWant);
int criaVooChegada(char *code, int initReady, int eta, int fuel);

//queues
queuePartida* inserir_filaPartida(VooPartida *partida, int init);
queuePartidaPISTA* inserir_filaPartidaPISTA(slotVooP *slotP, int takeoff);

void remover_filaPartida();
void remover_filaPartidaPISTA();
//int *getLastDeparture();

queueChegada* inserir_filaChegada(VooChegada *chegada, int init);
queueChegadaPISTA* inserir_filaChegadaPISTA(slotVooC *slotC, int tchegada);
queueChegadaPISTA* insere_emergencia(slotVooC *c);

void remover_filaChegada();
void remover_filaChegadaPISTA();

void printQueuePartida();
void printQueuePartidaPISTA();

void printQueueChegada();
void printQueueChegadaPISTA();

//estatisticas
void initStats();
void printStats();

// semaforos
void createSemaphore();



