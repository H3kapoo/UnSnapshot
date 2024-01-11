#!/bin/sh
cmake -B build -GNinja . # generate ninja data
cmake --build build      # link and compile
mv debug/UnSnapshot .      # move out of debug folder

# Enable dedicated gpu on linux, interchange between amd/nvidia
#DRI_PRIME=1 __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./exec