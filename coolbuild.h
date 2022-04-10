#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SPAN_TYPE(T) \
    struct { \
        T* begin; \
        T* end; \
        size_t length; \
    }

#define SPAN_FROM_BOUNDS(begin, end) \
    { (begin), (end), (end) - (begin) }

#define SPAN_WITH_LENGTH(begin, length) \
    { (begin), (begin) + (length), (length) }

#define BUFFER_TYPE(T) \
    struct { \
        T* data; \
        size_t length; \
        size_t capacity; \
    }

#define BUFFER_INIT \
    { NULL, 0, 0 }

#define BUFFER_FREE(buffer) free(buffer.data)

#define BUFFER_EXPAND(buffer, newlen) \
    do { \
        while (newlen > (buffer)->capacity) { \
            (buffer)->capacity = (buffer)->capacity * 2 + 1; \
            (buffer)->data = \
                    realloc((buffer)->data, (buffer)->capacity * sizeof((buffer)->data[0])); \
        } \
    } while (false)

#define BUFFER_PUSH(buffer, value) \
    do { \
        BUFFER_EXPAND(buffer, (buffer)->length + 1); \
        (buffer)->data[(buffer)->length] = (value); \
        (buffer)->length++; \
    } while (false)

#define BUFFER_APPEND(buffer, value, len) \
    do { \
        BUFFER_EXPAND(buffer, (buffer)->length + len); \
        memcpy((buffer)->data + (buffer)->length, (value), (len) * sizeof((buffer)->data[0])); \
        (buffer)->length += (len); \
    } while (false)

#define BUFFER_AS_SPAN(buffer) SPAN_WITH_LENGTH((buffer).data, (buffer).length)

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_LENGTH (sizeof PATH_SEPARATOR - 1)

#define TEMP_BUFFER_SIZE 1024

#define FOREACH_VARARGS(type, arg, begin, body) \
    do { \
        va_list args; \
        va_start(args, begin); \
        for (type arg = va_arg(args, type); arg != NULL; arg = va_arg(args, type)) { \
            body; \
        } \
        va_end(args); \
    } while (false)

#define FOREACH_ARRAY(type, item, items, body) \
    do { \
        for (size_t i_ = 0; i_ < sizeof(items) / sizeof(type); i_++) { \
            type item = (items)[i_]; \
            body; \
        } \
    } while (false)

#define FOREACH_FILE_IN_DIR(file, dirpath, body) \
    do { \
        struct dirent* dp = NULL; \
        DIR* dir = opendir(dirpath); \
        while ((dp = readdir(dir)) != NULL) { \
            const char* file = dp->d_name; \
            body; \
        } \
    } while (false)

#define REBUILD_SELF(argc, argv) \
    do { \
        const char* source_path = __FILE__; \
        const char* binary_path = argv[0]; \
        struct stat statbuf; \
        if (stat(source_path, &statbuf) < 0) { \
            fprintf(stderr, "Could not stat %s: %s\n", source_path, strerror(errno)); \
        } \
        int source_mtime = statbuf.st_mtime; \
        if (stat(binary_path, &statbuf) < 0) { \
            fprintf(stderr, "Could not stat %s: %s\n", binary_path, strerror(errno)); \
        } \
        int binary_mtime = statbuf.st_mtime; \
        if (source_mtime > binary_mtime) { \
            rename(binary_path, CONCAT(binary_path, ".old")); \
            CMD("cc", source_path, "-o", binary_path); \
            echo_cmd(argv); \
            execv(argv[0], argv); \
        } \
    } while (false)

#define COMPILE_OBJECTS(sources, flags, objects) \
    do { \
        *(objects) = malloc(sizeof(const char*) * (sizeof(sources) / sizeof(const char*) + 1)); \
        size_t i = 0; \
        FOREACH_ARRAY(const char*, source, sources, { \
            const char* object = PATH("build", "obj", CONCAT(source, ".o")); \
            CMD(cc(), "-c", flags, source, "-o", object); \
            (*(objects))[i] = object; \
            i++; \
        }); \
        (*(objects))[i] = NULL; \
    } while (false)

