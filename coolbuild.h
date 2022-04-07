#include <assert.h>
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

// Begin span.h

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

#define SPAN_FROM_ARRAY(array) SPAN_WITH_LENGTH(array, sizeof(array) / sizeof(array[0]))

// End span.h

// Begin buffer.h

#define BUFFER_TYPE(T) \
    struct { \
        T* data; \
        size_t length; \
        size_t capacity; \
    }

#define BUFFER_INIT \
    { NULL, 0, 0 }

#define BUFFER_FREE(buffer) free(buffer.data)

#define BUFFER_EXPAND(buffer) \
    do { \
        if ((buffer)->length == (buffer)->capacity) { \
            (buffer)->capacity = (buffer)->capacity * 2 + 1; \
            (buffer)->data = \
                    realloc((buffer)->data, (buffer)->capacity * sizeof((buffer)->data[0])); \
        } \
    } while (false)

#define BUFFER_PUSH(buffer, value) \
    do { \
        BUFFER_EXPAND(buffer); \
        (buffer)->data[(buffer)->length] = (value); \
        (buffer)->length++; \
    } while (false)

#define BUFFER_AS_SPAN(buffer) SPAN_WITH_LENGTH((buffer).data, (buffer).length)

// End buffer.h

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_LENGTH (sizeof PATH_SEPARATOR - 1)

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

const char* vconcat_sep_impl(const char* sep, va_list args) {
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

    return result;
}

const char* concat_sep_impl(const char* sep, ...) {
    va_list args;
    va_start(args, sep);
    const char* result = vconcat_sep_impl(sep, args);
    va_end(args);

    return result;
}

#define CONCAT_SEP(sep, ...) concat_sep_impl(sep, __VA_ARGS__, NULL)

#define PATH(...) CONCAT_SEP(PATH_SEPARATOR, __VA_ARGS__)

void mkdirs_impl(int ignore, ...) {
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

const char* concat_impl(int ignore, ...) {
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

void coolbuild_exec(char** argv) {
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "[CB][ERROR] Could not fork: %s\n", strerror(errno));
        exit(1);
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "[CB][ERROR] Could not execute %s: %s\n", argv[0], strerror(errno));
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "[CB][ERROR] Subcommand reported non-zero status %d\n", status);
        exit(1);
    }
}

