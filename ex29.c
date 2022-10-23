/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include "defines.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define STRSIZE 100

FILE* fp;
char* fname = ".\\sensores.txt";

/* Handle para as Filas de Mensagens */
xQueueHandle xQueueSensores;
xQueueHandle xFilaSensorA;
xQueueHandle xFilaSensorB;
xQueueHandle xFilaSensorC;

/* Define o tipo de dado a ser utilizado */
typedef struct
{
	char* ucSource; // Campo que contem uma string ("A", "B" ou "C") que identifica o sensor
	float fValue;   // Campo que contem um float que representa o valor lido pelo sensor. No arquivo os números estão utilizando "," e não ".". No main.c a linha setlocale(LC_ALL, "Portuguese"); deve estar descomentada
} xSensorData;


void sensores_ISR(void* pvParameters);
void identificada_Sensor(void* pvParameters);
void Media_SensorA(void* pvParameters);
void Media_SensorB(void* pvParameters);
void Media_SensorC(void* pvParameters);

xSemaphoreHandle xBinarySemaphore;


void main_prova1(void)
{

	fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("Arquivo %s nao localizado\n", fname);
	}

	/* Cria a fila com tamanho diferentes e variaveis do tipo xSensorData */
	xQueueSensores = xQueueCreate(5, sizeof(xSensorData));
	xFilaSensorA = xQueueCreate(200, sizeof(xSensorData));
	xFilaSensorB = xQueueCreate(100, sizeof(xSensorData));
	xFilaSensorC = xQueueCreate(10, sizeof(xSensorData));

	vSemaphoreCreateBinary(xBinarySemaphore); //Cria semaforo binario 
	xSemaphoreTake(xBinarySemaphore, 0);     // Inicializa Indisponivel


	printf("Semaforo Criado\r\n");

	xTaskCreate(sensores_ISR, "sensor_ISR", 1000, NULL, 3, NULL);
	printf("Criada sensor_ISR\r\n");
	xTaskCreate(identificada_Sensor, "identificada_Sensor", 1000, NULL, 3, NULL);
	printf("Criada Task identificada_Sensor\r\n");
	xTaskCreate(Media_SensorA, "Media_SensorA", 1000, NULL, 6, NULL);
	printf("Criada Task Media_SensorA\r\n");
	xTaskCreate(Media_SensorB, "Media_SensorB", 1000, NULL, 5, NULL);
	printf("Criada Task Media_SensorB\r\n");
	xTaskCreate(Media_SensorC, "Media_SensorC", 1000, NULL, 4, NULL);
	printf("Criada Task Media_SensorC\r\n\n");


	printf("Escalonador Iniciado!\r\n\n");
	vTaskStartScheduler();

}

void sensores_ISR(void* pvParameters)
{
	portTickType xLastWakeTime;
	portBASE_TYPE xstatus;
	char str[STRSIZE];
	char* pt;
	char aux = 0;

	xSensorData Data;

	xLastWakeTime = xTaskGetTickCount();


	for (;;)
	{
		xSemaphoreTake(xBinarySemaphore, 0); // Responsavel por travar as task de maior prioridade de Media_Sensor A , B ou C depois delas terem executado

		if (fgets(str, STRSIZE, fp) == NULL)
		{
			printf("Final do Arquivo!\r\n\n");
			fclose(fp);
			vTaskDelete(NULL);
		}

		pt = strtok(str, ";");
		Data.ucSource = pt;
		aux = 1;
		while (pt) {
			pt = strtok(NULL, ";");
			if (aux == 1)
			{
				Data.fValue = atof(pt);
				aux = 0;

				// Realiza o envio das leituras individuais dos sensores
				xstatus = xQueueSendToBack(xQueueSensores, &Data, 0);
				printf("Dados Recebido do Sensor %s = %f\r\n", Data.ucSource, Data.fValue);
			}

			else
			{
				Data.ucSource = pt;
				aux = 1;

			}

			//vTaskDelayUntil(&xLastWakeTime, 100/ portTICK_RATE_MS);
			vTaskDelay(16 / portTICK_RATE_MS);
		}
	}
}


