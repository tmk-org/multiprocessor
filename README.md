# multiprocessor
Command data streamer with multi process and shared memory management 

# Build

Сервер:
```
cd proc_serv
gcc -Wall -o server *.c -lrt -pthread
./server
```
Модули-клиенты (стартуют в тесте автоматически):
```
cd proc_client
gcc -Wall -o capturer ../proc_server/shared_memory.c capturer.c -lrt -pthread
gcc -Wall -o filter ../proc_server/shared_memory.c filter.c -lrt -pthread
```
# Run test
```
cd proc_serv
./server
```
# Finish
Send pkill to server from other terminal.
```
pkill ./server
```
Tape the last data. All child processes (capturer and filter) and parent (server) should be killed by exit.
