# Ptrace Userland Rootkit Techniques

Proof-of-concept exploits using Linux `ptrace()` API for process injection and memory manipulation. Developed for memory forensics detection testing.

## Components

### victim

Target process for injection testing.

- Runs `prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY)` to allow tracing by any process.


### shellcode_inject_fa

Foreign allocation injection.

- Uses `mmap()` to allocate an RX page in target memory.
- Writes reverse shell shellcode (192.168.100.1:4444).
- Executes shellcode, then restores victim process.


### shellcode_inject_rip

RIP overwrite injection.

- Overwrites target code at `$rip` with reverse shell shellcode.
- Executes and restores original code + registers.


### lib_inject

Dynamic library injection.

- Overwrites target code to call `dlopen()` on malicious library.
- Executes library, then restores original code.


### hollow

Process hollowing.

- Forks child process, stops it with `ptrace()`.
- Replaces child code with reverse shell shellcode.
- Resumes execution.



## Build

```bash
make all    # Build all components
make clean  # Clean build artifacts
```