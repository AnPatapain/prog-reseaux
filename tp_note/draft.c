// C program to illustrate BEEP() function

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Driver Code
int main()
{

	char file_name[] = "text_dst.txt";
	FILE* fp = fopen(file_name, "w");
	
	char data_chunk[] = "ciao\nbonjour";
	int bytes_written = fwrite(data_chunk, 1, strlen(data_chunk), fp);
	if(bytes_written != strlen(data_chunk)) {
		printf("writting error");
		return -1;
	}
	
	return 0;
}

