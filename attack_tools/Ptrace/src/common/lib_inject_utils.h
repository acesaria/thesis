
int inject_library(pid_t pid, void *dlopen_addr, const char *library_path);
void *find_dlopen(pid_t target_pid);
