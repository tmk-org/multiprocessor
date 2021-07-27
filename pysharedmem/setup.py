#python3 #https://docs.python.org/3/extending/building.html

from distutils.core import setup, Extension
from os import path

module = Extension('dclprocessor',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                   sources = ['py_wrapper.cpp', 'ObjectDescriptorWrapper.cpp', 'FramesetWrapper.cpp', 'FrameWrapper.cpp'],
                   include_dirs = [
                       '/home/hedonistant/PycharmProjects/sharedmem/src',
                       '/home/hedonistant/PycharmProjects/mandrel_motion_detect/venv/lib/python3.8/site-packages/numpy/core/include/',
                       '/home/hedonistant/libs/glog/build',
                       '/home/hedonistant/libs/glog/src',
                       '/usr/include/opencv4',
                   ],
                   libraries = ['test',  'glog', 'opencv_imgproc', 'opencv_highgui'],
                   library_dirs = [
                       '/home/hedonistant/PycharmProjects/sharedmem/src',
                       '/home/hedonistant/libs/glog/build',
                   ],
                   extra_compile_args=['--std=c++17'],
                   language="g++")


setup(name='dclprocessor',
    version='1.0',
    description = 'This is a dclprocessor py package',
    author = 'Ekaterina Mossyko',
    author_email = 'ekaterina.mosseyko@gmail.com',
    ext_modules=[
            module
        ]
)

#setup(name='fastprime',
#    ext_modules=[
#            Extension('fastprime',
#                    ['fastprime.c'],
#                    extra_objects=["c_prime.o"] # Relocatable compiled code of the c_prime to prevent recompiling each time
#                    )
#            ]
#)
