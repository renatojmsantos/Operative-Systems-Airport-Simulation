
#include "header.h"

// Compile as: gcc store_solution.c -lpthread -D_REENTRANT -Wall -o store
// gcc main.c -pthread -Wall -o main

/**
	Renato Santos 2015237457
	Simao Brito
**/

// MAIN -> SIMULATOR MANAGER

void loadConfig(){
	/*
	unidade de tempo (ut, em milissegundos)
	duração da descolagem (T, em ut), intervalo entre descolagem (dt, em ut)
	duração da aterragem (L, em ut), intervalo entre aterragens (dl, em ut)
	holding duração mínima (min, em ut), máxima (max, em ut)
	quantidade máxima de partidas no sistema (D)
	quantidade máxima de chegadas no sistema (A)
	*/

	FILE *fp;
	fp = fopen("config.txt", "r");

	if(fp==NULL){
		perror("Erro a ler ficheiro...");
		exit(1);
	}
	int ut; //ms
	int duracaoDescolagem; int intervaloDescolagem;
	int duracaoAterragem; int intervaloAterragens;
	int holdingMin; int holdingMax;
	int maxPartidas;
	int maxChegadas;

	fscanf(fp, "%d", &ut);
	fscanf(fp, "%d, %d",&duracaoDescolagem,&intervaloDescolagem);
	fscanf(fp, "%d, %d",&duracaoAterragem,&intervaloAterragens);
	fscanf(fp, "%d, %d",&holdingMin,&holdingMax);
	fscanf(fp, "%d", &maxPartidas);
	fscanf(fp, "%d", &maxChegadas);
	

	config = (Config_struct*) malloc(sizeof(Config_struct));
	config->ut = ut;
	config->duracaoDescolagem = duracaoDescolagem;
	config->intervaloDescolagem = intervaloDescolagem;
	config->duracaoAterragem = duracaoAterragem;
	config->intervaloAterragens = intervaloAterragens;
	config->holdingMin = holdingMin;
	config->holdingMax = holdingMax;
	config->maxPartidas = maxPartidas;
	config->maxChegadas = maxChegadas;

	/*
	printf("\nUnidade de tempo: %d ms\n", config->ut);
	printf("Duracao da descolagem: %d\tIntervalo entre descolagem: %d\n", config->duracaoDescolagem, config->intervaloDescolagem);
	printf("Duracao da aterragem: %d\tIntervalo entre aterragem: %d\n", config->duracaoAterragem, config->intervaloAterragens);
	printf("Holding duracao minima: %d\tHolding duracao maxima: %d\n", config->holdingMin, config->holdingMax);
	printf("Quantidade maxima de partidas no sistema: %d\n", config->maxPartidas);
	printf("Quantidade maxima de chegadas no sistema: %d\n\n", config->maxChegadas);
	*/

	fclose(fp);
}

/*
Deverá pôr no log os seguintes eventos acompanhados da sua data e hora:
• Inicio e fim do programa;
• Inicio e fim de cada thread voo;
• Atribuição de uma manobra de holding a um voo (incluindo o holding time
atribuído, o nome do avião, a distância e o combustível)
• Ordem de aterragem e descolagem (incluir o nome do avião e pista atribuída)
• Leitura de um comando do pipe
• Erros nos comandos recebidos pelo pipe
• Mensagem de emergência de um avião
• Desvios de aviões para outro aeroporto
*/

// escrever no LOG
void writeLOG(char msg[]){ //char msg[]
	//printf("-->>> A escrever no LOG.txt .... \n");
	FILE *fp = fopen("log.txt", "a");

	if (fp == NULL){
	    printf("Error opening file!\n");
	    exit(1);
	}
	
	time_t now = time(NULL);
	struct tm local = *localtime(&now); 

	fprintf(fp, "%02d/%02d/%d   %02d:%02d:%02d -> %s\n", local.tm_mday, local.tm_mon + 1, local.tm_year + 1900, local.tm_hour, local.tm_min, local.tm_sec, msg);

	fclose(fp);
}

// remover recursos
void removeResources(){
	free(partida);
	free(chegada);

	free(partida_atual);
	free(noPartida_inicio);

	free(chegada_atual);
	free(noChegada_inicio);

	free(config);

	//free(partidaPISTA);
	//free(chegadaPISTA);

	free(partida_atualPISTA);
	free(noPartida_inicioPISTA);

	free(chegada_atualPISTA);
	free(noChegada_inicioPISTA);

	close(fd);
	unlink(PIPE_NAME);
	
	// semaforos
	sem_close(semLOG); //--
	sem_unlink("LOG");

	sem_close(sem_tempo);
	sem_unlink("TEMPO");

	sem_close(sem_voo);
	sem_unlink("VOO");

	sem_destroy(semLOG);
	sem_destroy(sem_tempo);
	sem_destroy(sem_voo);

	// mutex cond
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	//pthread_mutex_destroy(&mutexVoo);
	//pthread_cond_destroy(&condVoo);
	/*
	pthread_mutex_destroy(pmutex);
	pthread_mutexattr_destroy(&attrmutex); 

	pthread_cond_destroy(pcond);
	pthread_condattr_destroy(&attrcond); 
	*/

	//shm
	shmdt(partidaPISTA);
	shmdt(chegadaPISTA);
	shmdt(stats);
	shmdt(shm_tempo); //contador tempo em shm

	//shmdt(noPartida_inicioPISTA);
	//shmdt(partida_atualPISTA);
	//shmctl(shmHeadPISTA_p,IPC_RMID,NULL);
	//shmctl(shmAtualPISTA_p,IPC_RMID,NULL);

	shmctl(shm_slotC,IPC_RMID,NULL);
	shmctl(shm_slotP,IPC_RMID,NULL);
	shmctl(shmid_stats,IPC_RMID,NULL);
	shmctl(tempo,IPC_RMID,NULL);

	//mq
	msgctl(mqid,IPC_RMID,NULL);

	//pthread_exit(NULL); // morre a thread do tempo

	/* Garante que todos os processos recebem um SIGTERM */
	kill(0, SIGTERM);
	//kill(pid, SIGUSR1);
	printf(".... recursos removidos\n");
}


void endprogram(int sig){
 	
    sem_wait(semLOG);
    printf("Fim do programa...\n");
	char msg[BUFFER_SIZE] = "Fim do programa";
	writeLOG(msg);
	sem_post(semLOG);

	removeResources();
	exit(0);
}

void *getTempo(){

	while(1){

		usleep(config->ut*1000);

		//sem_wait(&semaforo->sem_tempo);
		//sem_wait(sem_tempo);
		pthread_mutex_lock(&mutex);
		tempo++;
		//sem_post(sem_tempo);
		//sem_post(&semaforo->sem_tempo);

		printf("%d ut\n", tempo);
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);


		if(noPartida_inicio!=NULL){
			if(tempo == noPartida_inicio->partida->initial_time){
				//printf("=========>>> CRIAR THREAD VOO PARTIDA\n");
				sem_wait(sem_tempo);
				//pthread_create(&thread_voo, NULL, gerirVooP, NULL);
				criaThreadVoo();
				sem_post(sem_tempo);
			}
		}
		if(noChegada_inicio!=NULL){
			if(tempo == noChegada_inicio->chegada->initial_time){
				//printf("=========>>> CRIAR THREAD VOO CHEGADA\n");
				sem_wait(sem_tempo);
				//pthread_create(&thread_voo, NULL, gerirVooC, NULL);
				criaThreadVoo();
				sem_post(sem_tempo);
			}
		}
	}
	pthread_exit(NULL);
}