void echo_cmd(char** argv) {
    printf("[CB][CMD]");
    for (size_t i = 0; argv[i] != NULL; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
}

void run_cmd(char** argv) {
    echo_cmd(argv);
    coolbuild_exec(argv);
}

void cmd_impl(int ignore, ...) {
    size_t argc = 0;

    FOREACH_VARARGS(const char*, arg, ignore, {
        if (arg[0] != '\0') {
            argc += 1;
        }
    });
    assert(argc >= 1);

    char** argv = malloc(sizeof(char*) * (argc + 1));

    argc = 0;
    FOREACH_VARARGS(char*, arg, ignore, {
        if (arg[0] != '\0') {
            argv[argc] = arg;
            argc += 1;
        }
    });
    argv[argc] = NULL;

    va_list args;
    va_start(args, ignore);
    const char* command_message = vconcat_sep_impl(" ", args);
    va_end(args);
    printf("[CB][CMD] %s\n", command_message);

    coolbuild_exec(argv);
}

#define CMD(...) cmd_impl(0, __VA_ARGS__, NULL)

char** collect_args(char* fmt, ...) {
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

typedef struct {
    size_t offset;
    size_t orig_length;
    const char* new_value;
    size_t new_value_len;
} coolbuild__config_replacement_t;

typedef BUFFER_TYPE(coolbuild__config_replacement_t) coolbuild__config_replacements_t;

void configure_file_impl(const char* input, const char* output, ...) {
    fprintf(stderr, "[CB][INFO] Configuring %s -> %s\n", input, output);

    FILE* input_file = fopen(input, "rb");
    if (input_file == NULL) {
        fprintf(stderr, "[CB][ERROR] Could not open file '%s' for configuring: %s\n", input,
                strerror(errno));
        exit(1);
    }

    int fd = fileno(input_file);
    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        fprintf(stderr, "[CB][ERROR] Could not stat file '%s' for configuring: %s\n", input,
                strerror(errno));
        exit(1);
    }

    char* input_contents = malloc(statbuf.st_size + 1);
    if (fread(input_contents, 1, statbuf.st_size, input_file) != (size_t)statbuf.st_size) {
        fprintf(stderr, "[CB][ERROR] Could not read file '%s' for configuring: %s\n", input,
                strerror(errno));
        exit(1);
    }
    input_contents[statbuf.st_size] = '\0';
    fclose(input_file);

    // iterate keys and values
    va_list args;
    va_start(args, output);
    coolbuild__config_replacements_t replacements = BUFFER_INIT;
    while (true) {
        const char* key = va_arg(args, const char*);
        if (key == NULL) {
            break;
        }
        const char* value = va_arg(args, const char*);
        assert(value != NULL && "incorrect args passed to CONFIGURE_FILE");

        // search the input string for @key@
        const char* search_key = CONCAT("@", key, "@");
        const char* iter = strstr(input_contents, search_key);
        size_t search_key_len = strlen(search_key);
        size_t value_len = strlen(value);

        while (iter != NULL) {
            // replace the key with the value
            coolbuild__config_replacement_t replacement = {
                    .offset = iter - input_contents,
                    .orig_length = search_key_len,
                    .new_value = value,
                    .new_value_len = value_len,
            };
            BUFFER_PUSH(&replacements, replacement);

            // search for the next key
            iter = strstr(iter + search_key_len, search_key);
        }
    }

    // replacements have been collected. apply them
    size_t output_len = statbuf.st_size;
    for (size_t i = 0; i < replacements.length; i++) {
        coolbuild__config_replacement_t replacement = replacements.data[i];
        output_len += replacement.new_value_len - replacement.orig_length;
    }
    char* output_contents = malloc(output_len + 1);
    size_t output_offset = 0;
    size_t input_offset = 0;
    for (size_t i = 0; i < replacements.length; i++) {
        coolbuild__config_replacement_t replacement = replacements.data[i];
        // copy what's in between
        memcpy(output_contents + output_offset, input_contents + input_offset,
                replacement.offset - input_offset);
        output_offset += replacement.offset - input_offset;
        input_offset = replacement.offset + replacement.orig_length;

        // copy the replacement
        memcpy(output_contents + output_offset, replacement.new_value, replacement.new_value_len);
        output_offset += replacement.new_value_len;
    }

    // copy remainder
    memcpy(output_contents + output_offset, input_contents + input_offset,
            statbuf.st_size - input_offset);

    FILE* output_file = fopen(output, "wb");
    if (output_file == NULL) {
        fprintf(stderr, "[CB][ERROR] Could not open file '%s' for configuring: %s\n", output,
                strerror(errno));
        exit(1);
    }

    if (fwrite(output_contents, 1, output_len, output_file) != output_len) {
        fprintf(stderr, "[CB][ERROR] Could not write file '%s' for configuring: %s\n", output,
                strerror(errno));
        exit(1);
    }

    fclose(output_file);
}

// usage: CONFIGURE_FILE("config.h.coolbuild_in", "config.h", "k1", "v1", "k2", "v2")
#define CONFIGURE_FILE(input, output, ...) configure_file_impl(input, output, __VA_ARGS__, NULL)

char* xstrdup(const char* str) {
    size_t len = strlen(str);
    char* result = malloc(len + 1);
    memcpy(result, str, len + 1);
    return result;
}

const char* curdir(void) {
    static char* curdir = NULL;
    if (curdir == NULL) {
        char* dir = xstrdup(__FILE__);
        char* last_slash = strrchr(dir, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        } else {
            dir = ".";
        }
        curdir = malloc(PATH_MAX + 1);
        if (realpath(dir, curdir) == NULL) {
            fprintf(stderr, "[CB][ERROR] Could not get current directory: %s\n", strerror(errno));
            exit(1);
        }
    }
    return curdir;
}
