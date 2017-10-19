#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#define CS_MCP3208  6       // GPIO 6 / BCM_GPIO 25
#define RELAY		5		// GPIO 5 / BCM_GPIO 24
#define SPI_CHANNEL 0
#define SPI_SPEED   1000000 // 1MHz

int	 read_mcp3208_adc(unsigned char adc_ch);
int  VOC_remover(double ppm);
void DieWithError(char* errorMessage);

int main (int argc, char* argv[]){
	int				adc_ch	=0;
	int				adc_out	=0;
	float 			v_out	=0;
	float 			ppm		=0;
	int 			sock;
	struct 			sockaddr_in server_addr;
	unsigned short 	server_port;
	char* 			server_ip;
	char			send_str[50];
	unsigned int 	send_strlen;

	server_ip = "106.240.251.245";
	server_port = atoi(argv[1]);
		
	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		DieWithError("socket() failed\n");
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons(server_port);

	if(connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr))<0){
		DieWithError("connect() failed\n");
	}

	if(wiringPiSetup() == -1){
		fprintf (stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return 1 ;
	}

	if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1){
		fprintf (stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));
		return 1 ;
	}

	pinMode(CS_MCP3208, OUTPUT);
	pinMode(RELAY, OUTPUT);
	
	
	while(1){
		int 			remover;
		unsigned int 	ppm_int;
		char 			ppm_str[5];
		adc_out = read_mcp3208_adc(adc_ch);
		v_out=adc_out*(5.0/4095.0);
		//printf("adc0 Value = %u | V = %f\n", adc_out, v_out);
		ppm=pow(10,(-1.586+1.078*v_out-0.087*pow(v_out,2)));
		ppm_int = round(ppm*100);
		sprintf(ppm_str, "%04d", ppm_int);
		remover = VOC_remover(ppm);
		//printf("%c %c %c %c %s\n", ppm_str[0], ppm_str[1], ppm_str[2], ppm_str[3],ppm_str);
		sprintf(send_str, "%d,%c%c.%c%c",remover, ppm_str[0], ppm_str[1], ppm_str[2], ppm_str[3]);
		printf("%s\n", send_str);		
		send_strlen = strlen(send_str);
		
		if(send(sock, send_str, send_strlen, 0) != send_strlen){
			DieWithError("send() sent a different number of bytes than expected\n");
		}
		usleep(1000000/2);
	}
	return 0;
}

int read_mcp3208_adc(unsigned char adc_ch){
	unsigned char 	buff[3];
	int 			adc_out=0;

	buff[0] = 0x06 | ((adc_ch & 0x07) >> 7);
	buff[1] = ((adc_ch & 0x07) << 6);
	buff[2] = 0x00;

	digitalWrite(CS_MCP3208, 0);  // Low : CS Active
	wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
	buff[1] = 0x0F & buff[1];
	adc_out = ( buff[1] << 8) | buff[2];
	digitalWrite(CS_MCP3208, 1);  // High : CS Inactive
	return adc_out;
}

int VOC_remover(double ppm){
	printf("%0.2f(", ppm);
	if(ppm >= 20.0){
		digitalWrite(RELAY, 0);
		printf("ON)\n");
		return 1;		
	}
	else{
		digitalWrite(RELAY, 1);
		printf("OFF)\n");
		return 0;
	}
}
