#define _XOPEN_SOURCE 600

#include <pty.h>        // forkpty helper
#include <unistd.h>     // execlp read write
#include <sys/ioctl.h>  // terminal sizing
#include <fcntl.h>      
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

int pty_spawn(const char* shell, int* out_master_fd, int* out_child_pid) {
    int master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid < 0) return -1;

    if (pid == 0) {
        // launch interactive shell in child
        if (shell == NULL) shell = "bash";
        execlp(shell, shell, (char*)NULL);
        _exit(127);
    }

    // switch master to non blocking mode
    int flags = fcntl(master_fd, F_GETFL, 0);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
    *out_master_fd = master_fd;
    *out_child_pid = pid;
    return 0;
}

int pty_set_winsize(int master_fd, int rows, int cols) {
    struct winsize ws = {0};
    ws.ws_row = (unsigned short)rows;
    ws.ws_col = (unsigned short)cols;
    // inform child of terminal dimensions
    return ioctl(master_fd, TIOCSWINSZ, &ws);
}

ssize_t pty_write(int master_fd, const void* buf, size_t n) {
    // forward bytes to pseudo terminal
    return write(master_fd, buf, n);
}

ssize_t pty_read(int master_fd, uint8_t* buf, size_t cap) {
    // read bytes produced by child process
    return read(master_fd, buf, cap); 
}

