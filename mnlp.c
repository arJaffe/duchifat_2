/*
 * mNLP.c
 *
 *  Created on: Jan 8, 2016
 *      Author: itay
 */
#include <mnlp.h>
#include <hal/Timing/RTT.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/UART.h>
#include <string.h>
#include <stdio.h>
#include <at91/boards/ISIS_OBC_G20/board.h>
#include <at91/utility/trace.h>
#include <at91/commons.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/projdefs.h>
#include <at91/peripherals/pio/pio.h>
#include <hal/interruptPriorities.h>
#include <hal/Utility/util.h>


int uartstart,sqnc,active_sqnc,deactive_sqnc,tt_ctr;
float is_on;
script_parse current_script;




//-------------------------------------------------------------------------------------
//----------------mnlp-task------------------------------------------------------------
//-------------------------------------------------------------------------------------


void mnlpTask(void)
{

	//shouldbe somewhere else.......


	xTaskGenericCreate(taskmnlp, (const signed char*)"mnlp Task", 4096, NULL, 2, NULL, NULL, NULL);
	// xTaskGenericCreate(_mnlplistener,(const signed char*)"mnlp listener Task", 4096, NULL, 2, NULL, NULL, NULL);//activate listener for the mnlp


}



/*
 * if(new_script_uload()==1)
 * {
 * 	parse_script(FileRead(...));
 * }
 * if(new_UTC_time_value()==1)
 * {
 * 	kill_MnlpTask();
 * 	xTaskGenericCreate(_TTTTask);
 * }
 * * _timesTableTask(script current_script){
 * int ctr=1;
 * while(ctr < current_script.times_table[0])
 * {
 *		if(new_day()==1) break;
 *		if(current_script.times_table[i]==_timer())
 *		{
 *			switch(current_script.times_table[i+1])
 *			{
 *				case 0x41
 *				{
 *					_sqnceTask(current_script.sq*, 0);
 *					break;
 *				}
 *				case 0x42
 *				{
 *					_sqnceTask(current_script.sq*, 1);
 *					break;
 *				}
 *				case 0x43
 *				{
 *					_sqnceTask(current_script.sq*, 2);
 *					break;
 *				}
 *				case 0x44
 *				{
 *					_sqnceTask(current_script.sq*, 3);
 *					break;
 *				}
 *				case 0x45
 *				{
 *					_sqnceTask(current_script.sq*, 4);
 *					break;
 *				}
 *			}
 *			i+=2;
 *		}
 * }
 * */
void taskmnlp()//task for the mnlp operation
{
	tt_ctr = 0;
	while(1)
	{
		if(chack_new_script()==1)
		{
			update_script();
		}
		check_timesTable();
		vTaskDelay(850);
	}
}

void check_timesTable()
{
	Time timo;
	int day_time,tempo;
	tempo = Time_get(&timo);
	day_time = timo.seconds + timo.minutes*60 + timo.hours*3600;
	if(day_time- current_script.t_t[1][tt_ctr]<20&&day_time - current_script.t_t[1][tt_ctr]>=0)
	{
		tt_ctr++;
		xTaskGenericCreate(_sqncTask, (const signed char*)"sequence Task", 4096, NULL, 2, NULL, NULL, NULL);
		if(tt_ctr == current_script.t_t_len)
		{
			tt_ctr = 0;
		}
		return;
	}

	vTaskDelay(1000);
}
/*in case that the times table isn't in order:
 *
 *
 * while(!(i<current_script.t_t_len)&&(day_time - current_script.t_t[1][i]<20&&day_time - current_script.t_t[1][i]>=0))
	{
		i++;
	}
	if(i < current_script.t_t_len && active_sqnc != current_script.t_t[0][i])
	{
		active_sqnc = current_script.t_t[0][i];
		xTaskGenericCreate(_sqncTask, (const signed char*)"sequence Task", 4096, NULL, 2, NULL, NULL, NULL);

	}
 *
 * */



