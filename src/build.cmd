@echo off

cls
gcc -shared -O2 -mno-clwb -IC:\lua\include -s vanir.c input.c hooks.c -o vanir.dll -LC:\lua -llua54 -lgdi32
echo compiled vanir.dll