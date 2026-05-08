/*
* ============================================================
* Project : Hospital Patient Triage & Bed Allocator
* File : admissions .c
* Group : Group 3
* Members : Abdul Rafay (24F -0521) , Rafay Jawad (24F -0759)
* Date : 2026 -05 -08
* Purpose : Central admissions manager - process spawning ,
* IPC , thread pool , scheduling , and bed allocation .
* Compile : gcc -Wall -o admissions admissions .c -lpthread
* ============================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include "../include/shared.h"


enum { BEST_FIT, FIRST_FIT, WORST_FIT };
int current_strategy = BEST_FIT;


WardMemory *ward;
int shmid;


PatientRecord p_queue[MAX_QUEUE];
int q_size = 0;


pthread_mutex_t ward_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_bed_freed = PTHREAD_COND_INITIALIZER;

sem_t sem_queue_items, sem_queue_slots;
sem_t sem_icu_limit, sem_iso_limit;


FILE *mem_log, *sched_log;
int patient_counter = 1;


double total_wait_time = 0;
double total_turnaround_time = 0;
int finished_patients = 0;



void log_memory_stats(const char* action, int p_id, int req_units) {
    if (!mem_log) return;

    int total_free = 0;
    int largest_free = 0;
    
    for(int i = 0; i < ward->partition_count; i++) {
        if(ward->partitions[i].is_free) {
            total_free += ward->partitions[i].size;
            if(ward->partitions[i].size > largest_free) {
                largest_free = ward->partitions[i].size;
            }
        }
    }

    float ext_frag = 0.0;
    if (total_free > 0 && total_free != largest_free) {
        ext_frag = (1.0 - ((float)largest_free / total_free)) * 100.0;
    }

    int pages_allocated = (req_units + PAGE_SIZE - 1) / PAGE_SIZE;
    int internal_wasted = (pages_allocated * PAGE_SIZE) - req_units;

    fprintf(mem_log, "--- Memory Report [%s] ---\n", action);
    fprintf(mem_log, "Patient ID: %d | Requested: %d units\n", p_id, req_units);
    fprintf(mem_log, "[Paging] Pages: %d | Internal Frag: %d units\n", pages_allocated, internal_wasted);
    fprintf(mem_log, "[Ward] Total Free: %d | External Frag: %.2f%%\n", total_free, ext_frag);
    fprintf(mem_log, "Active Partitions: %d\n", ward->partition_count);
    fprintf(mem_log, "---------------------------\n");
    fflush(mem_log);
}

/* --- MEMORY MANAGEMENT LOGIC --- */

void coalesce_memory() {
    for(int i = 0; i < ward->partition_count - 1; i++) {
        if(ward->partitions[i].is_free && ward->partitions[i+1].is_free && 
           strcmp(ward->partitions[i].bed_type, ward->partitions[i+1].bed_type) == 0) {
            
            ward->partitions[i].size += ward->partitions[i+1].size;
            for(int j = i + 1; j < ward->partition_count - 1; j++) {
                ward->partitions[j] = ward->partitions[j+1];
            }
            ward->partition_count--;
            i--; 
            }
    }
}

int find_bed_idx(int units, char* type) {
    int target = -1;
    if (current_strategy == FIRST_FIT) {
        for(int i=0; i < ward->partition_count; i++) {
            if(ward->partitions[i].is_free && ward->partitions[i].size >= units && strcmp(ward->partitions[i].bed_type, type) == 0) 
                return i;
        }
    } else if (current_strategy == BEST_FIT) {
        int min_diff = 9999;
        for(int i=0; i < ward->partition_count; i++) {
            if(ward->partitions[i].is_free && ward->partitions[i].size >= units && strcmp(ward->partitions[i].bed_type, type) == 0) {
                int diff = ward->partitions[i].size - units;
                if(diff < min_diff) { min_diff = diff; target = i; }
            }
        }
    } else if (current_strategy == WORST_FIT) {
        int max_diff = -1;
        for(int i=0; i < ward->partition_count; i++) {
            if(ward->partitions[i].is_free && ward->partitions[i].size >= units && strcmp(ward->partitions[i].bed_type, type) == 0) {
                int diff = ward->partitions[i].size - units;
                if(diff > max_diff) { max_diff = diff; target = i; }
            }
        }
    }
    return target;
}