// Essa Task e responsavel por separar quais leituras vindas da ISR são de cada sensor. Apos identificada
// as medidas serão armazenadas em em suas respectivas filas xFilaSensorA, xFilaSensorB ou xFilaSensorC.
void identificada_Sensor(void* pvParameters)
{

	portTickType xLastWakeTime;
	xSensorData ReceivedData;
	portBASE_TYPE xStatus;
	float Valor;
	char str;
	xLastWakeTime = xTaskGetTickCount();
	unsigned int ui;
	unsigned long ul;

	while (1) {

		int contemp = 0;

		for (;;)
		{
			xSemaphoreTake(xBinarySemaphore, 0); // Responsavel por travar as task de maior prioridade de Media_Sensor A , B ou C depois delas terem executado
			xStatus = xQueueReceive(xQueueSensores, &ReceivedData, 0); // Leitura da Fila criada pelo IRS

			str = *ReceivedData.ucSource;


			if (xStatus == pdPASS)
			{

				switch (str) // Após identificado quais são os sensores é enviado para as filas correspondentes
				{
				case 'A':
					xStatus = xQueueSendToBack(xFilaSensorA, &ReceivedData.fValue, portMAX_DELAY); break;

				case 'B':
					xStatus = xQueueSendToBack(xFilaSensorB, &ReceivedData.fValue, portMAX_DELAY); break;

				case 'C':
					xStatus = xQueueSendToBack(xFilaSensorC, &ReceivedData.fValue, portMAX_DELAY); break;

				}

				for (ui = 0; ui < 1; ui++)
				{
					for (ul = 0; ul < 10000001; ul++)
					{
						///// Usado para ajustar o tempo de amostragem
					}
				}
				contemp++; 
			}

			if (contemp == 300) // A cada + ou - 10s teremos 300 amostras ao total.
			{
				contemp = 0;
				xSemaphoreGive(xBinarySemaphore); // apos 10seg é destravado a chave para as task de Media e com maior prioridade possam executar.

			}

			//vTaskDelayUntil(&xLastWakeTime, 100 / portTICK_RATE_MS); // Não consegui ajustar o tempo para un numero menos que 100, o computador travava, por isso utilizei vTaskDelay(16 / portTICK_RATE_MS);
			vTaskDelay(16 / portTICK_RATE_MS);

		}

	}
}


void Media_SensorA(void* pvParameters) // Apos liberado ler a Fila A e apresenta a média do sensor A
{

	for (;;) {


		xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);

		portBASE_TYPE xStatus = 1;
		float Valor = 0, soma = 0, media = 0;
		int cont = 0;



		while (xStatus != NULL)
		{
			xStatus = xQueueReceive(xFilaSensorA, &Valor, 0);
			soma = soma + Valor;
			if (xStatus != NULL) cont++;
		}

		media = soma / cont;
		printf_colored("================================\r\n", COLOR_RED);
		printf(" A media do sensor A e = %.10f \r\n", media);

		xSemaphoreGive(xBinarySemaphore);// Apos liberado a Task Media_SensorB pegara o semaforo sendo ela a proxima de prioridade maior em Ready
		vTaskDelay(10); // envia task para blocked permitindo que Task B execute

	}
}

void Media_SensorB(void* pvParameters) // Apos liberado ler a Fila B e apresenta a média do sensor B 
{
	for (;;) {


		xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
		portBASE_TYPE xStatus = 1;
		float Valor = 0, soma = 0, media = 0;
		int cont = 0;



		while (xStatus != NULL)
		{
			xStatus = xQueueReceive(xFilaSensorB, &Valor, 0);
			soma = soma + Valor;
			if (xStatus != NULL) cont++;
		}
		media = soma / cont;
		printf(" A media do sensor B e = %.10f \r\n", media);






		xSemaphoreGive(xBinarySemaphore); // Apos liberado a Task Media_SensorC pegara o semaforo sendo ela a proxima de prioridade maior em Ready
		vTaskDelay(10); // envia task para blocked permitindo que Task C execute
		taskYIELD();

	}
}

void Media_SensorC(void* pvParameters) // Apos liberado ler a Fila c e apresenta a média do sensor c
{
	unsigned portBASE_TYPE uxPriority;
	for (;;) {


		xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);

		portBASE_TYPE xStatus = 1;
		float Valor = 0, soma = 0, media = 0;
		int cont = 0;
		unsigned int ui;
		unsigned long ul;


		while (xStatus != NULL)
		{
			xStatus = xQueueReceive(xFilaSensorC, &Valor, 0);
			soma = soma + Valor;
			if (xStatus != NULL) cont++;
		}
		media = soma / cont;
		printf(" A media do sensor C e = %.10f \r\n", media);
		printf_colored("================================\r\n\n", COLOR_RED);

		for (ui = 0; ui < 3; ui++)
		{
			for (ul = 0; ul < 100000000; ul++)
			{
				//Apos apresentar as 3 média o programa da uma pausa para que seja possivel visualizar as médias
				//Ajuste do tempo dessa pausa 
			}
		}
		xSemaphoreGive(xBinarySemaphore); // Apos liberado a Task ISR ou  Identificadora  pegara o semafo sendo elas as proximas de prioridade maior em Ready
		vTaskDelay(10);  // envia task para blocked permitindo que Task ISR ou identificadora execute

	}

}