// NAMED PIPE SERVER
void *readPipe() {
	// cria named pipe se nao existir
	if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)){
		perror("erro a criar criar named pipe ");
		exit(0);
	}else{
		printf("Input pipe criado!\n");
	}

	// abrir para leitura
	if ((readInputPipe = open(PIPE_NAME, O_RDONLY)) < 0) {
		perror("erro: nao abre pipe para leitura... ");
		exit(0);
	} else{
		printf("Pipe aberto para leitura...\n");
	}

	// ===================================================================
	//abre pipe para escrita	
	fd = open(PIPE_NAME, O_WRONLY);
	if(fd < 0) {
		perror("ERRO a escrever no pipe...");
		exit(0);
	}
	else{
		printf("Input pipe opened for writing.\n");
	}
	// ==================================================================
	
	char flight_code[CODE_SIZE];
	int takeoff_time; //departure
	int initial_time;
	int time_to_runway, initial_fuel; // arrive

	//signal(SIGINT, endprogram); //ctrl + C

	int n = 0;
	bool newCode = true;

	while(1){
		printf(" ---> A esperar por comando do client ...\n");

		char tipo[BUFFER_SIZE];
		char comando[BUFFER_SIZE];
		char newFly[20][20];
		int j=0,word=0;

		read(readInputPipe, comando, sizeof(comando));

		for(int i=0;i<=(strlen(comando));i++){
	        // dividir o comando por palavras
	        if(comando[i]==' '|| comando[i]=='\0'){
	            newFly[word][j]='\0';
	            word++;  //proxima palavra
	            j=0;    //proxima palavra -> indice 0
	        }
	        else{
	            newFly[word][j]=comando[i];
	            j++;
	        }
	    }

	    //printf("\n A enviar novo comando.... \n");
	    strcpy(tipo,newFly[0]);
    	if(strcmp(tipo,"DEPARTURE") == 0){
    		//printf("\nPartida...\n");
    		strcpy(flight_code, newFly[1]);
    		initial_time = atoi(newFly[3]);
    		takeoff_time = atoi(newFly[5]);

    		// coloca VOO na fila de espera -> criar thread voo no instante do voo
    		// guardar flight code ... e verificar se nao ha nenhum igual
    		// ---> strcmp(flight_code, codigoVoo) -> codigoVoo é o codigo que está na fila de espera
			
			if(n==1){
	    		queuePartida *t = NULL;
				t = noPartida_inicio;
				while(t!= NULL){
					//printf("verifica new flight code ...\n");
					if(strcmp(flight_code,t->partida->flight_code) == 0){
						// codigo ja introduzido em lista de espera...
						newCode = false;
					}
					else{
						newCode = true;
					}
					//printf("%s %d \n", t->partida->flight_code, t->partida->initial_time);
					t=t->next;
				}
			}
			n=1; // ja meteu algo na fila de espera
			
			//DEPARTURE TP440 init: 0 takeoff: 100
			if( (newCode == true) && (strcmp(newFly[2],"init:") == 0) && (strcmp(newFly[4],"takeoff:") == 0)){
				if(initial_time >= 0 && takeoff_time >= initial_time && tempo <= initial_time){
		    		criaVooPartida(flight_code, initial_time, takeoff_time);

		    		sem_wait(semLOG);
					char msg[BUFFER_SIZE] = "NEW COMMAND  ==>  ";
					strcat(msg,comando);
					writeLOG(msg);
					sem_post(semLOG);
	    		}else{
	    			sem_wait(semLOG);
	    			printf("Valores departure invalidos ... tempo ja passou ... initial_time >= 0 and takeoff_time >= initial_time\n");
	    			char msg[BUFFER_SIZE] = "WRONG COMMAND  ==>  ";
	    			strcat(msg,comando);
					writeLOG(msg);
					sem_post(semLOG);
	    		}
			}else{
				sem_wait(semLOG);
				printf("Comando DEPARTURE invalido... flight code ja inserido ou erro de sintaxe\n");
				char msg[BUFFER_SIZE] = "WRONG COMMAND  ==>  ";
				strcat(msg,comando);
				writeLOG(msg);
				sem_post(semLOG);
			}
    		/*
		    printf("Flight code: %s\n",flight_code);
			printf("Initial time: %d\n",initial_time);
			printf("Takeoff time: %d\n",takeoff_time);
			*/
    	}
    	
    	else if(strcmp(tipo,"ARRIVAL") == 0){
    		//printf("\nChegada...\n");
    		strcpy(flight_code, newFly[1]);
    		initial_time = atoi(newFly[3]);
    		time_to_runway = atoi(newFly[5]);
    		initial_fuel = atoi(newFly[7]);

    		// flight code ... e verificar se nao ha nenhum igual
    		// coloca VOO na fila de espera -> criar thread voo no instante do voo

			if(n==1){
	    		queueChegada *t = NULL;
				t = noChegada_inicio;
				while(t!= NULL){
					//printf("verifica new flight code ...\n");
					if(strcmp(flight_code,t->chegada->flight_code) == 0){
						// codigo ja introduzido em lista de espera...
						newCode = false;
					}
					else{
						newCode = true;
					}
					//printf("%s %d \n", t->partida->flight_code, t->partida->initial_time);
					t=t->next;
				}
			}
			n=1; // ja meteu algo na fila de espera
			
    		//ARRIVAL TP437 init: 0 eta: 100 fuel: 1000
    		if( (newCode == true) && (strcmp(newFly[2],"init:") == 0) && (strcmp(newFly[4],"eta:") == 0) && (strcmp(newFly[6],"fuel:") == 0)){
    			if(initial_time>=0 && time_to_runway > 0 && initial_fuel > 0 && tempo <= initial_time && initial_fuel >= time_to_runway) {
	    			criaVooChegada(flight_code, initial_time, time_to_runway, initial_fuel);

	    			sem_wait(semLOG);
	    			char msg[BUFFER_SIZE] = "NEW COMMAND  ==>  ";
					strcat(msg,comando);
					writeLOG(msg);
					sem_post(semLOG);

	    		} else{
	    			sem_wait(semLOG);
	    			printf("Valores arrival invalidos ... tempo ja passou ... initial_time>=0 and time_to_runway > 0 and initial_fuel > 0 ... nao tem fuel\n");
	    			char msg[BUFFER_SIZE] = "WRONG COMMAND  ==>  ";
	    			strcat(msg,comando);
					writeLOG(msg);
					sem_post(semLOG);
	    		}
    		}else{
    			sem_wait(semLOG);
    			printf("Comando ARRIVAL invalido...\n");
    			char msg[BUFFER_SIZE] = "WRONG COMMAND  ==>  ";
    			strcat(msg,comando);
				writeLOG(msg);
				sem_post(semLOG);
    		}
    		/*
    		printf("Flight code: %s\n",flight_code);
			printf("Initial time: %d\n",initial_time);
			printf("Time to runway (ETA): %d\n",time_to_runway); //ETA
			printf("Initial Fuel: %d\n",initial_fuel);
			*/
    	}
    	else{
    		sem_wait(semLOG);
    		printf("Comando invalido...\n");
    		char msg[BUFFER_SIZE] = "WRONG COMMAND  ==>  ";
    		strcat(msg,comando);
    		writeLOG(msg);
    		sem_post(semLOG);
    	}
	}	
}

// -------- VOO PARTIDA -----------
int criaVooPartida(char *code, int initReady, int initWant){
	partida = (VooPartida*) malloc(sizeof(VooPartida));

	strcpy(partida->flight_code, code);
	partida->initial_time = initReady;
	partida-> takeoff_time = initWant;

	printf("NOVO VOO PARTIDA .... %s %d \n", partida->flight_code, partida->initial_time);

	inserir_filaPartida(partida, initReady);
	printQueuePartida();

	//criaThreadVoo(); // meter no le pipe ... ???

	//printf(" --> voo partida %s colocado em fila de espera com SUCESSO!\n", partida->flight_code);
	return 0;
}

