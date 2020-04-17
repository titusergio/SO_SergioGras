#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include <stdbool.h>
#include <pthread.h>


typedef struct {
	char nombre[20];
	int socket;
}Conectado;

typedef struct {
	Conectado conectados[100];
	int num;
}ListaConectados;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //Acceso excluyente

int añadir(ListaConectados *lista, char nombre[20], int socket){
	//A�ade nuevo conectado.
	if(lista->num == 100){
		return -1;
	}else{
		strcpy(lista->conectados[lista->num].nombre, nombre);
		lista->conectados[lista->num].socket = socket;
		lista->num = lista->num + 1;
		return 0;
	}
}
	
int Eliminar(ListaConectados *lista, char nombre[20]){
		
	int pos = DamePosicion(lista, nombre);
	
	if(pos == -1){
		return -1;
	}else{
		int i;
		for(i=pos; i<lista->num-1; i++){
			
			strcpy(lista->conectados[i].nombre, lista->conectados[i+1].nombre);
			lista->conectados[i].socket = lista->conectados[i+1].socket;
			
		}
		lista->num = lista->num-1;
		return 0;
	}
}
		
int DamePosicion(ListaConectados *lista, char nombre[20]){
	//Devuelve Posicion
	int i = 0;
	int encontrado = 0;
	
	while(i<lista->num && encontrado==0){
		if(strcmp(nombre, lista->conectados[i].nombre)==0){
			encontrado = 1;
		}if(encontrado==0){
			i++;
		}
	}
	if(encontrado == 1){
		return i;
	}else{
		return -1;
	}
}
			
			
void DameConectados(ListaConectados *lista, char conectados[300]){
	//Pone en conectados los nombres de todos los conectados separados por /
	//primero pone el num. de conectados. Ejemplo: 3/Maria/Juan
	int i;
	sprintf(conectados, "%s", lista->conectados[0].nombre);
	printf("NUMERO DE CONECTATS: %d\n", lista->num);
	if(lista->num > 0){
		for(i=1; i<lista->num; i++){
			sprintf(conectados, "%s/%s", conectados, lista->conectados[i].nombre);
		}
	}
	printf("CONECTADOS: %s\n", conectados);
	
}


	
void *AtenderCliente(ListaConectados *lista){
	
	char conectados[300];
	int sock_conn;
	int *s;
	int pos = lista->num;
	printf("POSICIO: %d\n", pos);
	printf("LISTA: %d\n", lista->conectados[pos].socket);
	s=&lista->conectados[pos].socket;
	printf("S: %d\n", *s);
	sock_conn = *s;
	printf("1SOCK_CONN:%d\n", sock_conn);
	printf("LIST: %d\n", lista->conectados[pos].socket);
	int ret, err;
	char buff[512],buff2[512], nombre[20],contrasena[20], email[20], idPartida[20];;
	MYSQL *conn;
	MYSQL_RES *resultado; // Estructura especial para almacenar resultados de consultas 
	MYSQL_ROW row;
	
	
	int terminar=0;
	int codigo;
	
	//Creamos una conexion al servidor MYSQL 
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexi￳n: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//inicializar la conexin
	conn = mysql_real_connect (conn, "localhost","root", "mysql", "juego",0, NULL, 0);
	if (conn==NULL) {
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	while (terminar==0){
		ret=read(sock_conn, buff, sizeof(buff)); //recibimos mensaje
		printf ("Recibido\n");  // Tenemos que a?adirle la marca de fin de string, para que no escriba lo que hay despues en el buffer
		buff[ret]='\0';
		printf("MENSAJE: %s\n", buff);
		
		
		char *p;//porque el asterisco?????? Es un puntero, quan utilitzes la funcio strtok, fragmentes l'array pel caracter que
		//t'interessa, aixo ho fas a trav�s del punter. El punter va fins a la direcci� de memoria on hi ha el caracter que t'interessa,
		//el substitueix per un caracter fi de linea: '\0' i el punter queda en aquella posici� per seguir fragmentant l'array
		char consulta[80];
		p = strtok( buff, "/");
		codigo = atoi(p);
		printf("CODIGO: %d\n", codigo);
		if(codigo==1 || codigo==2){
			p = strtok( NULL, "/");
			strcpy (nombre, p);
		}
		
		
		switch (codigo)
		{
			case 0:{
				printf("DESCONEXION\n");
				close(sock_conn);
				terminar = 1;
				Eliminar(lista, nombre);
				printf("Conectados: %d\n", lista->num);
				break;
			}
			case 1:
			{
				strcpy(consulta,"INSERT INTO jugador VALUES('");
			
				/*p = strtok( NULL, "/");
				strcpy (nombre, p);*/
				strcpy(lista->conectados[pos].nombre, p);
				strcat(consulta,nombre);
				strcat(consulta,"',");
			
				p = strtok( NULL, "/");
				strcpy (contrasena, p);
				strcat(consulta,contrasena);
				strcat(consulta,",'");
			
				p = strtok( NULL, "/");
				strcpy (email, p);
			
				strcat(consulta,email);
				strcat(consulta,"');");
			
			
				err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al insertar datos en la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}else
				{
					strcpy (buff2,"El servidor realizo correctamente el registro");
					write (sock_conn,buff2, strlen(buff2));
					// Se acabo el servicio para este cliente
					//close(sock_conn); 
				
				
				}
			 
				break;
			}
		
			case 2:
			{
				pthread_mutex_lock (&mutex);//Pedimos que no interrumpan
				strcpy(consulta,"SELECT contrasena FROM jugador WHERE usuario = '");
				/*p = strtok( NULL, "/");
				strcpy (nombre, p);*/
				//strcpy(lista->conectados[pos].nombre, nombre);
				strcat(consulta,nombre);
				strcat(consulta,"';");
				p = strtok( NULL, "/");
				strcpy (contrasena, p);
				err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al insertar datos en la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}	
				//recogemos el resultado de la consulta. El resultado de la
				//consulta se devuelve en una variable del tipo puntero a
				//MYSQL_RES tal y como hemos declarado anteriormente.
				//Se trata de una tabla virtual en memoria que es la copia
				//de la tabla real en disco.
				resultado = mysql_store_result (conn);							// El resultado es una estructura matricial en memoria,en la que cada fila contiene los datos de una persona.
				
			
				// Ahora obtenemos la primera fila que se almacena en una
				// variable de tipo MYSQL_ROW
				row = mysql_fetch_row (resultado);
				// En una fila hay tantas columnas como datos tiene una
				// persona. En nuestro caso hay tres columnas: dni(row[0]),
				// nombre(row[1]) y edad (row[2]).
				printf("Resultat: %s\n", row[0]);
			
				if(strcmp(row[0], contrasena)==0){
					Pon(lista, nombre, sock_conn);
					/////////////////////////////////////////////////////////AQUEST SOCKET JA L'HEM POSAT A LA LLISTA A BAIX DE TOOOT!!
				//	DameConectados(&lista, lista->conectados);
					strcpy (buff2,"Acceso");
					write (sock_conn,buff2, strlen(buff2));
					// Se acabo el servicio para este cliente
					//close(sock_conn); 
				}
				pthread_mutex_unlock (&mutex);//Ya pueden interrumpir
			
				break;
			}
			case 3:
			{
				p = strtok( NULL, "/");
				strcpy (idPartida, p);
				strcat(consulta,idPartida);
				strcpy(consulta,"SELECT ganador FROM partida WHERE ID = '");
				strcat(consulta,idPartida);
				strcat(consulta,"';");
				printf("BE\n");
				err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al insertar datos en la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}	
				//recogemos el resultado de la consulta. El resultado de la
				//consulta se devuelve en una variable del tipo puntero a
				//MYSQL_RES tal y como hemos declarado anteriormente.
				//Se trata de una tabla virtual en memoria que es la copia
				//de la tabla real en disco.
				resultado = mysql_store_result (conn);
				// El resultado es una estructura matricial en memoria
				// en la que cada fila contiene los datos de una persona.
			
				// Ahora obtenemos la primera fila que se almacena en una
				// variable de tipo MYSQL_ROW
				row = mysql_fetch_row (resultado);
				// En una fila hay tantas columnas como datos tiene una
				// persona. En nuestro caso hay tres columnas: dni(row[0]),
				// nombre(row[1]) y edad (row[2]).
				printf("Resultado: %s\n", row[0]);
				strcpy (buff2,row[0]);
				write (sock_conn,buff2, strlen(buff2));
				//close(sock_conn); 
			
			
				break;
			
			}
			case 4:
			{
				strcpy(consulta,"SELECT posicion FROM resumen, partida, jugador WHERE partida.ID = ");
				strcat(consulta, idPartida);
				strcat(consulta," AND jugador.usuario = '");
				strcat(consulta, nombre);
				strcat(consulta,"' AND resumen.jugador = jugador.usuario AND resumen.partida = partida.ID;");
				err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al insertar datos en la base %u %s\n",
					mysql_errno(conn), mysql_error(conn));
					exit (1);
				}	
				//recogemos el resultado de la consulta. El resultado de la
				//consulta se devuelve en una variable del tipo puntero a
				//MYSQL_RES tal y como hemos declarado anteriormente.
				//Se trata de una tabla virtual en memoria que es la copia
				//de la tabla real en disco.
				resultado = mysql_store_result (conn);
				// El resultado es una estructura matricial en memoria
				// en la que cada fila contiene los datos de una persona.
				
				// Ahora obtenemos la primera fila que se almacena en una
				// variable de tipo MYSQL_ROW
				row = mysql_fetch_row (resultado);
				// En una fila hay tantas columnas como datos tiene una
				// persona. En nuestro caso hay tres columnas: dni(row[0]),
				// nombre(row[1]) y edad (row[2]).
				printf("Posicion: %s\n", row[0]);
				strcpy (buff2,row[0]);
				write (sock_conn,buff2, strlen(buff2));
				//close(sock_conn); 
			
			
				break;
			
			}
			case 5:
			{
				p = strtok( NULL, "/");
				strcpy (idPartida, p);
				strcat(consulta, idPartida);
				strcpy(consulta,"SELECT duracion FROM partida WHERE partida.ID = ");
				strcat(consulta, idPartida);
				strcat(consulta,";");
				err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al insertar datos en la base %u %s\n",
					mysql_errno(conn), mysql_error(conn));
					exit (1);
				}	
				//recogemos el resultado de la consulta. El resultado de la
				//consulta se devuelve en una variable del tipo puntero a
				//MYSQL_RES tal y como hemos declarado anteriormente.
				//Se trata de una tabla virtual en memoria que es la copia
				//de la tabla real en disco.
				resultado = mysql_store_result (conn);
				// El resultado es una estructura matricial en memoria
				// en la que cada fila contiene los datos de una persona.
			
				// Ahora obtenemos la primera fila que se almacena en una
				// variable de tipo MYSQL_ROW
				row = mysql_fetch_row (resultado);
				// En una fila hay tantas columnas como datos tiene una
				// persona. En nuestro caso hay tres columnas: dni(row[0]),
				// nombre(row[1]) y edad (row[2]).
				printf("Duracion: %s\n", row[0]);
				strcpy (buff2,row[0]);
				write (sock_conn,buff2, strlen(buff2));
			
				break;
			}
		case 6:
			{
				pthread_mutex_lock (&mutex);//Pedimos que no interrumpan
				DameConectados(lista, conectados);
				
				printf("La funcio ha anat be, Llista: %s", conectados);
				
				write (sock_conn,conectados, strlen(conectados));
				//close(sock_conn); 
				printf("CONECTADOS: %d", lista->num);
				
				pthread_mutex_unlock (&mutex);//Ya pueden interrumpir
				
				break;
				
			}
		};
	}
		
	
}

