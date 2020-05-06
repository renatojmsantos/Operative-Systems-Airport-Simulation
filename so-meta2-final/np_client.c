#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <errno.h> 
#include <string.h>

#include "header.h"

/*
Renato Santos 2015237457
Simao Brito
*/

//gcc -Wall pipe.c -o pipe

void clear_pipe(int sig){
  	//close(fd);
  	printf("Input pipe closed.\n\n");
  	exit(0);
}

// NAMED PIPE CLIENT
int main(){
	signal(SIGINT, clear_pipe); // fecha np client
	
	/*
	//abre pipe para escrita
	fd = open(PIPE_NAME, O_WRONLY);
	if(fd < 0) {
		perror("ERRO a escrever no pipe...");
		exit(0);
	}
	else{
		printf("Input pipe opened for reading.\n");
	}
	*/

	/*
		# estrutura do comando para adicionar uma partida
		# DEPARTURE {flight_code} init: {initial time} takeoff: {takeoff time}

		DEPARTURE TP440 init: 0 takeoff: 100
		DEPARTURE TP89 init: 10 takeoff: 100
		DEPARTURE TP73 init: 10 takeoff: 100

		# estrutura do comando para adicionar uma chegada
		# ARRIVAL {flight_code} init: {initial time} eta: {time to runway}
		# 	fuel: {initial fuel}

		ARRIVAL TP437 init: 0 eta: 100 fuel: 1000
		ARRIVAL TP88 init: 10 eta: 200 fuel: 1500
		ARRIVAL TP70 init: 10 eta: 100 fuel: 1500

		# Todos os tempos são dados em unidades de tempo (ut); os tempos de início
		# e de descolagem são contados em ut desde o início da simulação; o initial
		# time é o instante em que deve ser criada a thread que simula o voo.
	*/

	while(1){
		printf("\n>>> DEPARTURE {flight_code} init: {initial time} takeoff: {takeoff time} <<<\n");
		printf(">>> ARRIVAL {flight_code} init: {initial time} eta: {time to runway} fuel: {initial fuel} <<<\n");
		
		printf("\nNovo comando para adicionar uma partida/chegada:\n");
		
		char comando[BUFFER_SIZE];

		fgets(comando, BUFFER_SIZE, stdin);
		//printf("%s\n", comando);

		if ((strlen(comando) > 0) && (comando[strlen (comando) - 1] == '\n')){
    		comando[strlen (comando) - 1] = '\0'; //fim da string

	    	//printf("%s\n", comando);
	    	fflush(stdout);
	        write(fd, comando,sizeof(comando));
	        printf("enviado...\n");
		}
        sleep(1);
	}
	return 0;
}