queuePartida* inserir_filaPartida(VooPartida *p, int init){
	/* https://stackoverflow.com/questions/21788598/c-inserting-into-linked-list-in-ascending-order */
	printf("Tamanho da fila de espera: %d\n", tamFilaPartidas);
	
	if(tamFilaPartidas <= config->maxPartidas){
		tamFilaPartidas++;
		
		// se a cabeca da lista ainda nao foi criada... cria
		if(noPartida_inicio == NULL){
			noPartida_inicio = (queuePartida*)malloc(sizeof(queuePartida));
			noPartida_inicio->partida = p;
			noPartida_inicio->next = NULL;
			printf("new head partida\n");
			return noPartida_inicio;
		}

		//cria novo no da lista ligada
		//printf("--1--\n");
		partida_atual = (queuePartida*)malloc(sizeof(queuePartida));
		partida_atual->partida = p;
		partida_atual->next = NULL;

		//se o tempo do novo voo for superior ao do cabecalho da lista -> substitui cabecalhado
		if (noPartida_inicio->partida->initial_time > init) {
			//printf("---2--\n");
		    partida_atual->next = noPartida_inicio;
		    noPartida_inicio = partida_atual;

		    return partida_atual;
		}
		
		//pesquisa o sitio certo da lista e adiciona o novo no -> de forma ordenada por ordem crescente do INIT
		queuePartida *temp;
		queuePartida *prev;

		temp = noPartida_inicio;
		while(temp != NULL && temp->partida->initial_time <= init) {
			//printf("---3--\n");
		    prev = temp;
		    temp = temp->next;
		    //printf("---4--\n");
		}
		//printf("---5--\n");
		partida_atual->next = temp;
		prev->next = partida_atual;
		//printf("-----\n");

		//printf("Voo de partida %s %d inserido na fila de espera com sucesso!\n", partida_atual->partida->flight_code, partida_atual->partida->initial_time);
		//printf("Voo para deslocar no topo da fila: %s %d \n\n", noPartida_inicio->partida->flight_code, partida_atual->partida->initial_time);

		//criaThreadVooPartida(tamFilaPartidas);
		return noPartida_inicio;
	}else{
		sem_wait(semLOG);
		char msg[BUFFER_SIZE] = "VOO REJEITADO => capacidade maxima de partidas";
		writeLOG(msg);		
		printf("ALERT: numero maximo de partidas do aeroporto ATINGIDO!\nAviao desviado para outro aeroporto...\n");
		sem_post(semLOG);
		stats->nrVoosRejeitados++;
		return noPartida_inicio;
	}
	//printf("tam: %d\n",tam);
	printf("end insert...\n");
	return noPartida_inicio;
}


queuePartidaPISTA* inserir_filaPartidaPISTA(slotVooP *p, int takeoff){
	printf("Tamanho da fila PISTA partida espera: %d\n", tamFilaPartidasPISTA);
	
	tamFilaPartidasPISTA++;
	
	// se a cabeca da lista ainda nao foi criada... cria
	if(noPartida_inicioPISTA == NULL){
		noPartida_inicioPISTA = (queuePartidaPISTA*)malloc(sizeof(queuePartidaPISTA));
		noPartida_inicioPISTA->partidaPISTA = p;
		noPartida_inicioPISTA->next = NULL;
		printf("new head partida PISTA\n");
		return noPartida_inicioPISTA;
	}

	//cria novo no da lista ligada
	//printf("--1--\n");
	partida_atualPISTA = (queuePartidaPISTA*)malloc(sizeof(queuePartidaPISTA));
	partida_atualPISTA->partidaPISTA = p;
	partida_atualPISTA->next = NULL;

	//se o tempo do novo voo for superior ao do cabecalho da lista -> substitui cabecalhado
	if (noPartida_inicioPISTA->partidaPISTA->takeoff > takeoff) {
		//printf("---2--\n");
	    partida_atualPISTA->next = noPartida_inicioPISTA;
	    noPartida_inicioPISTA = partida_atualPISTA;

	    return partida_atualPISTA;
	}
	
	//pesquisa o sitio certo da lista e adiciona o novo no -> de forma ordenada por ordem crescente do INIT
	queuePartidaPISTA *temp;
	queuePartidaPISTA *prev;

	temp = noPartida_inicioPISTA;
	while(temp != NULL && temp->partidaPISTA->takeoff <= takeoff) {
		//printf("---3--\n");
	    prev = temp;
	    temp = temp->next;
	    //printf("---4--\n");
	}
	//printf("---5--\n");
	partida_atualPISTA->next = temp;
	prev->next = partida_atualPISTA;
	//printf("-----\n");

	//printf("Voo de partida %s %d inserido na fila de espera com sucesso!\n", partida_atual->partida->flight_code, partida_atual->partida->initial_time);
	//printf("Voo para deslocar no topo da fila: %s %d \n\n", noPartida_inicio->partida->flight_code, partida_atual->partida->initial_time);

	//printf("tam: %d\n",tam);
	printf("end insert...\n");
	return noPartida_inicioPISTA;
}

void remover_filaPartida(){ 
	//retira o cabecalho da lista ligada
	queuePartida *temp = noPartida_inicio;
	queuePartida *node; // ponteiro para guardar o proximo node do node que vamos remover

	if(temp!=NULL){ 
		// se a lista ligada tiver no cabecalho
		node=temp->next; 
		free(temp); // liberta memoria do no que vamos remover
		noPartida_inicio=node; // novo head é o proximo node do node removido
	}
    else{
    	printf("\nFila de espera de partidas vazia...");
	}
	tamFilaPartidas--;
	//printf("head partida removido...\n");
}

void remover_filaPartidaPISTA(){ 
	//retira o cabecalho da lista ligada
	queuePartidaPISTA *temp = noPartida_inicioPISTA;
	queuePartidaPISTA *node; // ponteiro para guardar o proximo node do node que vamos remover

	if(temp!=NULL){ 
		// se a lista ligada tiver no cabecalho
		node=temp->next; 
		free(temp); // liberta memoria do no que vamos remover
		noPartida_inicioPISTA=node; // novo head é o proximo node do node removido
	}
    else{
    	printf("\nFila de espera de partidas PISTA vazia...");
	}
	printf("head partida PISTA removido...\n");
	tamFilaPartidasPISTA--;
	//printQueuePartidaPISTA();
}

void printQueuePartida(){
	printf("---> FILA PARTIDAS\n");
	queuePartida *temp = NULL;

	temp = noPartida_inicio;
	while(temp!= NULL){
		printf("%s %d \n", temp->partida->flight_code, temp->partida->initial_time);
		temp=temp->next;
	}
	printf("---------------------\n");
}

void printQueuePartidaPISTA(){
	printf(">>>>>>>> FILA PARTIDAS PISTA\n");
	queuePartidaPISTA *temp = NULL;

	temp = noPartida_inicioPISTA;
	while(temp!= NULL){
		printf("%s %d \n", temp->partidaPISTA->flight_code, temp->partidaPISTA->takeoff);
		temp=temp->next;
	}
	printf("---------------------\n");

}

// ================================================================================================
// ============================= VOO CHEGADA ======================================================
// ================================================================================================

int criaVooChegada(char *code, int initReady, int eta, int fuel){
	// eta -> tempo até à pista
	chegada = (VooChegada*) malloc(sizeof(VooChegada));

	strcpy(chegada->flight_code, code);
	chegada->initial_time = initReady;
	chegada->time_to_runway = eta;
	chegada->initial_fuel = fuel;

	printf("NOVO VOO CHEGADA .... %s %d %d\n", chegada->flight_code, chegada->initial_time, chegada->time_to_runway);

	inserir_filaChegada(chegada,initReady);
	printQueueChegada();

	//criaThreadVoo(); // meter no le pipe ... ??? 

	//printf("---> voo chegada %s colocado em fila de espera\n", chegada->flight_code);
	return 0;
}

queueChegada* inserir_filaChegada(VooChegada *c, int init){
	/* https://stackoverflow.com/questions/21788598/c-inserting-into-linked-list-in-ascending-order */
	printf("Tamanho da fila de espera: %d\n", tamFilaChegadas);
	
	if(tamFilaChegadas <= config->maxChegadas){
		tamFilaChegadas++;
		
		// se a cabeca da lista ainda nao foi criada... cria
		if(noChegada_inicio == NULL){
			noChegada_inicio = (queueChegada*)malloc(sizeof(queueChegada));
			noChegada_inicio->chegada = c;
			noChegada_inicio->next = NULL;
			printf("new head chegada\n");
			return noChegada_inicio;
		}

		//cria novo no da lista ligada
		//printf("--1--\n");
		chegada_atual = (queueChegada*)malloc(sizeof(queueChegada));
		chegada_atual->chegada = c;
		chegada_atual->next = NULL;

		//se o tempo do novo voo for superior ao do cabecalho da lista -> substitui cabecalhado
		if (noChegada_inicio->chegada->initial_time > init) {
			//printf("---2--\n");
		    chegada_atual->next = noChegada_inicio;
		    noChegada_inicio = chegada_atual;

		    return chegada_atual;
		}
		
		//pesquisa o sitio certo da lista e adiciona o novo no -> de forma ordenada por ordem crescente do INIT
		queueChegada *temp;
		queueChegada *prev;

		temp = noChegada_inicio;
		while(temp != NULL && temp->chegada->initial_time <= init) {
			//printf("---3--\n");
		    prev = temp;
		    temp = temp->next;
		    //printf("---4--\n");
		}
		//printf("---5--\n");
		chegada_atual->next = temp;
		prev->next = chegada_atual;
		//printf("-----\n");

		//printf("Voo de partida %s %d inserido na fila de espera com sucesso!\n", partida_atual->partida->flight_code, partida_atual->partida->initial_time);
		//printf("Voo para deslocar no topo da fila: %s %d \n\n", noPartida_inicio->partida->flight_code, partida_atual->partida->initial_time);

		//criaThreadVooPartida(tamFilaPartidas);
		return noChegada_inicio;
	}else{
		sem_wait(semLOG);
		char msg[BUFFER_SIZE] = "VOO REJEITADO => capacidade maxima de chegadas -> aviao desviado para outro aeroporto";
		writeLOG(msg);		
		printf("ALERT: numero maximo de chegadas do aeroporto ATINGIDO!\nAviao desviado para outro aeroporto...\n");
		sem_post(semLOG);
		stats->nrVoosRejeitados++;
		return noChegada_inicio;
	}
	//printf("tam: %d\n",tam);
	printf("end insert...\n");
	return noChegada_inicio;
}


