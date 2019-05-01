#!/bin/sh

# script for client-2 for COMP112 demo on 5/1

echo "ls -lh"
ls -lh

read -n1 ans

echo "./client init $1 $2"
./client init $1 $2

read -n1 ans

echo "./client new_client jonah 1337hacker"
./client new_client jonah l337hacker

read -n1 ans

echo "./client upload_file jonah 1337hacker catnap-mobisys.pdf"
./client upload_file jonah 1337hacker catnap-mobisys.pdf

read -n1 ans

echo "./client user_list"
./client user_list

read -n1 ans

echo "./client file_list jonah 1337hacker fahad"
./client file_list jonah 1337hacker fahad

read -n1 ans

mkdir requested_files
cd requested_files
pwd

echo "../client request_file jonah 1337hacker fahad TCPfaststart.docx"
../client request_file jonah 1337hacker fahad TCPfaststart.docx

echo "ls -lh"
ls -lh

read -n1 ans

echo "../client request_file jonah 1337hacker fahad final_exam.tex"
../client request_file jonah 1337hacker fahad final_exam.tex

read -n1 ans

echo "../client request_file jonah 1337hacker fahad final_exam.tex"
../client request_file jonah 1337hacker fahad final_exam.tex

read -n1 ans

echo "ls -lh"
ls -lh

read -n1 ans

cd ..
rm -rf requested_files
