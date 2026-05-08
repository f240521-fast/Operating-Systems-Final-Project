/*
* ============================================================
* Project : Hospital Patient Triage & Bed Allocator
* File : admissions .c
* Group : Group 3
* Members : Abdul Rafay (24F -0521) , Rafay Jawad (24F -0759)
* Date : 2026 -05 -08
* Purpose : Central admissions manager - process spawning ,
* IPC , thread pool , scheduling , and bed allocation .
* Compile : gcc -Wall -o patient_simulator.c -lpthread
* ============================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "../include/shared.h"

int main(int argc, char* argv[]) {
    int p_id = atoi(argv[1]);
    char* name = argv[2];
    int b_id = atoi(argv[3]);
    char* type = argv[4];
    long arrival = atol(argv[5]);

    printf("->  [Patient %d: %s] Bed %d (%s) treatment start.\n", p_id, name, b_id, type);
    sleep((strcmp(type, "ICU") == 0) ? 6 : 3);

    int fd = open(DISCHARGE_FIFO, O_WRONLY);
    write(fd, &p_id, sizeof(int));
    write(fd, &arrival, sizeof(long));
    close(fd);

    printf("<-  [Patient %d: %s] Treatment done.\n", p_id, name);
    return 0;
}