queueChegadaPISTA* inserir_filaChegadaPISTA(slotVooC *c, int tcheg){
	//printf(" >>>> Tamanho da fila de espera PISTA chegada: %d\n", tamFilaChegadasPISTA);
	
	// while ?? --> o voo q nao entrar fica fila de espera (HOLDING) enquanto tiver FUEL
	
	tamFilaChegadasPISTA++;

	// se a cabeca da lista ainda nao foi criada... cria
	if(noChegada_inicioPISTA == NULL){
		noChegada_inicioPISTA = (queueChegadaPISTA*)malloc(sizeof(queueChegadaPISTA));
		noChegada_inicioPISTA->chegadaPISTA = c;
		noChegada_inicioPISTA->next = NULL;
		printf("new head chegada PISTA\n");
		return noChegada_inicioPISTA;
	}

	//cria novo no da lista ligada
	//printf("--1--\n");
	chegada_atualPISTA = (queueChegadaPISTA*)malloc(sizeof(queueChegadaPISTA));
	chegada_atualPISTA->chegadaPISTA = c;
	chegada_atualPISTA->next = NULL;

	//se o tempo do novo voo for superior ao do cabecalho da lista -> substitui cabecalhado

	if ( (noChegada_inicioPISTA->chegadaPISTA->tchegada) > tcheg) {
		//printf("---2--\n");
	    chegada_atualPISTA->next = noChegada_inicioPISTA;
	    noChegada_inicioPISTA = chegada_atualPISTA;

	    return chegada_atualPISTA;
	}
	
	//pesquisa o sitio certo da lista e adiciona o novo no -> de forma ordenada por ordem crescente do INIT
	queueChegadaPISTA *temp;
	queueChegadaPISTA *prev;

	temp = noChegada_inicioPISTA;
	while(temp != NULL && (noChegada_inicioPISTA->chegadaPISTA->tchegada) <= tcheg) {
		//printf("---3--\n");
	    prev = temp;
	    temp = temp->next;
	    //printf("---4--\n");
	}
	//printf("---5--\n");
	chegada_atualPISTA->next = temp;
	prev->next = chegada_atualPISTA;
	//printf("-----\n");

	//printf("Voo de partida %s %d inserido na fila de espera com sucesso!\n", partida_atual->partida->flight_code, partida_atual->partida->initial_time);
	//printf("Voo para deslocar no topo da fila: %s %d \n\n", noPartida_inicio->partida->flight_code, partida_atual->partida->initial_time);

	//criaThreadVooPartida(tamFilaPartidas);
	return noChegada_inicioPISTA;
}

void remover_filaChegada(){
	//retira o cabecalho da lista ligada
	queueChegada *temp = noChegada_inicio;
	queueChegada *node; // ponteiro para guardar o proximo node do node que vamos remover

	if(temp!=NULL){ 
		// se a lista ligada tiver no cabecalho
		node=temp->next; 
		free(temp); // liberta memoria do no que vamos remover
		noChegada_inicio=node; // novo head é o proximo node do node removido
	}
    else{
    	printf("\nFila de espera de chegadas vazia...");
	}
	tamFilaChegadas--;
	//printf("head chegada removido...\n");
}

void remover_filaChegadaPISTA(){
	//retira o cabecalho da lista ligada
	queueChegadaPISTA *temp = noChegada_inicioPISTA;
	queueChegadaPISTA *node; // ponteiro para guardar o proximo node do node que vamos remover

	if(temp!=NULL){ 
		// se a lista ligada tiver no cabecalho
		node=temp->next; 
		free(temp); // liberta memoria do no que vamos remover
		noChegada_inicioPISTA=node; // novo head é o proximo node do node removido
	}
    else{
    	printf("\nFila de espera de chegadas PISTA vazia...");
	}
	printf("head PISTA chegada removido...\n");
	tamFilaChegadas = tamFilaChegadas - 1;
}

void printQueueChegada(){
	printf("---> FILA CHEGADAS\n");
	queueChegada *temp = NULL;

	temp = noChegada_inicio;
	while(temp!= NULL){
		printf("%s %d %d \n", temp->chegada->flight_code, temp->chegada->initial_time, temp->chegada->time_to_runway);
		temp=temp->next;
	}
	printf("---------------------\n");
}


void printQueueChegadaPISTA(){
	printf(" >>>>>>>>>> FILA PISTA CHEGADAS\n");
	queueChegadaPISTA *temp = NULL;

	temp = noChegada_inicioPISTA;
	while(temp!= NULL){
		printf("%s %d %d \n", temp->chegadaPISTA->flight_code, temp->chegadaPISTA->eta, temp->chegadaPISTA->tchegada);
		temp=temp->next;
	}
	printf("----------------------------------------------\n");
}


queueChegadaPISTA* insere_emergencia(slotVooC *c){
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> insere VOO de EMERGENCIA <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	printQueueChegadaPISTA();
	printf(">>>>>>>>>>>>>>>>>>>>>");
	tamFilaChegadasPISTA++;

	chegada_atualPISTA = (queueChegadaPISTA*)malloc(sizeof(queueChegadaPISTA)); // ????
	chegada_atualPISTA->chegadaPISTA = c;
	chegada_atualPISTA->next = NULL;

    chegada_atualPISTA->next = noChegada_inicioPISTA;
    noChegada_inicioPISTA = chegada_atualPISTA;

    printf("... inserido VOO de EMERGENCIA\n");
    return chegada_atualPISTA;
}


// --------------------------------------------------------------------

void *gerirVooP(){ //void* idp
	
	printf("NEW DEPARTURE FLY\n");

	//adiciona voo às stats;
	stats->nrVoos += 1;
	nrPartidas++;

	int t = noPartida_inicio->partida->takeoff_time;

	sem_wait(sem_voo);
	inserir_filaPartidaPISTA(partidaPISTA,t);
	printQueuePartidaPISTA();	
	sem_post(sem_voo);

	// envia msg à torre de controlo , através da msq com instante desejado de partida
	message msg;

	msg.mtype = SLOT;

	strcpy(msg.flight_code, noPartida_inicio->partida->flight_code);

	msg.takeoff = t;

	msg.readyP = false;

	//partidaPISTA->ready = false;
	//partidaPISTA->descolagem = false;

	msgsnd(mqid, &msg, sizeof(msg) - sizeof(long), 0);

	printf("-----> ENVIAR flight_code: %s\n", msg.flight_code);
	printf("-----> ENVIAR takeoff: %d\n", msg.takeoff);
	printf("-----> mtype : %ld \n", msg.mtype);

	/* Espera receber uma mensagem com o seu id (=VOO) */
	msgrcv(mqid, &msg, sizeof(msg) - sizeof(long), VOO, 0);

	printf("[VOO] Recebi uma mensagem!\n");
	printf(" MEU SLOT = %ld\n", msg.slot);
	printf("---------------------------\n");

	/*
	pthread_mutex_lock(&mutexVoo);
	inserir_filaPartidaPISTA(partidaPISTA,noPartida_inicio->partida->takeoff_time);
	printQueuePartidaPISTA();	
	pthread_mutex_unlock(&mutexVoo);
	*/ 

    pthread_mutex_lock(&mutex); 
    while(tempo < t + config->duracaoDescolagem){ //+ config->duracaoDescolagem
    	pthread_cond_wait(&cond, &mutex); 
    }
    pthread_mutex_unlock(&mutex); 
    
    // ------- passou tempo
    /*
    printf("......\n");
    pthread_mutex_lock(&pmutex);
    partidaPISTA->ready = true;
    printf("... ready\n");
    pthread_cond_broadcast(&pcond);
    pthread_mutex_unlock(&pmutex);*/

    //printf("... ready....\n");
	msg.mtype  = GO;
    msg.readyP = true;
    msgsnd(mqid, &msg, sizeof(msg) - sizeof(long), 0);
    
    //printf("%d\n",partidaPISTA->ready);
	/* Espera receber uma mensagem com o seu id (=VOO) */
	msgrcv(mqid, &msg, sizeof(msg) - sizeof(long), READY, 0);

	/*
    pthread_mutex_lock(&mutexVoo); 
    while(partidaPISTA->descolagem == false){
    	pthread_cond_wait(&condVoo, &mutexVoo); 
    }
    pthread_mutex_unlock(&mutexVoo); 
	*/

    if(partidaPISTA->descolagem == true){
    	stats->nrTotalVoosDescolados++;
    	if(partidaPISTA->pista==1){
			P1=false;
		}else if(partidaPISTA->pista==2){
			P2=false;
		}
		remover_filaPartidaPISTA();
		printQueuePartidaPISTA();

		partidaPISTA->descolagem = false;

		pthread_exit(NULL);
    }

	return NULL;
}

