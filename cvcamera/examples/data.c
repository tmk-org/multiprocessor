#include <stdio.h>
#include <string.h>

#define TEST_DATA_CNT  8
#define TEST_HEADER_SIZE sizeof(int)
#define TEST_RAW_SIZE 20
#define TEST_DATA_SIZE (TEST_HEADER_SIZE + TEST_RAW_SIZE)

int main(){
    char data[TEST_DATA_CNT][TEST_DATA_SIZE];
    char *buf = NULL;
    int data_size = TEST_DATA_SIZE;
    int i, j;
    for (i = 0; i < TEST_DATA_CNT; i++) {
        char *chunk = (char*)(data + i * TEST_DATA_SIZE); 
        memcpy((void*)chunk, (void*)&data_size, TEST_HEADER_SIZE);
        memcpy((void*)(chunk + TEST_HEADER_SIZE), (void*)&i, TEST_HEADER_SIZE);
        for (j = TEST_HEADER_SIZE*2; j < TEST_DATA_SIZE - 1; j++) {
            *(chunk + j) = 65 + i;
        }
        *(chunk + TEST_DATA_SIZE)='\0';
        buf = (char*)(data + i * TEST_DATA_SIZE);
        printf("[%d]: %d %d %s\n", i, *((int*)buf), *((int*)(buf+sizeof(int))), (char*)(buf + TEST_HEADER_SIZE*2)); 
    }
    return 0;
}
