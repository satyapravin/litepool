# LitePool
An all encompassing bareboned lightweight port of C++ RL environment pool relying on CMake only.
It allows users to create their own environments in C++ for use with popular RL Python libraries.

## BUILD on Linux:
1. Build all third Party and install
2. Create folder build on root litepool
3. cmake ..
4. Make
5. Copy *.so from build/lib to litepool/dummy and litepool/rltrader_litepool.so respectively
6. pip install .