void *gerirVooC(){

	printf("NEW ARRIVAL FLY \n");

	//adiciona voo às stats;
	stats->nrVoos += 1;
	nrAterragens++;

	int tINIT = noChegada_inicio->chegada->initial_time;
	int eta = noChegada_inicio->chegada->time_to_runway;
	int fuel = noChegada_inicio->chegada->initial_fuel;

	int tcheg = tINIT + eta;
	printf("--> %d + %d = chegada %d ut\n",tINIT, eta, tcheg);

	sem_wait(sem_voo);
	inserir_filaChegadaPISTA(chegadaPISTA,tcheg);
	printQueueChegadaPISTA();
	sem_post(sem_voo);

	// envia msg à torre de controlo , através da msq com o ETA e o combustivel
	// envia msg à torre de controlo , através da msq com instante desejado de partida
	message msg;
	msg.mtype = SLOT;

	strcpy(msg.flight_code, noChegada_inicio->chegada->flight_code);
	msg.tchegada = tcheg;
	msg.eta = eta;
	msg.fuel = fuel;

	// 1 ut =  - 1 fuel
	// ETA ==> tempo até à pista
	
	msg.readyC = false;

	msgsnd(mqid, &msg, sizeof(msg) - sizeof(long), 0);

	printf("------> ENVIAR flight_code: %s \n", msg.flight_code);
	printf("------> ENVIAR ETA: %d \n", msg.eta);
	printf("------> ENVIAR tempo CHEGADA: %d \n", msg.tchegada);
	printf("------> ENVIAR fuel: %d \n", msg.fuel);
	printf("------> ENVIAR emergencia: %d \n", msg.emergencia);
	printf("------> mtype: %ld \n", msg.mtype);
	
	/* Espera receber uma mensagem com o seu id (=VOO) */
	msgrcv(mqid, &msg, sizeof(msg) - sizeof(long), VOO, 0);

	printf("[VOO] Recebi uma mensagem!\n");
	printf(" MEU SLOT = %ld\n", msg.slot);
	printf(" EMERGENCIA = %d\n", msg.emergencia);
	printf(" HOLDING = %d\n", msg.holding);
	printf("---------------------------\n");

	/*
	sem_wait(sem_voo);
	inserir_filaChegadaPISTA(chegadaPISTA,msg.tchegada);
	printQueueChegadaPISTA();
	sem_post(sem_voo);
	*/

	//printf("... a voar: %s %d %d\n", chegadaPISTA->flight_code,chegadaPISTA->tchegada, chegadaPISTA->fuel);

	//printf("~~~~~~~ tempo %d \n", tempo);

	if(msg.emergencia == true){
		contaEmergencias++;
		sem_wait(sem_voo);
		printf("emergencia!!!!!\n");
		// funcao insere novo cabecalho!!!
		insere_emergencia(chegadaPISTA);
		printQueueChegadaPISTA();
		sem_post(sem_voo);
	}

	// ------------- PISTA -----------------------------
    pthread_mutex_lock(&mutex); 
    while(tempo < tcheg + config->duracaoAterragem){ // + config->duracaoAterragem
    	pthread_cond_wait(&cond, &mutex); 
    	//printf("... a voar: %s %d %d\n", chegadaPISTA->flight_code,chegadaPISTA->tchegada, chegadaPISTA->fuel);
    	//printf("~~~~~~~ tempo %d \n", tempo);
    	int tHOLD = config->holdingMin;
    	if(msg.holding == true){
    		contaHolding++;
    		stats->nrMedioHoldingAterragem = contaHolding/nrAterragens;
    		while(tHOLD <= config->holdingMax && tamFilaChegadasPISTA > 5){
    			tHOLD++;
    			chegadaPISTA->tchegada = chegadaPISTA->tchegada + tHOLD;
    			chegadaPISTA->fuel = chegadaPISTA->fuel - 1;
    			if(chegadaPISTA->fuel < 0){
    				stats->nrVoosRedirecionados++;
    				char log[BUFFER_SIZE];
					sem_wait(semLOG);
					sprintf(log,"%s LEAVING TO OTHER AIRPORT",chegadaPISTA->flight_code);
					writeLOG(log);
					printf("%s LEAVING TO OTHER AIRPORT => FUEL < 0\n", chegadaPISTA->flight_code);
					sem_post(semLOG);
		    		//printf("Aviao desviado para outro aeroporto...\n");
		    		remover_filaChegadaPISTA();
					printQueueChegadaPISTA();
		    		pthread_exit(NULL);
		    	}
    		}
    		if(tamFilaChegadasPISTA<=5){
    			inserir_filaChegadaPISTA(chegadaPISTA, chegadaPISTA->tchegada);
    		}else{
    			// passou holding MAX
    			stats->nrVoosRedirecionados++;
    			char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s LEAVING TO OTHER AIRPORT",chegadaPISTA->flight_code);
				writeLOG(log);
				printf("%s LEAVING TO OTHER AIRPORT \n", chegadaPISTA->flight_code);
				sem_post(semLOG);
    			//printf("Voo desviado para outro aeroporto\n");
    			remover_filaChegadaPISTA();
				printQueueChegadaPISTA();
    			pthread_exit(NULL);
    		}
    	}else if(msg.emergencia == true){
    		// a média da quantidade de voos que foram 
    		//colocados em holding devido às entrada de um voo de emergência

    		// se um voo é colocado em estado de emergencia => inicio QUEUE 
    		// => 5º elemento passa a ser o 6º elemento => HOLDING
    		if(tamFilaChegadasPISTA >= 5){
    			contaHoldingEmergencia++;
    			stats->nrMedioHoldingUrgencia = contaHoldingEmergencia/contaEmergencias;

    			while(tHOLD <= config->holdingMax && tamFilaChegadasPISTA > 5){
	    			tHOLD++;
	    			chegadaPISTA->tchegada = chegadaPISTA->tchegada + tHOLD;
	    			chegadaPISTA->fuel = chegadaPISTA->fuel - 1;
	    			if(chegadaPISTA->fuel < 0){
	    				stats->nrVoosRedirecionados++;
			    		//printf("Aviao desviado para outro aeroporto...\n");
			    		char log[BUFFER_SIZE];
						sem_wait(semLOG);
						sprintf(log,"%s LEAVING TO OTHER AIRPORT",chegadaPISTA->flight_code);
						writeLOG(log);
						printf("%s LEAVING TO OTHER AIRPORT => FUEL < 0\n", chegadaPISTA->flight_code);
						sem_post(semLOG);
			    		remover_filaChegadaPISTA();
						printQueueChegadaPISTA();
			    		pthread_exit(NULL);
			    	}
	    		}
	    		if(tamFilaChegadasPISTA<=5){
	    			inserir_filaChegadaPISTA(chegadaPISTA, chegadaPISTA->tchegada);
	    		}else{
	    			// passou holding MAX
	    			stats->nrVoosRedirecionados++;
	    			char log[BUFFER_SIZE];
					sem_wait(semLOG);
					sprintf(log,"%s LEAVING TO OTHER AIRPORT",chegadaPISTA->flight_code);
					writeLOG(log);
					printf("%s LEAVING TO OTHER AIRPORT\n", chegadaPISTA->flight_code);
					sem_post(semLOG);

	    			//printf("Voo REJEITADO! Desvio para outro aeroporto\n");
	    			remover_filaChegadaPISTA();
					printQueueChegadaPISTA();
	    			pthread_exit(NULL);
	    		}
    		}
    		
    	}
    	chegadaPISTA->fuel = chegadaPISTA->fuel - 1;
    	if(chegadaPISTA->fuel < 0){
    		char log[BUFFER_SIZE];
			sem_wait(semLOG);
			sprintf(log,"%s LEAVING TO OTHER AIRPORT",chegadaPISTA->flight_code);
			writeLOG(log);
			printf("%s LEAVING TO OTHER AIRPORT => FUEL < 0\n", chegadaPISTA->flight_code);
			sem_post(semLOG);
    		//printf("Aviao desviado para outro aeroporto...\n");

    		remover_filaChegadaPISTA();
			printQueueChegadaPISTA();
    		pthread_exit(NULL);
    	}
    }

    pthread_mutex_unlock(&mutex); 

	msg.mtype  = GO; //GO //SLOT
    msg.readyC = true;

    msgsnd(mqid, &msg, sizeof(msg) - sizeof(long), 0);
    //printf("%d\n",partidaPISTA->ready);
	/* Espera receber uma mensagem com o seu id (=VOO) */
	msgrcv(mqid, &msg, sizeof(msg) - sizeof(long), READY, 0); //READY //VOO

    if(chegadaPISTA->aterragem == true){
    	stats->nrVoosAterrados++;
    	remover_filaChegadaPISTA();
		printQueueChegadaPISTA();

		if(chegadaPISTA->pista == 1){
			C1=false;
		}else if(chegadaPISTA->pista == 2){
			C2=false;
		}

		chegadaPISTA->aterragem = false;

		pthread_exit(NULL);
    }
	return NULL;
}