void _sqncTask( void* pvParameters )//task for the sqnc
{

	int ctr =0 , cmd_return,j=0;
	while(ctr< sqnce_length &&active_sqnc!=-1&&j< 20)
	{
		vTaskDelay((current_script.mnlp_com[active_sqnc][ctr].deltaTime)*1000);
		cmd_return = send_mnlp_cmd(current_script.mnlp_com[active_sqnc][ctr]);
		if(current_script.mnlp_com[active_sqnc][ctr].CMD_ID==sqnc_eot){
			active_sqnc =-1;
			break;
		}
		else if(cmd_return== 1) {
			j++;//
			continue;
		}
		else if(cmd_return== 0) ctr++;
	}
}

int Fletcher16( int* data, int count )
{
	int sum1 = 0;
	int sum2 = 0;
	int index;
	for( index = 0; index < count; ++index ) {
		sum1 = (sum1 + data[index]) % 255;
		sum2 = (sum2 + sum1) % 255;
	}
	return (sum2 << 8) | sum1;
}


int send_mnlp_cmd(sequence_com cmd)//send command to the mnlp
{
	int temp;
	if(cmd.CMD_ID == on_cmd)
		if(obc_su_on()){
			is_on =1;
			return 0;
		}
		else
			return 1;
	else if(cmd.CMD_ID==off_cmd)
		if(obc_su_off()){
			is_on=0;
			return 0;
		}
		else
			return 1;
	else
	{

		temp = UART_write(bus0_uart,cmd.to_send,cmd.LEN+2);
		if(temp!= 0)
			return 1;
		return 0;

	}
	return 0;
}


//-----------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------need to be done!!!!!!-----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------


