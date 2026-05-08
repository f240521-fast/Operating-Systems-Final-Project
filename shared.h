#ifndef SHARED_H
#define SHARED_H

#include <time.h>

#define SHM_KEY 0xBEDF00D
#define MAX_QUEUE 50
#define PAGE_SIZE 4
#define MAX_PARTITIONS 100

#define TRIAGE_FIFO "/tmp/triage_fifo"
#define DISCHARGE_FIFO "/tmp/discharge_fifo"

typedef struct {
    char name[50];
    int age;
    int priority;
    int care_units;
    int patient_id;
    long arrival_time;
} PatientRecord;

typedef struct {
    int partition_id;
    int start_unit;
    int size;
    int is_free;
    int patient_id;
    char bed_type[16]; // ICU, ISOLATION, GENERAL
} BedPartition;

typedef struct {
    BedPartition partitions[MAX_PARTITIONS];
    int partition_count;
} WardMemory;

#endif