int criaThreadVoo(){ 
	printf("========>>> CRIAR THREAD VOO \n");
	//printf("->UT: %d\n",tempo);

	// PARTIDAS
	if(noPartida_inicio != NULL){ 
		printQueuePartida();
		
		//printf("t = %d\n", tempo);
		//printf("voo: %s %d \n", noPartida_inicio->partida->flight_code, noPartida_inicio->partida->initial_time);
		
		if(pthread_create(&thread_voo, NULL, gerirVooP, NULL)){
			perror("erro a criar thread partida...");
			//exit(1);
		}

		sem_wait(semLOG);
		char msg[BUFFER_SIZE];
		sprintf(msg,"%s DEPARTURE started",noPartida_inicio->partida->flight_code);
		writeLOG(msg);
		printf("DEPARTURE %s %d started\n", noPartida_inicio->partida->flight_code, noPartida_inicio->partida->initial_time);
		sem_post(semLOG);

		//int t = noPartida_inicio->partida->takeoff_time;
		remover_filaPartida(); 
	}

	// CHEGADAS
	if(noChegada_inicio != NULL){ 
		printQueueChegada();
		
		//printf("-->tempo = %d\n", tempo);
		//printf("voo: %s %d \n", noChegada_inicio->chegada->flight_code, noChegada_inicio->chegada->initial_time);
		if(pthread_create(&thread_voo, NULL, gerirVooC, NULL)){
			perror("erro a criar thread chegada...");
			//exit(1);
		}

		sem_wait(semLOG);
		char msg[BUFFER_SIZE];
		sprintf(msg,"%s ARRIVAL started",noChegada_inicio->chegada->flight_code);
		writeLOG(msg);
		printf("ARRIVAL %s %d started\n", noChegada_inicio->chegada->flight_code, noChegada_inicio->chegada->initial_time);
		sem_post(semLOG);

		//int tINIT = noChegada_inicio->chegada->initial_time;
		//int eta = noChegada_inicio->chegada->time_to_runway;

		//int tcheg = tINIT + eta;

		remover_filaChegada();	
	}
	//printf("--> tempo = %d\n", tempo);
	
	return 0;
}

// colocar as coisas na SHM
void createSHM(){
	//CRIAR MEMORIA PARTILHADA

	// estatisticas
	shmid_stats = shmget(IPC_PRIVATE, sizeof(stats_struct), IPC_CREAT | 0700);  // 0766 ??
	if(shmid_stats < 0){
		perror("ERRO ao criar SHM stats\n");
		exit(0);
	}else{
		printf("SHM stats created! \n");
	}
	// Attach shared memory
	stats = (stats_struct *)shmat(shmid_stats, NULL, 0);	
	if (stats == (stats_struct *) - 1){
		perror("erro no mapeamento SHM stats\n");
		exit(0);
	}else{
		printf("SHM stats: Attach shared memory! \n");
	}

	int max = 0;
	if(config->maxPartidas >= config->maxChegadas){
		max = config->maxPartidas;
	}else{
		max = config->maxChegadas;
	}

	
	shm_slotP = shmget(IPC_PRIVATE, sizeof(slotVooP) * max, IPC_CREAT | 0700); 
	if(shm_slotP < 0){
		perror("ERRO ao criar SHM voo partida\n");
	}else{
		printf("SHM voo partida criada\n");
	}
	partidaPISTA = (slotVooP*)shmat(shm_slotP,NULL,0);
	if(partidaPISTA == (slotVooP*) - 1){
		perror("erro shmat voos partida\n");
		exit(1);
	}else{
		printf("SHM voo: attach shm\n");
	}

	/*
	shmHeadPISTA_p = shmget(IPC_PRIVATE, sizeof(queuePartidaPISTA) * max, IPC_CREAT | 0700); 
	if(shm_slotP < 0){
		perror("ERRO ao criar SHM voo partida\n");
	}else{
		printf("SHM voo partida criada\n");
	}
	noPartida_inicioPISTA = (queuePartidaPISTA*)shmat(shmHeadPISTA_p,NULL,0);
	if(noPartida_inicioPISTA == (queuePartidaPISTA*) - 1){
		perror("erro shmat voos partida\n");
		exit(1);
	}else{
		printf("SHM voo: attach shm\n");
	}
	*/

	/*
	shmAtualPISTA_p = shmget(IPC_PRIVATE, sizeof(queuePartidaPISTA), IPC_CREAT | 0700); 
	if(shmAtualPISTA_p < 0){
		perror("ERRO ao criar SHM voo partida\n");
	}else{
		printf("SHM voo partida criada\n");
	}
	partida_atualPISTA = (queuePartidaPISTA*)shmat(shmAtualPISTA_p,NULL,0);
	if(partida_atualPISTA == (queuePartidaPISTA*) - 1){
		perror("erro shmat voos partida\n");
		exit(1);
	}else{
		printf("SHM voo: attach shm\n");
	}
	*/

	shm_slotC = shmget(IPC_PRIVATE, sizeof(slotVooC) * max, IPC_CREAT | 0700);
	if(shm_slotC < 0){
		perror("ERRO ao criar SHM voo chegada\n");
	}else{
		printf("SHM voo chegada criada\n");
	}
	chegadaPISTA = (slotVooC*)shmat(shm_slotC,NULL,0);
	if(chegadaPISTA == (slotVooC*) - 1){
		perror("erro shmat voos chegada\n");
		exit(0);
	}else{
		printf("SHM voo: attach shm\n");
	}
	

	tempo = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0700); 
	if(tempo < 0){
		perror("ERRO ao criar SHM tempo\n");
		exit(0);
	}else{
		printf("SHM tempo created! \n");
	}
	// Attach shared memory
	shm_tempo = (int *)shmat(tempo, NULL, 0);	
	if (shm_tempo == (int *) - 1){
		perror("erro no mapeamento SHM tempo\n");
		exit(0);
	}else{
		printf("SHM tempo: Attach shared memory! \n");
	}

}

