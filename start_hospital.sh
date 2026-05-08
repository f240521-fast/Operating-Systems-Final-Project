#!/bin/bash
# ============================================================
# Project : Hospital Patient Triage & Bed Allocator
# Script : start-hospital.sh# Group : Group XX
# Members : Abdul Rafay Khan (24F -0521) , Rafay Jawad (24F -0759)
# Date : 2026 -04 -01
# Purpose : Sytarting the System
# Usage : ./ start_hospital.sh 
# ============================================================

echo "--------------------------------------------------------"
echo "           Hospital Patient Triage System  "
echo "--------------------------------------------------------"

mkdir -p bin logs

# Compile the project
make clean
make

# Setup IPC mechanisms
rm -f /tmp/triage_fifo /tmp/discharge_fifo
mkfifo /tmp/triage_fifo
mkfifo /tmp/discharge_fifo



> logs/schedule_log.txt
> logs/memory_log.txt

./bin/admissions &
ADMISSIONS_PID=$!
echo $ADMISSIONS_PID > bin/hospital.pid

echo "Admissions Manager launched successfully (PID: $ADMISSIONS_PID)."
echo "--------------------------------------------------------"
echo "You may now run ./scripts/triage.sh or ./scripts/stress_test.sh"
echo "--------------------------------------------------------"