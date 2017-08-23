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
	int datatowrite = 0;
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
			char date[6], time[10], lat[10], lon[10], spd[8], northsouth[1], eastwest[1], valid[1];
			char alt[5], hdop[5], numsat[2];

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
//				printf("i:%i - %s",datatowrite, rx_buffer);
				match = strstr (rx_buffer,"GPRMC");
				if(match){
					char **list;
					size_t i, len;
					datatowrite++;
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

					strcpy(time, list[1]);
					strcpy(valid, list[2]);
					strcpy(lat, list[3]);
					strcpy(northsouth, list[4]);
					strcpy(lon, list[5]);
					strcpy(eastwest, list[6]);
					strcpy(spd, list[7]);
					strcpy(date, list[9]);

					/* free list */
					for(i = 0; i < len; ++i)
						free(list[i]);
					free(list);

				}
				match = 0;

				match = strstr (rx_buffer,"GPGGA");
				if(match){
					char **list;
					size_t i, len;
					datatowrite++;
					explode(rx_buffer, ",", &list, &len);

					//0      1          2         3 4          5 6 7  8   9   10 11  12
					//$GPGGA,234444.000,5127.8942,N,00528.6027,E,1,06,1.8,48.7,M,0.0,M,,*53

					strcpy(numsat, list[7]);
					strcpy(hdop, list[8]);
					strcpy(alt, list[9]);

					/* free list */
					for(i = 0; i < len; ++i)
						free(list[i]);
					free(list);
				}
				match = 0;

				if(datatowrite>1){
					if(!strcmp(valid,posfix)){
						fp = fopen ("file.txt", "a");
						printf("DTG:%s/%sZ*POS:%s%s %s%s*SPD:%s*ALT:%s*HDOP:%s*Sats:%s\n",date, time, lat, northsouth, lon, eastwest, spd, alt, hdop, numsat);
						fprintf(fp,"%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\r\n",date, time, lat, northsouth, lon, eastwest, spd, alt, hdop, numsat);
						fclose(fp);
					} else{
						printf("POSITION INVALID\n");
					}
				datatowrite=0;
				}
			}
		}
	}
	//----- CLOSE THE UART -----
	close(uart0_filestream);
  return 0;
}
