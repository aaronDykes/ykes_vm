#include <stdio.h>
#include "virtual_machine.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

static void repl(void);
static void run_file(const char *path);
static char *read_file(const char *path);

int main(int argc, char **argv)
{

    if (argc == 1)
        repl();
    else if (argc == 2)
        run_file(argv[1]);
    else
    {
        fprintf(stderr, "USAGE: ykes [path]\n");
        exit(69);
    }

    return EXIT_SUCCESS;
}

static void repl(void)
{

    initVM();
    char ar[1024] = {0};
    // Arena ar = GROW_ARRAY(NULL, 1024 * sizeof(char), ARENA_STR);

    for (;;)
    {
        printf("$ ");

        if (!fgets(ar, 1024, stdin))
        {
            printf("\n");
            break;
        }
        if (strcmp(ar, "end\n") == 0)
            break;

        interpret(ar);
    }
    freeVM();
}

static int str_len(char *str)
{
    int count = 0;
    char *s = str;

    while ((*s++))
        count++;
    return count;
}

static void strip_path(char *str)
{
    int len = str_len(str) - 1;
    char *s = str + len;

    for (; *s != '/'; --s, --len)
        ;

    str[len + 1] = '\0';
}

static char *get_name(char *path)
{
    int len = strlen(path) - 1;
    char *tmp = path + len;

    int count;
    for (count = 0; tmp[-1] != '/'; --tmp, count++)
        ;

    char *file = ALLOC((count + 1) * sizeof(char));

    strcpy(file, tmp);

    return file;
}

static char *get_full_path(char *path)
{
    // char res[PATH_MAX] = {0};

    char *ptr = NULL;
    char *tmp = NULL;
    tmp = realpath(path, NULL);

    if (tmp)
    {
        ptr = ALLOC(strlen(tmp));
        strcpy(ptr, tmp);
    }
    else
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    return ptr;
}

static void run_file(const char *path)
{
    initVM();

    char *source = NULL;
    source = read_file(get_full_path((char *)path));

    char *name = NULL;
    name = get_name((char *)path);
    strip_path((char *)path);

    Interpretation result = interpret_path(source, path, name);
    free(source);

    if (result == INTERPRET_COMPILE_ERR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERR)
        exit(70);

    freeVM();
}

static char *read_file(const char *path)
{

    FILE *file = NULL;
    if (path)
        file = fopen(path, "rb");
    else if (!file)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = malloc(fileSize + 1);

    if (!buffer)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;

    /*

            struct stat s;
        int fd = open(path, O_RDONLY);
        int status = fstat(fd, &s);
        int size = s.st_size;

        void *f = (char *)mmap(0, PAGE, PROT_READ, MAP_PRIVATE, -1, 0);
        char *tmp = f;

        munmap(f, size);
        return tmp;
    */
}