static inline const char* vconcat_sep_impl(const char* sep, va_list args) {
    size_t length = 0;
    int64_t seps = -1;
    size_t sep_len = strlen(sep);

    va_list args2;
    va_copy(args2, args);
    for (const char* arg = va_arg(args2, const char*); arg != NULL;
            arg = va_arg(args2, const char*)) {
        if (arg[0] != '\0') {
            length += strlen(arg);
            seps += 1;
        }
    }
    va_end(args2);

    char* result = malloc(length + (seps * sep_len) + 1);

    length = 0;
    for (const char* arg = va_arg(args, const char*); arg != NULL;
            arg = va_arg(args, const char*)) {
        if (arg[0] != '\0') {
            size_t n = strlen(arg);
            memcpy(result + length, arg, n);
            length += n;
            if (seps > 0) {
                memcpy(result + length, sep, sep_len);
                length += sep_len;
                seps -= 1;
            }
        }
    }
    result[length] = '\0';

    return result;
}

static inline const char* concat_sep_impl(const char* sep, ...) {
    va_list args;
    va_start(args, sep);
    const char* result = vconcat_sep_impl(sep, args);
    va_end(args);

    return result;
}

#define CONCAT_SEP(sep, ...) concat_sep_impl(sep, __VA_ARGS__, NULL)

#define PATH(...) CONCAT_SEP(PATH_SEPARATOR, __VA_ARGS__)

static inline void mkdirs_impl(int ignore, ...) {
    size_t length = 0;
    int64_t seps = -1;

    FOREACH_VARARGS(const char*, arg, ignore, {
        length += strlen(arg);
        seps += 1;
    });

    char* result = malloc(length + (seps * PATH_SEPARATOR_LENGTH) + 1);

    length = 0;
    FOREACH_VARARGS(const char*, arg, ignore, {
        size_t n = strlen(arg);
        memcpy(result + length, arg, n);
        length += n;

        result[length] = '\0';
        if (mkdir(result, 0755) < 0) {
            if (errno != EEXIST) {
                fprintf(stderr, "[CB][ERROR] Could not create directory '%s': %s\n", result,
                        strerror(errno));
                exit(1);
            }
        } else {
            printf("[CB][INFO] mkdir %s\n", result);
        }

        if (seps > 0) {
            memcpy(result + length, PATH_SEPARATOR, PATH_SEPARATOR_LENGTH);
            length += PATH_SEPARATOR_LENGTH;
            seps -= 1;
        }
    });
}

#define MKDIRS(...) mkdirs_impl(0, __VA_ARGS__, NULL)

static inline const char* concat_impl(int ignore, ...) {
    size_t length = 0;

    FOREACH_VARARGS(const char*, arg, ignore, { length += strlen(arg); });

    char* result = malloc(length + 1);

    length = 0;
    FOREACH_VARARGS(const char*, arg, ignore, {
        size_t n = strlen(arg);
        memcpy(result + length, arg, n);
        length += n;
    });
    result[length] = '\0';

    return result;
}

#define CONCAT(...) concat_impl(0, __VA_ARGS__, NULL)

static inline const char* coolbuild_exec(char** argv, bool capture) {
    int pipefds[2];
    if (capture) {
        if (pipe(pipefds) < 0) {
            fprintf(stderr, "Could not create pipe: %s\n", strerror(errno));
            exit(1);
        }
    }
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "[CB][ERROR] Could not fork: %s\n", strerror(errno));
        exit(1);
    }
    if (pid == 0) {
        if (capture) {
            close(pipefds[0]);
            dup2(pipefds[1], STDOUT_FILENO);
        }
        execvp(argv[0], argv);
        fprintf(stderr, "[CB][ERROR] Could not execute %s: %s\n", argv[0], strerror(errno));
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "[CB][ERROR] Subcommand reported non-zero status %d\n", status);
        exit(WEXITSTATUS(status));
    }

    if (capture) {
        close(pipefds[1]);
        BUFFER_TYPE(char) result = BUFFER_INIT;
        while (true) {
            char buffer[TEMP_BUFFER_SIZE];
            ssize_t n = read(pipefds[0], buffer, TEMP_BUFFER_SIZE);
            if (n < 0) {
                fprintf(stderr, "[CB][ERROR] Could not read from pipe: %s\n", strerror(errno));
                exit(1);
            }
            if (n == 0) {
                break;
            }
            BUFFER_APPEND(&result, buffer, n);
        }
        BUFFER_PUSH(&result, '\0');
        return result.data;
    }
    return NULL;
}