int perform_allocation(int units, char* type) {
    int idx = find_bed_idx(units, type);
    if (idx == -1) return -1;

    int remaining = ward->partitions[idx].size - units;
    if (remaining > 0) {
        for(int j = ward->partition_count; j > idx + 1; j--) {
            ward->partitions[j] = ward->partitions[j-1];
        }
        ward->partitions[idx+1].size = remaining;
        ward->partitions[idx+1].start_unit = ward->partitions[idx].start_unit + units;
        ward->partitions[idx+1].is_free = 1;
        ward->partitions[idx+1].patient_id = -1;
        strcpy(ward->partitions[idx+1].bed_type, type);
        ward->partition_count++;
    }
    ward->partitions[idx].size = units;
    ward->partitions[idx].is_free = 0;
    return idx;
}

/* --- THREAD FUNCTIONS --- */

void* receptionist_thread(void* arg) {
    mkfifo(TRIAGE_FIFO, 0666);
    int fd = open(TRIAGE_FIFO, O_RDWR);
    FILE* stream = fdopen(fd, "r");
    char buf[256];

    while(fgets(buf, sizeof(buf), stream)) {
        PatientRecord rec;
        if(sscanf(buf, "%s %d %d %d", rec.name, &rec.age, &rec.priority, &rec.care_units) == 4) {
            rec.patient_id = patient_counter++;
            rec.arrival_time = time(NULL);
            
            sem_wait(&sem_queue_slots);
            pthread_mutex_lock(&queue_mutex);
            
            int i = q_size++;
            while(i > 0 && p_queue[i-1].priority < rec.priority) {
                p_queue[i] = p_queue[i-1];
                i--;
            }
            p_queue[i] = rec;
            
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&sem_queue_items);
            printf("[Receptionist] %s (ID:%d) added to Priority Queue.\n", rec.name, rec.patient_id);
            fflush(stdout);
        }
    }
    return NULL;
}

void* scheduler_thread(void* arg) {
    while(1) {
        sem_wait(&sem_queue_items);
        pthread_mutex_lock(&queue_mutex);
        PatientRecord rec = p_queue[0];
        for(int i=0; i < q_size-1; i++) p_queue[i] = p_queue[i+1];
        q_size--;
        pthread_mutex_unlock(&queue_mutex);
        sem_post(&sem_queue_slots);

        // Map Bed Type and check Semaphores
        char type[16];
        if(rec.priority >= 8) { 
            strcpy(type, "ICU"); 
            sem_wait(&sem_icu_limit); 
        } else if(rec.care_units <= 2) { 
            strcpy(type, "ISOLATION"); 
            sem_wait(&sem_iso_limit); 
        } else {
            strcpy(type, "GENERAL");
        }

        int bed_idx = -1;
        pthread_mutex_lock(&ward_mutex);
        
        while((bed_idx = perform_allocation(rec.care_units, type)) == -1) {
            printf("[Scheduler] %s (ID:%d) waiting for %s bed availability...\n", rec.name, rec.patient_id, type);
            fflush(stdout);
            pthread_cond_wait(&cond_bed_freed, &ward_mutex); 
        }
        
        ward->partitions[bed_idx].patient_id = rec.patient_id;
        total_wait_time += difftime(time(NULL), rec.arrival_time);
        
        // Logging
        if(sched_log) {
            fprintf(sched_log, "ADMITTED: %s | ID: %d | Bed: %d (%s) | Strategy: %d\n", 
                    rec.name, rec.patient_id, bed_idx, type, current_strategy);
            fflush(sched_log);
        }
        log_memory_stats("ALLOCATION", rec.patient_id, rec.care_units);
        
        pthread_mutex_unlock(&ward_mutex);

        printf("[Scheduler] Admitted %s to %s (Partition %d)\n", rec.name, type, bed_idx);
        fflush(stdout);

        // Simulation Fork
        if(fork() == 0) {
            char pid_s[10], bid_s[10], arr_s[20];
            sprintf(pid_s, "%d", rec.patient_id);
            sprintf(bid_s, "%d", bed_idx);
            sprintf(arr_s, "%ld", rec.arrival_time);
            execl("./bin/patient_simulator", "patient_simulator", pid_s, rec.name, bid_s, type, arr_s, NULL);
            exit(0);
        }
    }
}

