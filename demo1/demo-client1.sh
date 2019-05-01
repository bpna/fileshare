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

echo "original size"
echo "ls -lh"
ls -lh

read -n1 ans

dd if=/dev/urandom of=./final_exam.tex count=653 bs=1024 2> /dev/null

echo "Fahad changes the file"
echo "ls -lh"
ls -lh

read -n1 ans

echo "../client update_file fahad topsecretpassword fahad final_exam.tex"
../client update_file fahad topsecretpassword fahad final_exam.tex

read -n1 ans


