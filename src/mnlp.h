/*
 * mmnlp.h
 *
 *  Created on: Jan 8, 2016
 *      Author: itay
 */

#include <hal/boolean.h>
#include "trxvu.h"
#ifndef MNLP_H_
#define MNLP_H_

#define sqnce_length  256
#define TimeTableStart 10
#define sqnc_eot 0xFE
#define OBC_EOT 0xfe
#define on_cmd 0xF1
#define off_cmd 0xF2
#define GPIO_04	PIN_GPIO04
#define GPIO_05	PIN_GPIO05
#define GPIO_06	PIN_GPIO06
#define GPIO_07 PIN_GPIO07
#define MNLP_SCRIPT_ADDRESS 0x10000
#define MNLP_SCRIPT_MAX_SIZE 2024

#define HEADER_LENGTH 10
/*
 * first element in the arrays saves the array's length
 * times_table - arry of the sequences to activate and when in a 24 hours format
 * sq[sqnc num][sqnc data]-  the sequnces of the script in two Two-Dimensional array,
 * int len - script length
 * int start_time - UTC format time to activate the script
 * char header[10] - the scripts's header
 * */
typedef struct sequence_com{
	int deltaTime;
	char CMD_ID;
	char LEN;
	char SEQ_CNT;
	unsigned char to_send[256];
}sequence_com;
typedef struct script_parse
{
	int t_t_len; // LEngth THingY
	int t_t[2][100]; // time_table
	int unix_start_time; // UTC
	struct sequence_com mnlp_com[5][sqnce_length];
}script_parse;


int set_time_table(unsigned char script[],int tt[][100],int *sos);
void set_definition_sequence(unsigned char script[],struct sequence_com mnlp_com[5][256],int sos,int len);
void get_new_script(struct script_parse *helper);



void taskmnlp();//task for the mnlp operation

void _sqncTask( void* pvParameters );//task for the sqnc

void _mnlplistener( void* pvParameters);// readi9ng from the mnlp

int Fletcher16( int* data, int count );//check sum for the uploaded script


/*
 * Receives a script struct , a counter and a sequence and sends a command to the mNLP
 * return 0 if everything is OK.
 * return 1 if received as a response from the mNLP an error packet. */
int send_mnlp_cmd(sequence_com cmd);//send command to the mnlp



Boolean obc_su_on();//turn on the

Boolean obc_su_off();//turn on the

void error_protocol();

int chack_new_script();//return 1 if ther's a new script to start

void update_script();//updates the running script

void check_timesTable();//check is ther's a new sequence to run

#endif /* MNLP_H_ */



