#! /bin/bash

./client.o 3 &\
  sleep 2; ./client.o 5 &\
  sleep 4; ./client.o 10