// semaforo para LOG
void createSemaphore(){
	// named semaphore
	sem_unlink("LOG");
	semLOG = sem_open("LOG", O_CREAT | O_EXCL, 0700, 1);
	if (semLOG == (sem_t *)-1){
		perror("erro na cricao do semaphore\n");
		exit(0);
	}else{
		printf("semaphore created! \n");
	}

	sem_unlink("TEMPO");
	sem_tempo = sem_open("TEMPO", O_CREAT | O_EXCL, 0700, 1);
	if (sem_tempo == (sem_t *)-1){
		perror("erro na cricao do semaphore\n");
		exit(0);
	}else{
		printf("semaphore created! \n");
	}

	sem_unlink("VOO");
	sem_voo = sem_open("VOO", O_CREAT | O_EXCL, 0700, 1);
	if (sem_voo == (sem_t *)-1){
		perror("erro na cricao do semaphore\n");
		exit(0);
	}else{
		printf("semaphore created! \n");
	}

	// init unnamed sem
	//sem_init(&semLOG,1,1);

	//sem_init(&semaforo->sem_tempo, 1, 1);
}

// criar fila de mensagens (MQ)
void createMQ(){
	/* Cria uma message queue */
	mqid = msgget(IPC_PRIVATE, IPC_CREAT|0700);
	if (mqid < 0){
		perror("Erro: Ao criar a message queue");
		exit(0);
	}else{
		printf("Message queue criada!\n");
	}
}

//inicializar estatisticas
void initStats(){
	stats->nrVoos=0;
	stats->nrVoosAterrados=0;
	stats-> tempoMedioEsperaAterragem=0.0;
	stats->nrTotalVoosDescolados = 0;
	stats->tempoMedioEsperaDescolagem=0.0;
	stats->nrMedioHoldingAterragem=0.0;
	stats->nrMedioHoldingUrgencia=0.0;
	stats->nrVoosRedirecionados=0;
	stats->nrVoosRejeitados=0;
}

// escrever estatisticas
void printStats(){
	printf("========================= Estatisticas =========================\n");
	printf("Número total de voos criados = %d\n", stats->nrVoos);
	printf("Número total de voos que aterraram = %d\n", stats->nrVoosAterrados);
	printf("Tempo médio de espera (para além do ETA) para aterrar = %lf\n",stats-> tempoMedioEsperaAterragem);
	printf("Número total de voos que descolaram = %d\n",stats->nrTotalVoosDescolados);
	printf("Tempo médio de espera para descolar = %lf\n",stats->tempoMedioEsperaDescolagem);
	printf("Número médio de manobras de holding por voo de aterragem = %lf\n",stats->nrMedioHoldingAterragem);
	printf("Número médio de manobras de holding por voo em estado de urgência = %lf\n",stats->nrMedioHoldingUrgencia);
	printf("Número de voos redirecionados para outro aeroporto = %d\n",stats->nrVoosRedirecionados);
	printf("Voos rejeitados pela Torre de Controlo = %d\n",stats->nrVoosRejeitados);
}

int init(){
	loadConfig();
	createSHM();
	createMQ();
	createSemaphore();	
	initStats();
	return 0;
}

//cria processo pai = gestor de simulacao
void simulatorManager(){
	sem_wait(semLOG);
	int p = 0;
	p = getppid();
	char msg[BUFFER_SIZE];
	sprintf(msg,"[%04d] Gestor de Simulacao pronto! ",p);
	writeLOG(msg);
	printf("[%d] Gestor de Simulacao pronto! \n",getppid());
	sem_post(semLOG);

	// comeca a contar o tempo
	tempo = 0;
	pthread_create(&thread_tempo,NULL,getTempo,NULL);
	readPipe();
}