int main(int argc, char *argv[])
{
	
	ListaConectados listaConectados;
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;

	//inicialitzem el socket per poder establir una connexi�

	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //INICIALITZACIONS,Obrim el socket	
		printf("Error creant socket");
	}
	// Fem el bind al port
	
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;	
	// asocia el socket a cualquiera de las IP de la m?quina.
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// escucharemos en el port 9050
	serv_adr.sin_port = htons(9080);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0){
		printf ("Error al bind");
	}
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0){
		printf("Error en el Listen");
	}
	
		//empezamos a escuchar
	int peticion = 0;
	int a = 0;
	listaConectados.num =0;
	//int sockets[100];
	
	pthread_t thread[100];//vector estructura thread
	
	while(peticion == 0){
		
		printf ("Escuchando\n");
		sock_conn = accept(sock_listen, NULL, NULL); //realizamos conexion
		printf ("Conexion realizada\n");
		printf("0SOCK_CONN: %d\n", sock_conn);
		
		listaConectados.conectados[listaConectados.num].socket=sock_conn;
		printf("LISTASOCKET: %d\n", listaConectados.conectados[listaConectados.num].socket);
		//listaConectados.num++;
		
		pthread_create(&thread[a], NULL, AtenderCliente, &listaConectados);
	}
}
		

