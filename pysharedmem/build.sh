#!/bin/bash -x

rm libtest.so.0
rm libtest.so

g++ -g -Wall -DMAJOR_VERSION=1 -DMINOR_VERSION=0 -fPIC -I/usr/include/opencv4 -c test.cpp -o test.o
g++ -shared -lopencv_highgui test.o  -Wl,-soname=libtest.so.0 -o libtest.so.0.1.0
ln -s libtest.so.0.1.0 libtest.so.0
ln -s libtest.so.0 libtest.so

python3 setup.py build
python3 setup.py install --user
LD_LIBRARY_PATH=".":${LD_LIBRAY_PATH} python3 test.py

#gcc -DNDEBUG -g -O3 -Wall -Wstrict-prototypes -fPIC -DMAJOR_VERSION=1 -DMINOR_VERSION=0 -I/usr/local/include -I/usr/local/include/python2.2 -c demo.c -o build/temp.linux-i686-2.2/demo.o
#gcc -shared build/temp.linux-i686-2.2/demo.o -L/usr/local/lib -ltcl83 -o build/lib.linux-i686-2.2/demo.so

#c->py
#https://docs.python.org/3/extending/extending.html#a-simple-example

#py->c
#http://www.cyberguru.ru/programming/cpp/embedding-python-into-c-part-i.html
