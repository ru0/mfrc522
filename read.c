#include "mfrc522.c"
#include <stdio.h>
#include <string.h>


int main(){
	int i,count;
	unsigned char s;
	unsigned char id[10];
	unsigned char key[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	unsigned char uid[5]; //4字节卡序列号，第5字节为校验字节
	unsigned char str[MAX_LEN];
	unsigned char wData[16] = {'h','a','c','k','e','d',' ','b','y',' ','r','u','o'};
	int isTrue = 1;


	//unsigned buff[5];

	if (!bcm2835_init()) return -1;

	init();

	while(isTrue){
		if (findCard(0x52,&s) == MI_OK){
			if ( anticoll(id) == MI_OK){
				memcpy(uid,id,5);
				printf("CARD UID:");
				for(i = 0;i < 5;i++)
					printf("%x",uid[i]);
				printf("\n");
			}
			else {
				printf("FindCard ERR.\n");
			}

			//select Card
			selectTag(uid);

			//auth
			if(auth(0x60,4,key,uid) == MI_OK){
				//write data
				if(write(4,wData) == MI_OK){
					printf("Write data success!\n");
					//isTrue = false;
				}
				
				//read data
				if(read(4,str) == MI_OK){
					printf("Hex:");
					for(i = 0;i < 16;i++)
						printf("%x",str[i]);
					printf("\n");
					printf("Data:%s\n",str);
				}
			}
			else{
				printf("Auth faild.\n");
			}
				

		}

		halt();

	}


	bcm2835_spi_end();
	bcm2835_close();

	return 0;
}