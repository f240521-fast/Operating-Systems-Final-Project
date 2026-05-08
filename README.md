# Operating-Systems-Final-Project
# Hospital Patient Triage & Bed Allocator
### CL2006 – Operating Systems Lab | Spring 2026
### FAST-NUCES, CFD Campus

## Group Members
- Member 1: Abdul Rafay Khan (24F-0521)
- Member 2: Rafay jawad (24F-0759)

## Project Description
A system-level C program that simulates hospital emergency room operations.
It covers process management, IPC, CPU scheduling, synchronization, and
memory allocation using real OS concepts.

## Files
- `admissions.c` – Main admissions manager process
- `patient_simulator.c` – Patient process (forked per admission)
- `bed_allocator.h` – Shared structs and constants
- `triage.sh` – Computes triage priority and pipes data to admissions
- `start_hospital.sh` – Initializes IPC and launches the system
- `stop_hospital.sh` – Shuts down system and cleans up IPC resources
- `Makefile` – Build system

## How to Run
```bash
make run
# or manually:
./start_hospital.sh
./triage.sh   
```

## How to Stop
```bash
./stop_hospital.sh
```

## Memory Strategy Selection
```bash
./admissions --strategy best
./admissions --strategy first
./admissions --strategy worst
```

## OS Concepts Demonstrated
- `fork()` / `execv()` for process creation
- Anonymous pipes and named FIFOs for IPC
- Shared memory for bed bitmap
- POSIX threads (receptionist, scheduler, nurse pool)
- Mutex and condition variables for synchronization
- Semaphores for ICU/Isolation capacity control
- Best-Fit, First-Fit, Worst-Fit memory allocation
- Coalescing and fragmentation reporting
- Paging simulation with internal fragmentation

## Dependencies
- GCC with POSIX support
- Linux OS (Ubuntu recommended)
- `pthread`, `semaphore`, `sys/shm` libraries (standard on Linux)
