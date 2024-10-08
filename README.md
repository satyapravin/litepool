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

## PRELIMINARY RESULTS

Starting capital: 2000$, Max leverage: 5, Period: 10 hours

Strategy: Uses an LSTM and Soft Actor Critic to train agent to quote bid ask spreads with a fixed amount of 1% of starting capital once every second.

Data: 1st Sept 24, Bitmex, XBTUSDT contract

Assumption: 1bps rebate maker fee


### P/L without fees: Strategy (Red) vs BTC (Blue)
![image](https://github.com/user-attachments/assets/ce2cffb0-723c-49b5-8130-1627c27332b0)


### Leverage
![image](https://github.com/user-attachments/assets/e8636b3b-b22c-4c0b-a030-ddcdb02a03c8)

### Fees earned
![image](https://github.com/user-attachments/assets/098b68bd-8cec-4796-9181-9c6299f63206)



