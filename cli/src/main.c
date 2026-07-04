#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define COBRA_CLI_VERSION "0.2.0"

static int build_and_run(const char *cb_file, int run_it) {
    char cmd[4096];
    char base[1024], basebuf[1024];

    if (!cb_file) {
        struct stat st;
        if (stat("src/main.cb", &st) == 0)
            cb_file = "src/main.cb";
        else if (stat("main.cb", &st) == 0)
            cb_file = "main.cb";
        else {
            fprintf(stderr, "Error: no .cb file found\n");
            return 1;
        }
    }
    strncpy(base, cb_file, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot) *dot = 0;
    strncpy(basebuf, base, sizeof(basebuf) - 1);
    basebuf[sizeof(basebuf) - 1] = 0;

    char *slash = strrchr(basebuf, '/');
    char outdir[1024];
    if (slash) {
        int dirlen = slash - basebuf;
        snprintf(outdir, sizeof(outdir), "build/%.*s", dirlen, basebuf);
    } else {
        snprintf(outdir, sizeof(outdir), "build");
    }

    snprintf(cmd, sizeof(cmd), "mkdir -p %s && cobrac %s -o build/%s.s 2>/dev/null", outdir, cb_file, basebuf);

    if (system(cmd) != 0) { fprintf(stderr, "Compilation error\n"); return 1; }

    snprintf(cmd, sizeof(cmd), "clang -c build/%s.s -o build/%s.o 2>/dev/null", basebuf, basebuf);
    if (system(cmd) != 0) { fprintf(stderr, "Assembly error\n"); return 1; }

    char runtime[1024];
    snprintf(runtime, sizeof(runtime), "runtime/libcobra_runtime.a");
    snprintf(cmd, sizeof(cmd), "clang build/%s.o -o build/%s %s -lc 2>/dev/null", basebuf, basebuf, runtime);
    if (system(cmd) != 0) {
        snprintf(runtime, sizeof(runtime), "../runtime/libcobra_runtime.a");
        snprintf(cmd, sizeof(cmd), "clang build/%s.o -o build/%s %s -lc 2>/dev/null", basebuf, basebuf, runtime);
    }
    if (system(cmd) != 0) { fprintf(stderr, "Linking error\n"); return 1; }

    if (run_it) {
        snprintf(cmd, sizeof(cmd), "./build/%s", basebuf);
        return system(cmd);
    }
    printf("Build successful: build/%s\n", basebuf);
    return 0;
}

static int repl(void) {
    char line[4096];
    while (1) {
        printf(">>> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "exit()") == 0 || strcmp(line, "quit()") == 0) break;
        if (line[0] == '\0') continue;

        char tmpfile[] = "/tmp/cobra_repl_XXXXXX";
        int fd = mkstemp(tmpfile);
        if (fd < 0) { printf("Error\n"); continue; }
        FILE *f = fdopen(fd, "w");
        if (f) {
            fprintf(f, "fn main():\n");
            fprintf(f, "    print(%s)\n", line);
            fclose(f);
        }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd),
            "cobrac %s -o /tmp/cobra_repl.s 2>/dev/null && "
            "clang -c /tmp/cobra_repl.s -o /tmp/cobra_repl.o 2>/dev/null && "
            "clang /tmp/cobra_repl.o -o /tmp/cobra_repl runtime/libcobra_runtime.a -lc 2>/dev/null && "
            "/tmp/cobra_repl 2>/dev/null || echo 'Error'",
            tmpfile);
        system(cmd);
        unlink(tmpfile);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return repl();

    if (strcmp(argv[1], "--version") == 0) {
        printf("%s\n", COBRA_CLI_VERSION);
        return 0;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: cobra [file.cb]    Compile and run a Cobra file\n");
        printf("       cobra              Start interactive REPL\n");
        printf("       cobra --version    Show version\n");
        printf("       cobra --help       Show this help\n");
        return 0;
    }

    const char *ext = strrchr(argv[1], '.');
    if (ext && strcmp(ext, ".cb") == 0)
        return build_and_run(argv[1], 1);

    if (strcmp(argv[1], "build") == 0)
        return build_and_run(NULL, 0);

    fprintf(stderr, "Usage: cobra [file.cb]\n");
    return 1;
}
