#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

char *strdup(const char *src)
{
    char *tmp = malloc(strlen(src) + 1);
    if(tmp)
        strcpy(tmp, src);
    return tmp;
}

void explode(const char *src, const char *tokens, char ***list, size_t *len)
{
    if(src == NULL || list == NULL || len == NULL)
        return;

    char *str, *copy, **_list = NULL, **tmp;
    *list = NULL;
    *len  = 0;

    copy = strdup(src);
    if(copy == NULL)
        return;

    str = strtok(copy, tokens);
    if(str == NULL)
        goto free_and_exit;

    _list = realloc(NULL, sizeof *_list);
    if(_list == NULL)
        goto free_and_exit;

    _list[*len] = strdup(str);
    if(_list[*len] == NULL)
        goto free_and_exit;
    (*len)++;


    while((str = strtok(NULL, tokens)))
    {
        tmp = realloc(_list, (sizeof *_list) * (*len + 1));
        if(tmp == NULL)
            goto free_and_exit;

        _list = tmp;

        _list[*len] = strdup(str);
        if(_list[*len] == NULL)
            goto free_and_exit;
        (*len)++;
    }


free_and_exit:
    *list = _list;
    free(copy);
}



int main (void)
{
	int uart0_filestream = -1;
	close(uart0_filestream);
	uart0_filestream = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}else{
		printf("Port opened \n");
	}

	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;				//Ignore parity
	options.c_oflag = 0;
	options.c_lflag |= ICANON;				//Use UART in line-mode, otherwise it just reads 8 bytes
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);


	while(1){
	if (uart0_filestream != -1)
	{
		char rx_buffer[256];  				//Complete NMEA string
		char *match;          				//if GPRMC is found in NMEA string, this is non zero.
		char posfix[] = "A";  				//Position valid string to compare against.
		int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);

		FILE * fp;

		if (rx_length < 0)
		{
			//Error
		}
		else if (rx_length == 0)
		{
			//printf(" no data waiting\n");//No data waiting
		}
		else
		{
			//Bytes received, terminate string with null
			rx_buffer[rx_length] = '\0';
			match = strstr (rx_buffer,"GPRMC");
			if(match){
				char **list;
				size_t i, len;
				explode(rx_buffer, ",", &list, &len);

				/*
				0: $GPRMC
				1: 115206.000
				2: A
				3: 5127.9049
				4: N
				5: 00528.6147
				6: E
				7: 0.00
				8: 78.93
				9: 190817
				10: A*52
				*/

				if(!strcmp(list[2],posfix)){
					fp = fopen ("file.txt", "a");
					printf("DTG:%s/%sZ*POS:%s%s %s%s*SPD:%s\n",list[9],list[1],list[4],list[3],list[6],list[5],list[7]);
					fprintf(fp,"%s;%s;%s;%s;%s;%s;%s\r\n",list[9],list[1],list[4],list[3],list[6],list[5],list[7]);
					fclose(fp);
				} else{
					printf("POSITION INVALID\n");
				}

				/* free list */
				for(i = 0; i < len; ++i)
					free(list[i]);
				free(list);

			}
		}
	}
	}
	//----- CLOSE THE UART -----
	close(uart0_filestream);
  return 0;
}
