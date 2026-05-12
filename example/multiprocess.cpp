#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>

#ifdef __linux__
#include <sys/shm.h>
#include <sys/ipc.h>
#elif __APPLE__
#include <sys/mman.h>
#include <fcntl.h>
#endif

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int child_task(int id, int input) {
    std::cout << "[Child " << id << "] PID=" << getpid() << " started, input=" << input << std::endl;
    int result = 0;
    if (input % 2 == 0) {
        result = input * 2;
        for (int i = 0; i < 1000; ++i) {
            result += i % 3;
        }
    } else {
        result = fibonacci(input % 20);
        for (int i = 0; i < 500; ++i) {
            if (i % 2 == 0) result += 1;
            else result -= 1;
        }
    }
    std::cout << "[Child " << id << "] result = " << result << std::endl;
    return result;
}

void test_pipes() {
    std::cout << "\n=== Test: Pipe Communication ===" << std::endl;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        int data = 42;
        std::cout << "[Child] Sending data: " << data << std::endl;
        write(pipefd[1], &data, sizeof(data));
        close(pipefd[1]);
        exit(0);
    } else {
        close(pipefd[1]);
        int received;
        read(pipefd[0], &received, sizeof(received));
        std::cout << "[Parent] Received data: " << received << std::endl;
        close(pipefd[0]);
        wait(nullptr);
    }
}

void test_shared_memory() {
    std::cout << "\n=== Test: Shared Memory ===" << std::endl;
#ifdef __linux__
    key_t key = ftok(".", 's');
    int shmid = shmget(key, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return;
    }
    int *shared = (int*)shmat(shmid, nullptr, 0);
    if (shared == (void*)-1) {
        perror("shmat");
        return;
    }
#elif __APPLE__
    int fd = shm_open("/cctracer_shared", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return;
    }
    ftruncate(fd, sizeof(int));
    int *shared = (int*)mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return;
    }
#endif

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        *shared = 100;
        std::cout << "[Child] Wrote to shared memory: " << *shared << std::endl;
#ifdef __linux__
        shmdt(shared);
#else
        munmap(shared, sizeof(int));
#endif
        exit(0);
    } else {
        wait(nullptr);
        std::cout << "[Parent] Read from shared memory: " << *shared << std::endl;
#ifdef __linux__
        shmdt(shared);
        shmctl(shmid, IPC_RMID, nullptr);
#else
        munmap(shared, sizeof(int));
        shm_unlink("/cctracer_shared");
#endif
    }
}

void test_multiple_forks() {
    std::cout << "\n=== Test: Multiple Forks ===" << std::endl;
    const int NUM_CHILDREN = 4;
    std::vector<pid_t> children;

    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            int result = child_task(i, i + 10);
            exit(result);
        } else if (pid > 0) {
            children.push_back(pid);
        } else {
            perror("fork");
        }
    }

    for (pid_t pid : children) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            std::cout << "[Parent] Child " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
        }
    }
}

void recursive_fork(int depth) {
    if (depth <= 0) return;
    std::cout << "[Process " << getpid() << "] depth=" << depth << std::endl;
    pid_t pid = fork();
    if (pid == 0) {
        recursive_fork(depth - 1);
        exit(0);
    } else if (pid > 0) {
        wait(nullptr);
    } else {
        perror("fork");
    }
}

int main() {
    std::cout << "=== CCTracer Multi-Process Test ===" << std::endl;
    std::cout << "Main process PID: " << getpid() << std::endl;

    test_pipes();

    test_shared_memory();

    test_multiple_forks();

    std::cout << "\n=== Test: Recursive Fork (depth=2) ===" << std::endl;
    recursive_fork(2);

    std::cout << "\n=== Test: system() call ===" << std::endl;
    int ret = system("echo 'Hello from system command'");
    std::cout << "system() returned " << ret << std::endl;

    std::cout << "\nAll tests completed." << std::endl;
    return 0;
}