void* nurse_thread(void* arg) {
    mkfifo(DISCHARGE_FIFO, 0666);
    int fd = open(DISCHARGE_FIFO, O_RDWR);
    int p_id; 
    long arrival;

    while(read(fd, &p_id, sizeof(int)) > 0) {
        read(fd, &arrival, sizeof(long));
        
        pthread_mutex_lock(&ward_mutex);
        int found = 0;
        for(int i=0; i < ward->partition_count; i++) {
            if(ward->partitions[i].patient_id == p_id) {
                ward->partitions[i].is_free = 1;
                ward->partitions[i].patient_id = -1;
                
                // Return Semaphore slot
                if(strcmp(ward->partitions[i].bed_type, "ICU") == 0) sem_post(&sem_icu_limit);
                else if(strcmp(ward->partitions[i].bed_type, "ISOLATION") == 0) sem_post(&sem_iso_limit);
                
                total_turnaround_time += difftime(time(NULL), (time_t)arrival);
                if(total_turnaround_time < 0){total_turnaround_time = 0; }
                finished_patients++;
                
                printf("[Nurse] Discharged Patient %d. Avg Wait: %.2fs | Avg TAT: %.2fs\n", 
                        p_id, total_wait_time/finished_patients, total_turnaround_time/finished_patients);
                fflush(stdout);
                if(sched_log) {
            fprintf(sched_log, "| Patient: %d", 
                    p_id);
            fflush(sched_log);
        }
                coalesce_memory();
                log_memory_stats("DEALLOCATION", p_id, 0);
                
                pthread_cond_broadcast(&cond_bed_freed);
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&ward_mutex);
        if(!found) printf("[Nurse] Warning: Patient %d not found for discharge!\n", p_id);
    }
    return NULL;
}

/* --- MAIN ENTRY --- */

int main(int argc, char* argv[]) {
    if(argc > 1) {
        if(strcmp(argv[1], "--first") == 0) current_strategy = FIRST_FIT;
        else if(strcmp(argv[1], "--worst") == 0) current_strategy = WORST_FIT;
    }
    
    mkdir("logs", 0777);
    mem_log = fopen("logs/memory_log.txt", "w");
    sched_log = fopen("logs/scheduler_log.txt", "w");
    
    // Setup Shared Memory
    shmid = shmget(SHM_KEY, sizeof(WardMemory), IPC_CREAT | 0666);
    ward = shmat(shmid, NULL, 0);
    
    // Initialize Ward Partitions
    ward->partition_count = 3;
    ward->partitions[0] = (BedPartition){0, 0, 12, 1, -1, "ICU"};
    ward->partitions[1] = (BedPartition){1, 12, 8, 1, -1, "ISOLATION"};
    ward->partitions[2] = (BedPartition){2, 20, 12, 1, -1, "GENERAL"};

    // Initialize Semaphores
    sem_init(&sem_queue_items, 0, 0);
    sem_init(&sem_queue_slots, 0, MAX_QUEUE);
    sem_init(&sem_icu_limit, 0, 4); // Max ICU Capacity
    sem_init(&sem_iso_limit, 0, 4); // Max Isolation Capacity

    // Spawn Roles
    pthread_t r, s, n;
    pthread_create(&r, NULL, receptionist_thread, NULL);
    pthread_create(&s, NULL, scheduler_thread, NULL);
    pthread_create(&n, NULL, nurse_thread, NULL);

    const char* str_name = (current_strategy == FIRST_FIT ? "First-Fit" : 
                           (current_strategy == WORST_FIT ? "Worst-Fit" : "Best-Fit"));
    printf("🏥 Hospital System Active | Strategy: %s\n", str_name);
    
    pthread_join(r, NULL); // Keep main alive
    
    if(mem_log) fclose(mem_log);
    if(sched_log) fclose(sched_log);
    return 0;
}