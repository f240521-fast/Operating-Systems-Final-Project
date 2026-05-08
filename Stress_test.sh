#!/bin/bash
echo "Firing Stress Test (20 Patients)..."
NAMES=("Ali" "Sara" "Omar" "Fatima" "Zayn" "Aisha" "Bilal" "Hira" "Usman" "Noor" 
       "Kamran" "Sana" "Tariq" "Maha" "Hassan" "Zara" "Waleed" "Nida" "Hamza" "Iqra")

for i in {0..19}; do
    ./scripts/triage.sh "${NAMES[$i]}" $((RANDOM%50+10)) $((RANDOM%10+1)) &
    sleep 0.1
done
wait
echo "All sent."