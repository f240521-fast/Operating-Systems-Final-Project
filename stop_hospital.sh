#!/bin/bash
# ============================================================
# Project : Hospital Patient Triage & Bed Allocator
# Script : start-hospital.sh# Group : Group XX
# Members : Abdul Rafay Khan (24F -0521) , Rafay Jawad (24F -0759)
# Date : 2026 -04 -01
# Purpose : Ending the System
# Usage : ./ stop_hospital.sh 
# ============================================================

if [ ! -f bin/hospital.pid ]; then
    echo "No hospital PID found. Is it running?"
    exit 1
fi

PID=$(cat bin/hospital.pid)

echo "Sending SIGTERM to Admissions (PID: $PID)..."
kill -15 $PID
sleep 1

# Clean IPC
ipcrm -M 0xBEDF00D 2>/dev/null
echo "Shared memory segment 0xBEDF00D destroyed."

rm -f /tmp/triage_fifo /tmp/discharge_fifo
echo "Named Pipes removed."

rm bin/hospital.pid
echo "--------------------------------------------------------"
echo "Hospital system completely shut down. Logs saved."
echo "--------------------------------------------------------"