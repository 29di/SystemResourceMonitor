SHELL := /bin/bash

CC=gcc
CXX=g++
CFLAGS=-O2 -Wall -Wextra -std=c11 -pthread
LDFLAGS=-pthread -lrt
CXXFLAGS=-O2 -Wall -Wextra -std=c++17

SRC_DIR=src
INC_DIR=include
BIN_DIR=bin
DATA_DIR=data
LOG_DIR=$(DATA_DIR)/logs
REPORT_DIR=$(DATA_DIR)/reports

MONITOR_SRC=$(SRC_DIR)/resource_monitor.c
MONITOR_BIN=$(BIN_DIR)/monitor

SCHED_SRC=$(SRC_DIR)/scheduler_simulator.cpp
SCHED_BIN=$(BIN_DIR)/scheduler

IPC_SRC=$(SRC_DIR)/ipc_consumer.c
IPC_BIN=$(BIN_DIR)/ipc_consumer

MAIN_SRC=$(SRC_DIR)/main.cpp
MAIN_BIN=$(BIN_DIR)/menu

.PHONY: all prepare monitor scheduler clean run_monitor

all: prepare monitor scheduler ipc menu

prepare:
	@mkdir -p $(BIN_DIR) $(LOG_DIR) $(REPORT_DIR)

monitor: $(MONITOR_BIN)

$(MONITOR_BIN): $(MONITOR_SRC) $(INC_DIR)/monitor.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(MONITOR_SRC) $(LDFLAGS)

scheduler: $(SCHED_BIN)

$(SCHED_BIN): $(SCHED_SRC) $(INC_DIR)/scheduler.h
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -o $@ $(SCHED_SRC)

ipc: $(IPC_BIN)

$(IPC_BIN): $(IPC_SRC)
	$(CC) $(CFLAGS) -o $@ $(IPC_SRC) -lrt

menu: $(MAIN_BIN)

$(MAIN_BIN): $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(MAIN_SRC)

run_monitor: monitor
	@echo "Starting Resource Monitor (Ctrl+C to stop)";
	$(MONITOR_BIN)

clean:
	rm -rf $(BIN_DIR)
