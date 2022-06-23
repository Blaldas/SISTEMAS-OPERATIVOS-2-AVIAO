#define _CRT_SECURE_NO_WARNINGS	//TOU FARTO DESTA PORCARIA

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//ESTADOS PARA OS AVIOES
#define ESTADO_NAO_ACEITA_NOVOS_AVIOES 0
#define ESTADO_CONECAO_INCIAL 1
#define ESTADO_AEROPORTO_INICIAL_MAL 2
#define ESTADO_ESPERA 3
#define ESTADO_EMBARCAR 4
#define ESTADO_DEFINE_DESTINO_BEM 5
#define ESTADO_DEFINE_DESTINO_MAL 6
#define ESTADO_VOO 7
#define ESTADO_DESLIGAR 8
#define ESTADO_DEFINE_AEROPORTO_INICAL 9
#define ESTADO_INDICA_NOVA_POSICAO 10



#define DLLLocation TEXT("D:\\Desktop\\isec\\3 ano\\2 semestre\\SO2\\trabalho prático\\x64\\SO2_TP_DLL_2021.dll")
#define TAM_BUFFER_CIRCULAR 10		//tamanho buffer circular
#define TAM 200


//programa aviões
#define SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES TEXT("SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES")
//Aviao2Control
#define BUFFER_AVIAO2CONTROL TEXT("EVENTO_AVIAO2CONTROL")
#define MUTEX_AVIAO2CONTROL TEXT("MUTEX_AVIAO2CONTROL")
#define SEMAFORO_ESCRITA_AVIAO2CONTROL TEXT("SEMAFORO_ESCRITA_AVIAO2CONTROL")
#define SEMAFORO_LEITURA_AVIAO2CONTROL TEXT("SEMAFORO_LEITURA_AVIAO2CONTROL")
//Control2Aviao
#define BUFFER_CONTROL2AVIAO TEXT("BUFFER_CONTROL2AVIAO")
#define MUTEX_CONTROL2AVIAO TEXT("MUTEX_CONTROL2AVIAO")
#define SEMAFORO_ESCRITA_CONTROL2AVIAO TEXT("SEMAFORO_ESCRITA_CONTROL2AVIAO")
#define SEMAFORO_LEITURA_CONTROL2AVIAO TEXT("SEMAFORO_LEITURA_CONTROL2AVIAO")

//aceder a variaveis locais
#define MUTEX_ACEDER_LISTA_AEROPORTOS TEXT("MUTEX_ACEDER_LISTA_AEROPORTOS")
#define MUTEX_ACEDER_LISTA_AVIOES TEXT("MUTEX_ACEDER_LISTA_AVIOES")
#define MUTEX_ACEDER_LISTA_PASSAGEIROS TEXT("MUTEX_ACEDER_LISTA_PASSAGEIROS")



typedef struct {
	int debug;
	int id;
	int startX, startY, destX, destY, posX, posY;
	int capacidade, numLugaresOcupados;
	TCHAR destino[TAM];
	int velocidade;
	int estado; //estado -> ESTADO_CONECAO_INCIAL |ESTADO_ESPERA | ESTADO_VOO
}aviao, * pAviao;
typedef struct {
	aviao planeBuffer[TAM_BUFFER_CIRCULAR];
	int proxEscrita;
	int proxLeitura;
	int maxPos;
	int nextID;
} AviaoEControl;


void escreveAviao2Control(aviao* plane);


TCHAR* getAeroportos();
void desligarPrograma();


//inerente ao programa
aviao plane;
int id; // A única coisa que não pode ser "perdida"
HMODULE dllHandle;

//coneção com buffers circulares
HANDLE hLimiteAvioesConectados;		//limita número de aviões que se podem conectar
//aviao2Control
AviaoEControl* aviao2Control;			//aviao -> control
HANDLE hMutexAviao2Control;				//mutex para escrever no Aviao2Control
HANDLE hSemafotoEscritaAviao2Control;	//semaforo para avioes poderem escrever no Aviao2Control
HANDLE hSemafotoLeituraAviao2Control;	//semaforo para o Control poder ler o Aviao2Control
HANDLE hAviao2Control;					//handle para o buffer circular
//Control2Aviao
AviaoEControl* control2Aviao;			//control -> aviao
HANDLE hMutexControl2Aviao;				//mutex para escrever no Control2Aviao
HANDLE hSemafotoEscritaControl2Aviao;	//semaforo para control poder escrever no Control2Aviao
HANDLE hSemafotoLeituraControl2Aviao;	//semaforo para avioes poderem ler no Control2Aviao
HANDLE hControl2Aviao;					//handle para o buffer circular



