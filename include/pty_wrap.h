#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

int  pty_spawn(const char* shell, int* out_master_fd, int* out_child_pid);
int  pty_set_winsize(int master_fd, int rows, int cols);
ssize_t pty_write(int master_fd, const void* buf, size_t n);
ssize_t pty_read(int master_fd, uint8_t* buf, size_t cap);