// processo TORRE DE CONTROLO
void torreControlo(){
	sem_wait(semLOG);
	int p = 0;
	p = getppid();
	char log[BUFFER_SIZE];
	sprintf(log,"[%04d] Torre de Controlo Ativa! ",p);
	writeLOG(log);
	printf("[%d] Torre de Controlo Ativa!\n",getpid());
	sem_post(semLOG);

	message msg;

	
	partidaPISTA->descolagem = false;
	chegadaPISTA->aterragem = false;

	//int n = 1;
	while(1){
		printf("A receber mensagem da thread VOO...\n");

		msgrcv(mqid, &msg, sizeof(msg)-sizeof(long), SLOT, 0); 
		
		nrSlot++;
		msg.slot = nrSlot;

		if(msg.takeoff == 0){ // CHEGADA
			printf("-----> RECEBIDO flight_code: %s \n",msg.flight_code);
			printf("-----> RECEBIDO ETA: %d \n",msg.eta);
			printf("-----> RECEBIDO tempo CHEGADA: %d \n",msg.tchegada);
			printf("-----> RECEBIDO Fuel: %d \n", msg.fuel);
			printf("-----> RECEBIDO emergencia: %d \n", msg.emergencia);
			printf("-----> RECEBIDO holding: %d \n", msg.holding);
			printf("-----> mtype : %ld \n", msg.mtype);

			sem_wait(sem_voo);
			strcpy(chegadaPISTA->flight_code, msg.flight_code);
			chegadaPISTA->eta = msg.eta;
			chegadaPISTA->tchegada = msg.tchegada;
			chegadaPISTA->fuel = msg.fuel;
			chegadaPISTA->nr_slot = msg.slot; //nrSlot
			chegadaPISTA->emergencia = msg.emergencia;
			sem_post(sem_voo);


			if(msg.fuel <= 4 + msg.eta + config->duracaoAterragem){
				msg.emergencia = true;

				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s EMERGENCY LANDING REQUEST",msg.flight_code);
				writeLOG(log);
				printf("%s EMERGENCY LANDING REQUEST\n", msg.flight_code);
				sem_post(semLOG);
			}else{
				msg.emergencia = false;
			}

			if(tamFilaChegadasPISTA > 5){
				msg.holding = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s HOLDING %d",msg.flight_code,msg.tchegada);
				writeLOG(log);
				printf("%s HOLDING %d\n", msg.flight_code, msg.tchegada);
				sem_post(semLOG);

				//printf("HOLDING !!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}else{
				msg.holding = false;
			}

			/*
			sem_wait(sem_voo);
			inserir_filaChegadaPISTA(chegadaPISTA,msg.tchegada);
			printQueueChegadaPISTA();
			sem_post(sem_voo);
			*/

			/*
			if(msg.emergencia == true){
				printf("emergencia!!!!!\n");
				// funcao insere novo cabecalho!!!
				insere_emergencia(chegadaPISTA);
				printQueueChegadaPISTA();
			}*/

		}else{ //PARTIDA
			printf("-----> RECEBIDO flight_code: %s \n",msg.flight_code);
			printf("-----> RECEBIDO Takeoff: %d \n",msg.takeoff);
			printf("-----> mtype : %ld \n", msg.mtype);

			//partidaPISTA = (slotVooP*) malloc(sizeof(slotVooP));
			sem_wait(sem_voo);
			strcpy(partidaPISTA->flight_code, msg.flight_code);
			partidaPISTA->nr_slot = msg.slot; 
			partidaPISTA->takeoff = msg.takeoff;
			sem_post(sem_voo);

			//inserir_filaPartidaPISTA(partidaPISTA,msg.takeoff);
			//printQueuePartidaPISTA();

			//printf("PISTA: NOVO VOO PARTIDA ... %s %d\n", partidaPISTA->flight_code, partidaPISTA->takeoff);
			
			//printf("-----> codigo: %s  \n", partidaPISTA->flight_code);
			//printf("-----> NO codigo: %s \n", noPartida_inicioPISTA->partidaPISTA->flight_code);

			/*
		    pthread_mutex_lock(&pmutex); 
		    while(partidaPISTA->ready == false){
		    	printf(".... ready false\n");
		    	pthread_cond_wait(&pcond, &pmutex); 
		    }
		    pthread_mutex_unlock(&pmutex); 
		    */
		    
			
			/*
			pthread_mutex_lock(&pmutex);
			partidaPISTA->descolagem = true; // descola
			pthread_cond_broadcast(&pcond);
			pthread_mutex_unlock(&pmutex);
			*/
			
		}
		printf("-----------------------------------------------\n");
		printf("-----> vai enviar msg SLOT : %ld \n", msg.slot);
		msg.mtype = VOO;

		// enviar mensagem à thread voo com o nr de slot atribuido
		msgsnd(mqid,&msg,sizeof(msg)-sizeof(long),0);


		// ========================================================================
		// =========================== GERIR OPERACOES DE VOO =====================
		// ========================================================================

		/*
		pista->p1 = false;
		pista->p2 = false;
		pista->c1 = false;
		pista->c2 = false;
 		*/
		msgrcv(mqid,&msg,sizeof(msg)-sizeof(long),GO,0);

		if(msg.readyC == true){ //chegou ao takeoff
			printf("pronto para aterrar...\n");
			if(P1 == false && P2 == false && C1 == false && C2 == false){
				chegadaPISTA->aterragem = true; // descola
				chegadaPISTA->pista = 1;
				C1 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s ARRIVAL concluded Pista C1",msg.flight_code);
				writeLOG(log);
				printf("%s ARRIVAL concluded Pista C1\n", msg.flight_code);
				sem_post(semLOG);
			}else if(P1 == false && P2 == false && C1 == true && C2 == false){
				chegadaPISTA->aterragem = true; // descola
				chegadaPISTA->pista = 2;
				C2 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s ARRIVAL concluded Pista C2",msg.flight_code);
				writeLOG(log);
				printf("%s ARRIVAL concluded Pista C2\n", msg.flight_code);
				sem_post(semLOG);
			}else if(P1 == false && P2 == false && C1 == false && C2 == true){
				chegadaPISTA->aterragem = true; // descola
				chegadaPISTA->pista = 1;
				C1 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s ARRIVAL concluded Pista C1",msg.flight_code);
				writeLOG(log);
				printf("%s ARRIVAL concluded Pista C1\n", msg.flight_code);
				sem_post(semLOG);
			}else{
				printf("Voo ADIADO\n");
				chegadaPISTA->tchegada += 5;
			}
			//remover_filaChegadaPISTA();
			//printQueueChegadaPISTA();
			
		}
		if(msg.readyP == true){ //chegou ao init + eta
			printf("pronto para descolar...\n");

			if(P1 == false && P2 == false && C1 == false && C2 == false){
				partidaPISTA->descolagem = true; // descola
				partidaPISTA->pista= 1;
				P1 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s DEPARTURE concluded Pista P1",msg.flight_code);
				writeLOG(log);
				printf("%s DEPARTURE concluded Pista P1\n", msg.flight_code);
				sem_post(semLOG);
			}else if(P1 == true && P2 == false && (C1 == false && C2 == false)){
				partidaPISTA->descolagem = true; // descola
				partidaPISTA->pista = 2;
				P2 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s DEPARTURE concluded Pista P2",msg.flight_code);
				writeLOG(log);
				printf("%s DEPARTURE concluded Pista P2\n", msg.flight_code);
				sem_post(semLOG);
			}else if(P1 == false && P2 == true && (C1 == false && C2 == false)){
				partidaPISTA->descolagem = true; // descola
				partidaPISTA->pista = 1;
				P1 = true;
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s DEPARTURE concluded Pista P1",msg.flight_code);
				writeLOG(log);
				printf("%s DEPARTURE concluded Pista P1\n", msg.flight_code);
				sem_post(semLOG);
			}else{ // pistas ocupadas
				//partidaPISTA->takeoff += 5;
				//msg.takeoff++;
				adiaPartida++;
			}
			//remover_filaPartidaPISTA();
			//printQueuePartidaPISTA();
		}

		/*
		if(msg.readyC == true && msg.readyP == true){
			continue;
		}

		else if(msg.readyC == true && msg.readyP == false){
			printf("pronto para aterrar...\n");
			continue;
			
			pista->c1 = true;
			if(pista->c1) = true{
				chegadaPISTA->aterragem = true; // descola
				char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s ARRIVAL CONCLUDED P1",chegadaPISTA->flight_code);
				writeLOG(log);
				printf("%s ARRIVAL CONCLUDED P1\n", chegadaPISTA->flight_code);
				sem_post(semLOG);
			}*/
			/*
		}else if(msg.readyC == false && msg.readyP == true){
			printf("pronto para descolar...\n");

			if(pista->p1 == false && pista->p2 == false){ //livres
				
				//pista 1
				pista->p1 = true; //ocupada
				partidaPISTA->descolagem = true; // descola
			    char log[BUFFER_SIZE];
				sem_wait(semLOG);
				sprintf(log,"%s DEPARTURE CONCLUDED P1",noPartida_inicioPISTA->partidaPISTA->flight_code);
				writeLOG(log);
				printf("%s DEPARTURE CONCLUDED P1\n", noPartida_inicioPISTA->partidaPISTA->flight_code);
				sem_post(semLOG);
				remover_filaPartidaPISTA();

				// pista 2
				pista->p2 = true;
				partidaPISTA->descolagem = true; // descola
				sem_wait(semLOG);
				sprintf(log,"%s DEPARTURE CONCLUDED P1",noPartida_inicioPISTA->partidaPISTA->flight_code);
				writeLOG(log);
				printf("%s DEPARTURE CONCLUDED P1\n", noPartida_inicioPISTA->partidaPISTA->flight_code);
				sem_post(semLOG);
				remover_filaPartidaPISTA();
			}
		}
		*/
		/*
		if(msg.readyC == true){ //chegou ao takeoff
			printf("pronto para aterrar...\n");

			chegadaPISTA->aterragem = true; // descola

			char log[BUFFER_SIZE];
			sem_wait(semLOG);
			sprintf(log,"%s ARRIVAL conclued",msg.flight_code);
			writeLOG(log);
			printf("%s ARRIVAL conclued\n", msg.flight_code);
			sem_post(semLOG);
			//remover_filaChegadaPISTA();
			//printQueueChegadaPISTA();
		}
		
		*/
		/*
		if(msg.readyP == true){ //chegou ao takeoff	
			printf("pronto para descolar...\n");
			//printf("codigo: %s",noPartida_inicioPISTA->partidaPISTA->flight_code);
			partidaPISTA->descolagem = true; // descola
			//printf("codigo: %s",partidaPISTA->flight_code);
			//TP89 DEPARTURE 1R concluded  ---> "FLIGHT CODE" DEPARTURE "NR PISTA" CONCLUDED
			
		    char log[BUFFER_SIZE];
			sem_wait(semLOG);

			//printf("-----> codigo: %s \n", partidaPISTA->flight_code);
			//printf("-----> NO codigo: %s \n", noPartida_inicioPISTA->partidaPISTA->flight_code);
			//remover_filaPartidaPISTA();
			//printf("-----> codigo: %s \n", partidaPISTA->flight_code);
			//printf("-----> NO codigo: %s \n", noPartida_inicioPISTA->partidaPISTA->flight_code);
			
			sprintf(log,"%s DEPARTURE CONCLUDED",msg.flight_code);
			writeLOG(log);
			printf("%s DEPARTURE CONCLUDED\n", msg.flight_code);
			sem_post(semLOG);
			
			//printQueuePartidaPISTA();
		}
		*/
		
		msg.mtype = READY;
		// enviar mensagem à thread voo com o nr de slot atribuido
		msgsnd(mqid,&msg,sizeof(msg)-sizeof(long),0);
	}	
}

int main(){ 
	// Redirecional alguns sinais graves para uma rotina de cleanup
	signal(SIGINT, endprogram);
	//signal(SIGHUP, endprogram);
	//signal(SIGQUIT, endprogram);
	//signal(SIGTERM, endprogram);
	signal(SIGUSR1, printStats);
	// kill -s SIGURSR1 

	init();
	
	sem_wait(semLOG);
	char msg[BUFFER_SIZE] = "Inicio do programa";
	writeLOG(msg);
	sem_post(semLOG);

	pid_t id;
	id = fork();
	if (id == 0){
		//signal(SIGUSR1, printStats); //1165
		//kill(getpid(), SIGUSR1); // FUNCIONA !!!  // kill -USR1 $pid (sem o $) -> $pid TC noutro terminal
		torreControlo(); // cria processo filho -> processo Torre de controlo
		exit(0);
	}else if(id == -1){
		perror("Erro fork()");
		exit(0);
	}
	else{	
		//kill(getppid(), SIGUSR1); // kill -> para enviar sinal para stats
	  	simulatorManager();   //processo pai -> inicia o sistema (simulator manager)
	  	pthread_join(thread_tempo, NULL); //espera que a thread tempo morra
	  	wait(NULL); //espera q o processo filho morra primeiro
		exit(0);
	}
	return 0;
}

