cmake_minimum_required(VERSION 3.11.4)
project (05-non-systemd-example)
add_executable(simple-daemon main.c util.c)
