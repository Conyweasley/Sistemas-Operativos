#!/bin/bash 

###################################################################
#Primer ejercicio
cat /proc/cpuinfo | grep "model name"
###################################################################
#Segundo ejercicio
cat /proc/cpuinfo | grep "model name" -wc
###################################################################
#Tercer ejercicio
wget https://www.gutenberg.org/files/11/11-0.txt -O texto.txt  &&sed -i 's/Alice/Cony/g' texto.txt && rm texto.txt
##Cuarto Ejercicio
sort -k 5 weather_cordoba.in | cut -f5 | { read first; echo "Mínimo: $first"; tail -n 1 | cut -f5 | echo "Máximo: $(cat)"; } | cut -d ' ' -f1-4 
##Quinto Ejercicio
sort -k 3 atpplayers.in
##Sexto Ejercicio
awk '{$9 = $7-$8;print}' superliga.in | sort -k 2,2nr -k 9nr | awk '{NF--;print}'
##Septimo Ejercicio
ip a | grep -o -m 1 -E 'ether ([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}' | grep -o -E '([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}'
##Octavo ejercicio
##a
mkdir Dr.\House && touch Dr.\House/fma_S01E{1..10}_es.srt
##b
cd Dr.\House && for i in *_es*; do mv "$i" "${i//_es}"; done