int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	

	//_tprintf(TEXT("%d\n"), argc);
	if (argc != 4) {
		_tprintf(TEXT("argumentos: <lotação> <velocidade> <nome do aeroporto incial>\n"));
		exit(1);
	}

	//--------------------Conecta à DLL
	dllHandle = LoadLibrary(DLLLocation);
	if (dllHandle == NULL) {
		_tprintf(TEXT("Erro ao abrir a DLL.\n"));
		exit(-1);
	}
	int (*ptrFunc)(int, int, int, int, int*, int*) = NULL;
	ptrFunc = (int (*)(int, int, int,int, int*, int*))GetProcAddress(dllHandle, "move");
	if (ptrFunc == NULL) {
		_tprintf(TEXT("\nErro a abrir a dll\nA sair!\n"));
		exit(1);
	}
	//--------------------

	//-----------------Tenta ligar à memoria partilhada 

	//ve se tem lugar no semáforo
	hLimiteAvioesConectados = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, TRUE, SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES);
	if (hLimiteAvioesConectados == NULL) {
		_tprintf(TEXT("Erro ao abrir o semáforo -> %d.\n"), GetLastError());
		exit(-1);
	}
	//espera pelo semáforo
	WaitForSingleObject(hLimiteAvioesConectados, INFINITE);



	/*		Aviao2Control
	AviaoEControl* aviao2Control;			//aviao -> control
	HANDLE hMutexAviao2Control;				//mutex para escrever no Aviao2Control
	HANDLE hSemafotoEscritaAviao2Control;	//semaforo para avioes poderem escrever no Aviao2Control
	HANDLE hSemafotoLeituraAviao2Control;	//semaforo para o Control poder ler o Aviao2Control
	HANDLE hAviao2Control;					//handle para o buffer circular
	*/
	hMutexAviao2Control = OpenMutex(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, MUTEX_AVIAO2CONTROL);
	hSemafotoEscritaAviao2Control = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SEMAFORO_ESCRITA_AVIAO2CONTROL);
	hSemafotoLeituraAviao2Control = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SEMAFORO_LEITURA_AVIAO2CONTROL);
	
	if (hMutexAviao2Control == NULL || hSemafotoEscritaAviao2Control == NULL || hSemafotoLeituraAviao2Control == NULL ) {
		_tprintf(TEXT("Erro ao abrir o semáforo ou o mutex Aviao2Control, idk-> %d.\n"), GetLastError());
		exit(-1);
	}

	//--------------------Ligação à memoria partilhada publica
	hAviao2Control = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, BUFFER_AVIAO2CONTROL);
	if (hAviao2Control == NULL) {
		_tprintf(TEXT("Erro ao abrir o file mapping criado pelo control e conhecido por todos -> %d.\n"), GetLastError());
		exit(-1);
	}

	aviao2Control = (AviaoEControl*) MapViewOfFile(
		hAviao2Control,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0);
	if (aviao2Control == NULL) {
		_tprintf(TEXT("Erro ao criar o map view do Aviao2Control -> %d.\n"), GetLastError());
		exit(-1);
	}

	/*	Control2Aviao
	AviaoEControl* control2Aviao;			//control -> aviao
	HANDLE hMutexControl2Aviao;				//mutex para escrever no Control2Aviao
	HANDLE hSemafotoEscritaControl2Aviao;	//semaforo para control poder escrever no Control2Aviao
	HANDLE hSemafotoLeituraControl2Aviao;	//semaforo para avioes poderem ler no Control2Aviao
	HANDLE hControl2Aviao;					//handle para o buffer circular	
	*/

	hMutexControl2Aviao = OpenMutex(SYNCHRONIZE, TRUE, MUTEX_CONTROL2AVIAO);
	hSemafotoEscritaControl2Aviao = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, TRUE, SEMAFORO_ESCRITA_CONTROL2AVIAO);
	hSemafotoLeituraControl2Aviao = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, TRUE, SEMAFORO_LEITURA_CONTROL2AVIAO);

	if (hMutexControl2Aviao == NULL || hSemafotoEscritaControl2Aviao == NULL || hSemafotoLeituraControl2Aviao == NULL) {
		_tprintf(TEXT("Erro ao abrir o semáforo ou o mutex Control2Aviao, idk-> %d.\n"), GetLastError());
		exit(-1);
	}


	//--------------------Ligação à memoria partilhada publica
	hControl2Aviao = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, BUFFER_CONTROL2AVIAO);
	if (hControl2Aviao == NULL) {
		_tprintf(TEXT("Erro ao abrir o file mapping criado pelo control e conhecido por todos -> %d.\n"), GetLastError());
		exit(-1);
	}

	control2Aviao = (AviaoEControl*)MapViewOfFile(
		 hControl2Aviao,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0);
	if (control2Aviao == NULL) {
		_tprintf(TEXT("Erro ao criar o map view do Control2Aviao -> %d.\n"), GetLastError());
		exit(-1);
	}

	_tprintf(TEXT("\nLigação com todos os mapas, semáforos e afins feita com sucesso!\n"));

	//coloca os dados iniciais no avião
	//<lotação> <velocidade> <nome do aeroporto incial>
	plane.capacidade = _wtoi(argv[1]);
	plane.velocidade = _wtoi(argv[2]);
	wcscpy(plane.destino, argv[3]);
	plane.numLugaresOcupados = 0;
	plane.estado = ESTADO_CONECAO_INCIAL;
	plane.startX = -1;
	plane.startY = -1;
	plane.destX = -1;
	plane.destY = -1;
	plane.posX = -1;
	plane.posY = -1;
	plane.debug = 0;
	
	//escreve coneção inicial
	WaitForSingleObject(hSemafotoEscritaAviao2Control, INFINITE);
	WaitForSingleObject(hMutexAviao2Control, INFINITE);

	plane.id = aviao2Control->nextID++;	//coloca id xD e aumenta-o<- esta linha inivabiliza que se use a função
	id = plane.id;		//guarda id na variavle global
	memcpy(&aviao2Control->planeBuffer[aviao2Control->proxEscrita], &plane, sizeof(plane));
	aviao2Control->proxEscrita == (aviao2Control->maxPos -1) ? aviao2Control->proxEscrita = 0 : aviao2Control->proxEscrita++;

	ReleaseMutex(hMutexAviao2Control);
	if (ReleaseSemaphore(hSemafotoLeituraAviao2Control, 1, NULL) == 0)	//chama o control para ler
		_tprintf(TEXT("Erro a libertar semáforo hSemafotoLeituraAviao2Control -> %d\n"), GetLastError());
	//--------------------ciclo para troca de dados
	_tprintf(TEXT("\nLigação com todos os mapas, semáforos e afins feita com sucesso!\n"));
	/*

#define ESTADO_NAO_ACEITA_NOVOS_AVIOES 0
#define ESTADO_CONECAO_INCIAL 1
#define ESTADO_AEROPORTO_INICIAL_MAL 2
#define ESTADO_ESPERA 3
#define ESTADO_EMBARCAR 4
#define ESTADO_DEFINE_DESTINO_BEM 5
#define ESTADO_DEFINE_DESTINO_MAL 6
#define ESTADO_VOO 7
#define ESTADO_DESLIGAR 8
#define ESTADO_DEFINE_AEROPORTO_INICAL 9

*
*
Esta thread usa 2 memorias partilhadas
Aviao2Control -> onde os aviões escrevem a sua info
Control2Aviao -> onde o control dá a resposta aos aviões

Os aviões conectam-se as mesmas, sendo que so escrevem na Aviao2Control -> o avião é quem escreve o id
Quando se conectam colocam o estado ESTADO_CONECAO_INCIAL e o nome do aeroporto inicial no 'destino'  <----------------- feito acima
O Control testa se o estado for ESTADO_CONECAO_INCIAL , se o destino esta bem escrito e se aceita aviões
Se não aceitar aviões, escreve no buffer Control2Aviao com estado ESTADO_NAO_ACEITA_NOVOS_AVIOES. Ao ler esse estado o avião sai.
Caso o nome do aerporto estiver mal escreve no Control2Aviao Com o estado ESTADO_AEROPORTO_INICIAL_MAL.
Caso esteja tudo bem devolve ESTADO_ESPERA

Quando o aviao recebe ESTADO_AEROPORTO_INICIAL_MAL pede um novo nome de aeroporto e devolve ESTADO_DEFINE_AEROPORTO_INICAL
Quando o controlador recebe ESTADO_DEFINE_AEROPORTO_INICAL, verifica se existe um aeroporto com esse nome. Se sim, coloca coordenadas inicias e devolve ESTADO_ESPERA.
Se nao existir, devolve ESTADO_AEROPORTO_INICIAL_MAL

Quando o control recebe um aviao com ESTADO_DEFINE_DESTINO_BEM verifica se o destino existe. Se existir devoolve ESTADO_DEFINE_DESTINO_BEM. Se não existir, devolve ESTADO_DEFINE_DESTINO_MAL
Quando um aviao receve ESTADO_DEFINE_DESTINO_MAL obtem um novo destino do utilizador e escreve-o com ESTADO_DEFINE_DESTINO_BEM
Quando um aviao recebe ESTADO_DEFINE_DESTINO_BEM decide ou embaracar pessoas, enviando ESTADO_EMBARCAR ou entao comeczar viagem devolvendo ESTADO_VOO e a posição seguinte de viagem.

Quando o controlador recebe ESTADO_EMBARCAR embarca as pessoas para o avião e devolve com ESTADO_EMBARCAR
Quando o aviao recebe ESTADO_EMBARCAR tem de iniciar viagem, usando ESTADO_VOO e a posição seguinte

Quando o Control recebe ESTADO_VOO verifica se a posição esta livre. Se estiver atualiza os dados na sua lista, se não, não os atualiza e devolve o que tem na sua lista.
Se o Controlo detetar que chegou ao aeroporto de destino desembarca toda a gente e devolve ESTADO_ESPERA
Quando o aviao recebe ESTADO_VOO calcula a proxima posição e devolve ESTADO_VOO

O aviao le o ESTADO_ESPERA e tem de definir para onde ir (obtem destino e envia ESTADO_DEFINE_DESTINO_BEM) OU sair, de onde devolve ESTADO_DESLIGAR e desliga o programa
Quando aviao recebe ESTADO_DESLIGAR, desliga o proghrama por ordem do control
Quando control le ESTADO_DESLIGAR, remove o aviao da lista

*/
	int erroCaulculoDeCoordenadas = 1;
	boolean desligar = FALSE;
	while (!desligar) {


		boolean flagAviaoCorreto = FALSE;
		//obtem dados
		WaitForSingleObject(hSemafotoLeituraControl2Aviao, INFINITE);
		WaitForSingleObject(hMutexControl2Aviao, INFINITE);

		memcpy(&plane, &control2Aviao->planeBuffer[control2Aviao->proxLeitura], sizeof(plane));
		if (plane.id == id) {	//verifica se é o id correto
			control2Aviao->proxLeitura == (control2Aviao->maxPos - 1) ? control2Aviao->proxLeitura = 0 : control2Aviao->proxLeitura++;
			flagAviaoCorreto = TRUE;
		}

		ReleaseMutex(hMutexControl2Aviao);
		if (ReleaseSemaphore(hSemafotoEscritaControl2Aviao, 1, NULL) == 0) {
			_tprintf(TEXT("\nErro a libertar semáforo hSemafotoEscritaControl2Aviao -> %d\n"), GetLastError());
		}
		

		//caso seja o nosso avião
		if (flagAviaoCorreto) {
			
			//Se não aceitar aviões, escreve no buffer Control2Aviao com estado ESTADO_NAO_ACEITA_NOVOS_AVIOES. Ao ler esse estado o avião sai.
			if (plane.estado == ESTADO_NAO_ACEITA_NOVOS_AVIOES) {
				_tprintf(TEXT("\nO controler não esta a aceitar novos aviões!\nA desligar\n\n"));
				break;
			}
			//Quando o aviao recebe ESTADO_AEROPORTO_INICIAL_MAL pede um novo nome de aeroporto e devolve ESTADO_DEFINE_AEROPORTO_INICAL
			else if (plane.estado == ESTADO_AEROPORTO_INICIAL_MAL) {
				_tprintf(TEXT("\nAeroporto inicial não existe. Indique um novo aeroporto inicial:\n"));
				_fgetts(plane.destino, TAM - 1, stdin);
				plane.destino[_tcslen(plane.destino) - 1] = '\0';
				plane.estado = ESTADO_DEFINE_AEROPORTO_INICAL;
			}
			//O aviao le o ESTADO_ESPERA e tem de definir para onde ir (obtem destino e envia ESTADO_DEFINE_DESTINO_BEM) OU sair, de onde devolve ESTADO_DESLIGAR e desliga o programa
			else if (plane.estado == ESTADO_ESPERA) {
				_tprintf(TEXT("\nEncontra-se no aeroporto!\n"));
				_tprintf(TEXT("Coordenadas:\t %d ; %d\n"), plane.posX, plane.posY);

				//UI
				TCHAR opt[TAM];
				while (1) {
					_tprintf(TEXT("destino - definir destino\nsair - desligar programa\n\n"));
					//_tscanf(TEXT("%s"), &opt);
					_fgetts(opt, TAM - 1, stdin);
					opt[_tcslen(opt) - 1] = '\0';

					if (wcscmp(TEXT("sair"), opt) == 0) {
						desligar = TRUE;
						plane.estado = ESTADO_DESLIGAR;
						break;
					}
					else if (wcscmp(TEXT("destino"), opt) == 0) {
						_tprintf(TEXT("\nIndique destino:\n"));
						_fgetts(plane.destino, TAM - 1, stdin);
						plane.destino[_tcslen(plane.destino) - 1] = '\0';
						plane.estado = ESTADO_DEFINE_DESTINO_BEM;
						break;
					}
				}

			}
			//Quando o aviao recebe ESTADO_EMBARCAR tem de iniciar viagem, usando ESTADO_VOO e a posição seguinte
			else if (plane.estado == ESTADO_EMBARCAR) {
				_tprintf(TEXT("\nPessoas embarcadas com sucesso!\nprima enter para iniciar!\n"));
				_gettchar();
				
				plane.estado = ESTADO_VOO;
				//CALCULA POSIÇÃO SEGUINTE:
				int res;
				for (int i = 0; i < plane.velocidade; i++) {
					res = ptrFunc(plane.startX, plane.startY, plane.destX, plane.destY, &plane.posX, &plane.posY);
					if (res == 2) {
						_tprintf(TEXT("\nErro ao calcular o próximo ponto da viagem\n"));
						exit(-1);
					}
				}
			}
			//Quando um aviao recebe ESTADO_DEFINE_DESTINO_BEM decide ou embaracar pessoas, enviando ESTADO_EMBARCAR ou entao comeczar viagem devolvendo ESTADO_VOO e a posição seguinte de viagem.
			else if (plane.estado == ESTADO_DEFINE_DESTINO_BEM) {
				while (1) {
					TCHAR opt[TAM];
					_tprintf(TEXT("\nDestino indicado com sucesso!\niniciar - inicia voo!\nembarcar - embarcar pessoas\nsair - desligar aviao"));
					//_tscanf(TEXT("%s"), &opt);
					_fgetts(opt, TAM - 1, stdin);
					opt[_tcslen(opt) - 1] = '\0';

					if (wcscmp(TEXT("embarcar"), opt) == 0) {
						plane.estado = ESTADO_EMBARCAR;
						break;
					}
					else if (wcscmp(TEXT("iniciar"), opt) == 0)
					{
						plane.estado = ESTADO_VOO;
						//CALCULA POSIÇÃO SEGUINTE:
						int res;

						for (int i = 0; i < plane.velocidade; i++) {
							plane.startX = plane.posX;
							plane.startY = plane.posY;
							res = ptrFunc(plane.startX, plane.startY, plane.destX, plane.destY, &plane.posX, &plane.posY);
							if (res == 2) {
								_tprintf(TEXT("Erro ao calcular o próximo ponto da viagem"));
								exit(-1);
							}
						}
						Sleep(1000);
						break;
					}
					else if (wcscmp(TEXT("sair"), opt) == 0) {
						desligar = TRUE;
						plane.estado = ESTADO_DESLIGAR;
						break;
					}
				}
			}
			//Quando um aviao receve ESTADO_DEFINE_DESTINO_MAL obtem um novo destino do utilizador e escreve-o com ESTADO_DEFINE_DESTINO_BEM
			else if (plane.estado == ESTADO_DEFINE_DESTINO_MAL) {
				_tprintf(TEXT("O destino indicado não foi encontrado!\n"));
				_tprintf(TEXT("Indique destino:\n"));
				//_tscanf(TEXT("%s"), &plane.destino);
				_fgetts(plane.destino, TAM - 1, stdin);
				plane.destino[_tcslen(plane.destino) - 1] = '\0';

				plane.estado = ESTADO_DEFINE_DESTINO_BEM;
			}
			//Quando o aviao recebe ESTADO_VOO calcula a proxima posição e devolve ESTADO_VOO
			else if (plane.estado == ESTADO_VOO) {
			_tprintf(TEXT("O avião encontra-se nas coordenadas { %d ; %d }\n"), plane.posX, plane.posY);
				erroCaulculoDeCoordenadas = 1;
				int res;;
				for (int i = 0; i < plane.velocidade; i++) {
					plane.startX = plane.posX;
					plane.startY = plane.posY;

					res = ptrFunc(plane.startX, plane.startY, plane.destX, plane.destY, &plane.posX, &plane.posY);
					if (res == 2) {
						_tprintf(TEXT("Erro ao calcular o próximo ponto da viagem\n"));
						exit(-1);
					}
				}

				Sleep(1000);	//garante que tem uma atualização por segundo! DSaí este sleep
			}
			//	Quando recebe ESTADO_DESLIGAR, desliga o proghrama por ordem do control
			else if (plane.estado == ESTADO_DESLIGAR) {
				_tprintf(TEXT("Foi dada ordem para desligar pelo controlador!\n"));
				break;
			}
			//quando recebe  ESTADO_INDICA_NOVA_POSICAO, envia uma nova posição
			else if (plane.estado == ESTADO_INDICA_NOVA_POSICAO) {
				_tprintf(TEXT("Erro no calculo de coordenadas. A recalcular coordenadas...\n"));		
				plane.posX += erroCaulculoDeCoordenadas;
				plane.estado = ESTADO_VOO;
				++erroCaulculoDeCoordenadas;
			}
			//erro em troca de estados
			else {
				_tprintf(TEXT("\n\nBug nos estados!\tEstado recebido: %d\n\nA sair!\n"), plane.estado);
				break;
			}		
			escreveAviao2Control(&plane);	
		} 
		else {
			Sleep(1);
		}
	}

	//--------------------fechar handles, mapas, etc


	if (ReleaseSemaphore(hLimiteAvioesConectados, 1, NULL) == 0)		//release do semáforo para poder entrar outro avião
		_tprintf(TEXT("Erro a libertar semáforo hLimiteAvioesConectados -> %d\n"), GetLastError());
	CloseHandle(hLimiteAvioesConectados);
	UnmapViewOfFile(aviao2Control);
	UnmapViewOfFile(control2Aviao);
	CloseHandle(hMutexAviao2Control);
	CloseHandle(hSemafotoEscritaAviao2Control);
	CloseHandle(hSemafotoLeituraAviao2Control);
	CloseHandle(hAviao2Control);
	CloseHandle(hMutexControl2Aviao);
	CloseHandle(hSemafotoEscritaControl2Aviao);
	CloseHandle(hSemafotoLeituraControl2Aviao);
	CloseHandle(hControl2Aviao);
	if (FreeLibrary(dllHandle) == 0) {
		_tprintf(TEXT("Não foi possivel fechar dll\n"));
	}

}

void escreveAviao2Control(aviao* plane) {
	plane->debug++;
	WaitForSingleObject(hSemafotoEscritaAviao2Control, INFINITE);
	WaitForSingleObject(hMutexAviao2Control, INFINITE);

	memcpy(&aviao2Control->planeBuffer[aviao2Control->proxEscrita], plane, sizeof(aviao));

	aviao2Control->proxEscrita == (aviao2Control->maxPos - 1) ? aviao2Control->proxEscrita = 0 : aviao2Control->proxEscrita++;
	ReleaseMutex(hMutexAviao2Control);
	if (ReleaseSemaphore(hSemafotoLeituraAviao2Control, 1, NULL) == 0)
		_tprintf(TEXT("Erro a libertar semaforo hSemafotoLeituraAviao2Control -> %d\n"), GetLastError());
}