Boolean obc_su_on(){


	/////////////////////////////////////////////////////////////WTF IS DIS DOIN' HIR
	UARTconfig configBus0 = {.mode = AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK | AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_OVER_16 | AT91C_US_NBSTOP_1_BIT,
			.baudrate = 9600, .timeGuard = 1, .busType = rs232_uart, .rxtimeout = 0xFFFF};
	/////////////////////////////////////////////////////////////BOLET AnD HArIg



	is_on = 1;

	unsigned int retValInt;
	Pin Pin04 = GPIO_04;
	Pin Pin05 = GPIO_05;
	Pin Pin06 = GPIO_06;
	Pin Pin07 = GPIO_07;

	retValInt = UART_start(bus0_uart, configBus0);
	printf("\n\r Start Test Test: \n\r");

	PIO_Configure(&Pin04, PIO_LISTSIZE(&Pin04));
	if(!PIO_Configure(&Pin04, PIO_LISTSIZE(Pin04))) {
		printf(" PinTest: Unable to configure PIOA pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin05, PIO_LISTSIZE(&Pin05));
	if(!PIO_Configure(&Pin05, PIO_LISTSIZE(Pin05))) {
		printf(" PinTest: Unable to configure PIOB pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin06, PIO_LISTSIZE(&Pin06));
	if(!PIO_Configure(&Pin06, PIO_LISTSIZE(Pin06))) {
		printf(" PinTest: Unable to configure PIOC pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin07, PIO_LISTSIZE(&Pin07));
	if(!PIO_Configure(&Pin07, PIO_LISTSIZE(Pin06))) {
		printf(" PinTest: Unable to configure PIOC pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	printf("\n\r PinTest: All pins should now be logic-0 (0V). Please check their states now. \n\r");
	printf(" PinTest: Press 1 then Enter when done. \n\r");
	//while(1);

	PIO_Set(&Pin04);
	vTaskDelay(10);
	PIO_Set(&Pin05);
	vTaskDelay(10);
	PIO_Set(&Pin06);
	vTaskDelay(10);
	PIO_Set(&Pin07);
	vTaskDelay(10);

	return TRUE;
}


Boolean obc_su_off()
{
	unsigned int retValInt;
	is_on = 0;

	Pin Pin04 = GPIO_04;
	Pin Pin05 = GPIO_05;
	Pin Pin06 = GPIO_06;
	Pin Pin07 = GPIO_07;
	retValInt = UART_stop(bus0_uart);

	printf("\n\r Start Test Test: \n\r");

	PIO_Configure(&Pin04, PIO_LISTSIZE(&Pin04));
	if(!PIO_Configure(&Pin04, PIO_LISTSIZE(Pin04))) {
		printf(" PinTest: Unable to configure PIOA pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin05, PIO_LISTSIZE(&Pin05));
	if(!PIO_Configure(&Pin05, PIO_LISTSIZE(Pin05))) {
		printf(" PinTest: Unable to configure PIOB pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin06, PIO_LISTSIZE(&Pin06));
	if(!PIO_Configure(&Pin06, PIO_LISTSIZE(Pin06))) {
		printf(" PinTest: Unable to configure PIOC pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	PIO_Configure(&Pin07, PIO_LISTSIZE(&Pin07));
	if(!PIO_Configure(&Pin07, PIO_LISTSIZE(Pin06))) {
		printf(" PinTest: Unable to configure PIOC pins as output! \n\r");
		while(1);
	}

	vTaskDelay(10);

	printf("\n\r PinTest: All pins should now be logic-0 (0V). Please check their states now. \n\r");
	printf(" PinTest: Press 1 then Enter when done. \n\r");
	//while(1);


	vTaskDelay(5000 / portTICK_RATE_MS);
	PIO_Clear(&Pin04);
	vTaskDelay(10);
	PIO_Clear(&Pin05);
	vTaskDelay(10);
	PIO_Clear(&Pin06);
	vTaskDelay(10);
	PIO_Clear(&Pin07);
	vTaskDelay(10);

	return TRUE;
}





void error_protocol(){}

int chack_new_script()
{
	//go over all scripts saved in memory and read times
	int i;
	//for loop
	FRAM_read(unsigned char *data, unsigned int address, unsigned int size);

	//compare to current time

	return 1;
}

void update_script()
{
	//update parameter

	// activate parser

}

void _mnlplistener( void* pvParameters)
{
	int temp;
	unsigned int j;//
	unsigned char receive[174];
	while (1)
	{
		if(is_on==1)
		{
			temp = UART_read(bus0_uart,receive,174);
			if(temp == 0)
			{
				for(j =0;j<174;j++)
				{
					printf("%x ",receive[j]);
				}
			}
		}

		vTaskDelay(1000);

	}
}
int set_time_table(unsigned char script[],  int tt[][100], int* sos)// acquires time table for sequences
{
	int i;
	i=0;
	while(script[TimeTableStart+i*4+3] != 0x55)
	{
		tt[0][i] = script[TimeTableStart+3+i*4];
		tt[1][i] = script[TimeTableStart+i*4]+script[TimeTableStart+1+i*4]*60+script[TimeTableStart+2+i*4]*3600;
		i++;
	}
	*sos=(TimeTableStart+i*4+4);
	return i;
}
void set_definition_sequence(unsigned char script[],struct sequence_com mnlp_com[5][256],int sos, int len)// defines each sequence
{
	int i = 0;
	int o = 0;
	int p = 0;
	for(;i<5;i++){
		o=0;
		while(script[sos+2] != 0x55){//should it be 0x55?
			mnlp_com[i][o].deltaTime=script[sos]+script[sos+1]*60;
			mnlp_com[i][o].CMD_ID = script[sos+2];
			mnlp_com[i][o].LEN = script[sos+3]+2;
			//mnlp_com[i][o].SEQ_CNT=script[sos+4];4
			sos+=2;
			for(p=0;p<mnlp_com[i][o].LEN;p++){
				mnlp_com[i][o].to_send[p] = script[sos++];
			}
			o++;
		}
		if(sos+9==len) return;
	}
}

void get_new_script(struct script_parse *helper)
{
	unsigned char script[502];
	unsigned char num;
	unsigned short len;
	int sos;
	FRAM_read((unsigned char*)&len, SCRIPT_RAW_ADDR-3,2);
	FRAM_read(&num,(SCRIPT_RAW_ADDR-1),1);
	FRAM_read(script,SCRIPT_RAW_ADDR,len);
	helper->unix_start_time = script[0] + script[1]*256 + script[2]*256*256 + script[3]*256*256*256;
	helper->t_t_len = set_time_table(script, helper->t_t,&sos);
	set_definition_sequence(script,helper->mnlp_com,sos,len);

}
