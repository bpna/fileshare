#!/bin/sh

# script for client-1 for COMP112 demo on 5/1

echo "ls -lh"
ls -lh

read -n1 ans

echo "./client init $1 $2"
./client init $1 $2

read -n1 ans

echo "./client new_client fahad topsecretpassword"
./client new_client fahad topsecretpassword

read -n1 ans

echo "./client upload_file fahad topsecretpassword final_exam.tex"
./client upload_file fahad topsecretpassword final_exam.tex

read -n1 ans

echo "./client upload_file fahad topsecretpassword TCPfaststart.docx"
./client upload_file fahad topsecretpassword TCPfaststart.docx

read -n1 ans

mkdir requested_files
cd requested_files
pwd

echo "../client checkout_file fahad topsecretpassword fahad final_exam.tex"
../client checkout_file fahad topsecretpassword fahad final_exam.tex

read -n1 ans

echo "ls -lh"
ls -lh

read -n1 ans

echo "diff final_exam.tex ../final_exam.tex"
diff final_exam.tex ../final_exam.tex

read -n1 ans

dd if=/dev/urandom of=./final_exam.tex count 653 bs=1024

echo "../client upload_file fahad topsecretpassword final_exam.tex"
../client upload_file fahad topsecretpassword final_exam.tex

read -n1 ans

cd ..
rm -rf requested_files
