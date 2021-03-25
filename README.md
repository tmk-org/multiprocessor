# multiprocessor
Command data streamer with multi process and shared memory management 

# Build with cmake and test

```
rm -rf build; mkdir -p build; cd build
cmake ..
make -j4
ctest --output-on-failure
```

# Examples

```
cd build/bin
./server
```
Child processes ./capture and ./filter are started automatically.
Send pkill to server from other terminal.
```
pkill ./server
```
Tape the last data. All child processes (capturer and filter) and parent (server) should be killed by exit.

# Camera and capturer

Camera: 
```
export TEST_SOURCE_PATH="/mnt/testdb/pencil_frames_5" && ./cvcamera 9999
```
Please, wait while init

Capturer:
```
./cvcapture localhost 9999
```
Message is ok!!!
Could not connect
[cvclient]: Can't init collector, may be camera isn't ready. Please, try again.
