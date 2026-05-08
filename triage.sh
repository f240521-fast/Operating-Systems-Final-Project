#!/bin/bash

# ============================================================
# Project : Hospital Patient Triage & Bed Allocator
# Script : triage .sh
# Group : Group XX
# Members : Ali Hassan (24F -0521) , Sara Khan (24F -0759)
# Date : 2026 -04 -01
# Purpose : Compute triage priority and pipe patient data
# to the admissions manager process .
# Usage : ./ triage .sh <name > <age > <severity 1 -10 >
# ============================================================
FIFO="/tmp/triage_fifo"
if [ ! -p $FIFO ]; then mkfifo $FIFO; fi

NAME=$1
AGE=$2
SEV=$3
UNITS=$(( (RANDOM % 4) + 1 ))

if [[ -z "$NAME" || -z "$AGE" || -z "$SEV" ]]; then
    echo "Usage: ./scripts/triage.sh [Name] [Age] [Severity 1-10]"
    exit 1
fi

echo "$NAME $AGE $SEV $UNITS" > $FIFO