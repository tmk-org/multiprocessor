# multiprocessor
Command data streamer with multi process and shared memory management 

#Build
Сервер:
```
cd proc_serv
gcc -Wall -o server *.c -lrt -pthread
./server
```
Модули-клиенты (стартуют в тесте автоматически):
```
cd proc_client
gcc -Wall -o capturer ../proc_serv/shared_memory.h capturer.c
gcc -Wall -o filter ../proc_serv/shared_memory.h filter.c
```
