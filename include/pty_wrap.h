#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

// spawn child process attached to pty and return descriptors
int  pty_spawn(const char* shell, int* out_master_fd, int* out_child_pid);
// update terminal window size for child process
int  pty_set_winsize(int master_fd, int rows, int cols);
// write bytes to pty master
ssize_t pty_write(int master_fd, const void* buf, size_t n);
// read bytes from pty master
ssize_t pty_read(int master_fd, uint8_t* buf, size_t cap);