static inline void echo_cmd(char** argv) {
    printf("[CB][INFO]");
    for (size_t i = 0; argv[i] != NULL; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
}

static inline const char* cmd_impl(int capture, ...) {
    size_t argc = 0;

    FOREACH_VARARGS(const char*, arg, capture, {
        if (arg[0] != '\0') {
            argc += 1;
        }
    });
    assert(argc >= 1);

    char** argv = malloc(sizeof(char*) * (argc + 1));

    argc = 0;
    FOREACH_VARARGS(char*, arg, capture, {
        if (arg[0] != '\0') {
            argv[argc] = arg;
            argc += 1;
        }
    });
    argv[argc] = NULL;

    va_list args;
    va_start(args, capture);
    const char* command_message = vconcat_sep_impl(" ", args);
    va_end(args);
    printf("[CB][INFO] %s\n", command_message);

    return coolbuild_exec(argv, capture);
}

#define CMD(...) cmd_impl(0, __VA_ARGS__, NULL)
#define CMD_OUTPUT(...) cmd_impl(1, __VA_ARGS__, NULL)

static inline void run_command(char** argv) {
    echo_cmd(argv);
    coolbuild_exec(argv, false);
}

static inline char** split_args(const char* args) {
    size_t argc = 0;
    size_t length = strlen(args);
    size_t start = 0;
    for (size_t i = 0; i < length; i++) {
        if (isspace(args[i]) && start < i) {
            argc += 1;
            while (isspace(args[i])) {
                i += 1;
            }
            start = i;
            i -= 1;
        }
    }
    argc += 1;

    char** argv = malloc(sizeof(char*) * (argc + 1));

    argc = 0;
    start = 0;
    for (size_t i = 0; i < length; i++) {
        if (isspace(args[i]) && start < i) {
            argv[argc] = malloc(i - start + 1);
            memcpy(argv[argc], args + start, i - start);
            argv[argc][i - start] = '\0';
            argc += 1;
            while (isspace(args[i])) {
                i += 1;
            }
            start = i;
            i -= 1;
        }
    }
    if (start < length) {
        argv[argc] = malloc(length - start + 1);
        memcpy(argv[argc], args + start, length - start);
        argv[argc][length - start] = '\0';
        argc += 1;
    }
    argv[argc] = NULL;

    return argv;
}

static inline char** collect_args(char* fmt, ...) {
    size_t length = 0;
    va_list args;
    va_start(args, fmt);
    size_t fmtlen = strlen(fmt);
    for (size_t i = 0; i < fmtlen; i++) {
        switch (fmt[i]) {
            case 'v':
                (void)va_arg(args, char*);
                length += 1;
                break;
            case 'p': {
                char** argv = va_arg(args, char**);
                while (*argv != NULL) {
                    length += 1;
                    argv++;
                }
            } break;
            default:
                assert(false && "illegal format character");
        }
    }
    va_end(args);

    char** result = malloc(sizeof(char*) * (length + 1));
    va_start(args, fmt);
    size_t j = 0;
    for (size_t i = 0; i < fmtlen; i++) {
        switch (fmt[i]) {
            case 'v':
                result[j] = va_arg(args, char*);
                j++;
                break;
            case 'p': {
                char** temp = va_arg(args, char**);
                while (*temp != NULL) {
                    result[j] = *temp;
                    j++;
                    temp++;
                }
            } break;
        }
    }
    va_end(args);
    result[j] = NULL;

    return result;
}
