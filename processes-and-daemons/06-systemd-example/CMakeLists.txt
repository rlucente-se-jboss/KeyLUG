cmake_minimum_required(VERSION 3.11.4)
project (06-systemd-example)
add_executable(simple-daemon main.c util.c)
install(TARGETS simple-daemon
	RUNTIME DESTINATION bin)
install(FILES simple-daemon.service
	DESTINATION /etc/systemd/system)
