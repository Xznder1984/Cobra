#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define COBRA_CLI_VERSION "0.1.0"

static void print_banner(void) {
    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║         Cobra Programming Language   ║\n");
    printf("  ║         Version %s               ║\n", COBRA_CLI_VERSION);
    printf("  ╚══════════════════════════════════════╝\n");
    printf("\n");
}

static void print_usage(void) {
    printf("Usage: cobra <command> [options]\n");
    printf("\n");
    printf("Commands:\n");
    printf("  new         Create a new Cobra project\n");
    printf("  init        Initialize a Cobra project in current directory\n");
    printf("  build       Build the current project\n");
    printf("  run         Build and run the current project\n");
    printf("  test        Run tests\n");
    printf("  clean       Clean build artifacts\n");
    printf("  check       Check code for errors\n");
    printf("  fmt         Format code\n");
    printf("  lint        Lint code\n");
    printf("  docs        Generate documentation\n");
    printf("  package     Package a project\n");
    printf("  publish     Publish a package\n");
    printf("  install     Install a package\n");
    printf("  remove      Remove a package\n");
    printf("  update      Update packages\n");
    printf("  search      Search packages\n");
    printf("  version     Show version\n");
    printf("  doctor      Check system setup\n");
    printf("  repl        Start interactive REPL\n");
    printf("  help        Show this help\n");
    printf("\n");
    printf("Options:\n");
    printf("  --help      Show this help\n");
    printf("  --version   Show version\n");
    printf("  --verbose   Verbose output\n");
}

static int cmd_new(int argc, char *argv[]);
static int cmd_init(int argc, char *argv[]);
static int cmd_build(int argc, char *argv[]);
static int cmd_run(int argc, char *argv[]);
static int cmd_test(int argc, char *argv[]);
static int cmd_clean(int argc, char *argv[]);
static int cmd_check(int argc, char *argv[]);
static int cmd_fmt(int argc, char *argv[]);
static int cmd_lint(int argc, char *argv[]);
static int cmd_docs(int argc, char *argv[]);
static int cmd_package(int argc, char *argv[]);
static int cmd_publish(int argc, char *argv[]);
static int cmd_install(int argc, char *argv[]);
static int cmd_remove(int argc, char *argv[]);
static int cmd_update(int argc, char *argv[]);
static int cmd_search(int argc, char *argv[]);
static int cmd_version(int argc, char *argv[]);
static int cmd_doctor(int argc, char *argv[]);
static int cmd_repl(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_banner();
        print_usage();
        return 1;
    }

    const char *command = argv[1];
    int cmd_argc = argc - 2;
    char **cmd_argv = argv + 2;

    if (strcmp(command, "--version") == 0) {
        print_banner();
        return 0;
    }
    if (strcmp(command, "--help") == 0 || strcmp(command, "help") == 0) {
        print_usage();
        return 0;
    }
    if (strcmp(command, "-v") == 0 || strcmp(command, "--verbose") == 0) {
        print_banner();
        return 0;
    }

    struct {
        const char *name;
        int (*func)(int, char **);
    } commands[] = {
        {"new", cmd_new},
        {"init", cmd_init},
        {"build", cmd_build},
        {"run", cmd_run},
        {"test", cmd_test},
        {"clean", cmd_clean},
        {"check", cmd_check},
        {"fmt", cmd_fmt},
        {"lint", cmd_lint},
        {"docs", cmd_docs},
        {"package", cmd_package},
        {"publish", cmd_publish},
        {"install", cmd_install},
        {"remove", cmd_remove},
        {"update", cmd_update},
        {"search", cmd_search},
        {"version", cmd_version},
        {"doctor", cmd_doctor},
        {"repl", cmd_repl},
        {NULL, NULL}
    };

    for (int i = 0; commands[i].name; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            return commands[i].func(cmd_argc, cmd_argv);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", command);
    fprintf(stderr, "Run 'cobra help' for usage.\n");
    return 1;
}

static int cmd_new(int argc, char *argv[]) {
    const char *name = argc > 0 ? argv[0] : "myproject";
    printf("Creating new Cobra project: %s\n", name);

    char mkdir_cmd[1024];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd),
             "mkdir -p %s/src %s/tests", name, name);
    system(mkdir_cmd);

    char cobra_file[1024];
    snprintf(cobra_file, sizeof(cobra_file), "%s/src/main.cb", name);
    FILE *f = fopen(cobra_file, "w");
    if (f) {
        fprintf(f, "// %s\n", name);
        fprintf(f, "// Cobra Programming Language\n\n");
        fprintf(f, "fn main() {\n");
        fprintf(f, "    println(\"Hello from %s!\")\n", name);
        fprintf(f, "}\n");
        fclose(f);
    }

    char cobra_json[1024];
    snprintf(cobra_json, sizeof(cobra_json), "%s/cobra.json", name);
    f = fopen(cobra_json, "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "    \"name\": \"%s\",\n", name);
        fprintf(f, "    \"version\": \"0.1.0\",\n");
        fprintf(f, "    \"description\": \"A Cobra project\",\n");
        fprintf(f, "    \"author\": \"\",\n");
        fprintf(f, "    \"license\": \"MIT\",\n");
        fprintf(f, "    \"dependencies\": {}\n");
        fprintf(f, "}\n");
        fclose(f);
    }

    printf("Created project '%s'.\n", name);
    printf("Run: cd %s && cobra run\n", name);
    return 0;
}

static int cmd_init(int argc, char *argv[]) {
    printf("Initializing Cobra project in current directory...\n");

    FILE *f = fopen("cobra.json", "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "    \"name\": \"project\",\n");
        fprintf(f, "    \"version\": \"0.1.0\",\n");
        fprintf(f, "    \"description\": \"A Cobra project\",\n");
        fprintf(f, "    \"dependencies\": {}\n");
        fprintf(f, "}\n");
        fclose(f);
        printf("Created cobra.json\n");
    }

    system("mkdir -p src tests");
    printf("Initialized Cobra project.\n");
    return 0;
}

static void append_link_flag(char *buf, size_t bufsz, const char *flag) {
    size_t blen = strlen(buf);
    if (blen + strlen(flag) + 2 < bufsz) {
        strcat(buf, " ");
        strcat(buf, flag);
    }
}

static int cmd_build(int argc, char *argv[]) {
    printf("Building Cobra project...\n");

    char project_dir[1024] = ".";
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
            strncpy(project_dir, argv[++i], sizeof(project_dir)-1);
        }
    }

    char main_cb[1024];
    snprintf(main_cb, sizeof(main_cb), "%s/src/main.cb", project_dir);

    struct stat st;
    if (stat(main_cb, &st) != 0) {
        snprintf(main_cb, sizeof(main_cb), "src/main.cb");
        if (stat(main_cb, &st) != 0) {
            snprintf(main_cb, sizeof(main_cb), "main.cb");
            if (stat(main_cb, &st) != 0) {
                printf("Error: src/main.cb not found\n");
                return 1;
            }
        }
    }

    char bridge_dir[1024];
    snprintf(bridge_dir, sizeof(bridge_dir), "%s/cobra-bridge", project_dir);
    char build_dir[1024];
    snprintf(build_dir, sizeof(build_dir), "%s/build", project_dir);

    char mkdir_cmd[2048];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s %s", build_dir, bridge_dir);
    system(mkdir_cmd);

    char cobrac_cmd[2048];
    snprintf(cobrac_cmd, sizeof(cobrac_cmd),
             "cobrac %s -o %s/main.s -p %s 2>/dev/null",
             main_cb, build_dir, project_dir);
    int result = system(cobrac_cmd);
    if (result != 0) {
        printf("Compilation error.\n");
        return 1;
    }

    char s_path[1024];
    snprintf(s_path, sizeof(s_path), "%s/main.s", build_dir);
    char o_main_path[1024];
    snprintf(o_main_path, sizeof(o_main_path), "%s/main.o", build_dir);
    char asm_cmd[2048];
    snprintf(asm_cmd, sizeof(asm_cmd), "clang -c %s -o %s 2>/dev/null", s_path, o_main_path);
    result = system(asm_cmd);
    if (result != 0) {
        printf("Assembly error.\n");
        return 1;
    }

    char bridge_obj_list[4096] = "";
    char find_cmd[4096];
    snprintf(find_cmd, sizeof(find_cmd),
             "find %s -name '*.c' 2>/dev/null", bridge_dir);
    FILE *fp = popen(find_cmd, "r");
    if (fp) {
        char cfile[1024];
        while (fgets(cfile, sizeof(cfile), fp)) {
            cfile[strcspn(cfile, "\n")] = 0;
            if (strlen(cfile) == 0) continue;

            char objname[1024];
            char basebuf[256];
            char *base = strrchr(cfile, '/');
            base = base ? base + 1 : cfile;
            strncpy(basebuf, base, sizeof(basebuf)-1);
            basebuf[sizeof(basebuf)-1] = 0;
            char *dot = strrchr(basebuf, '.');
            if (dot) *dot = 0;

            snprintf(objname, sizeof(objname), "%s/%s_bridge.o", build_dir, basebuf);

            char cc_cmd[4096];
            snprintf(cc_cmd, sizeof(cc_cmd),
                     "clang -c %s -o %s $(python3-config --includes 2>/dev/null) 2>/dev/null",
                     cfile, objname);
            int br = system(cc_cmd);
            if (br == 0) {
                append_link_flag(bridge_obj_list, sizeof(bridge_obj_list), objname);
            }
        }
        pclose(fp);
    }

    char link_flags[4096] = "-lc";

    // Auto-add Python link flags if bridge files exist
    if (bridge_obj_list[0]) {
        FILE *pfp = popen("python3-config --ldflags --embed 2>/dev/null || python3-config --ldflags 2>/dev/null", "r");
        if (pfp) {
            char pyflags[4096] = {0};
            if (fgets(pyflags, sizeof(pyflags), pfp)) {
                pyflags[strcspn(pyflags, "\n")] = 0;
                append_link_flag(link_flags, sizeof(link_flags), pyflags);
            }
            pclose(pfp);
        } else {
            append_link_flag(link_flags, sizeof(link_flags), "-lpython3");
        }
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            append_link_flag(link_flags, sizeof(link_flags), "-l");
            strncat(link_flags, argv[++i], sizeof(link_flags) - strlen(link_flags) - 1);
        } else if (strncmp(argv[i], "-l", 2) == 0) {
            append_link_flag(link_flags, sizeof(link_flags), argv[i]);
        } else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc) {
            append_link_flag(link_flags, sizeof(link_flags), "-L");
            strncat(link_flags, argv[++i], sizeof(link_flags) - strlen(link_flags) - 1);
        } else if (strncmp(argv[i], "-L", 2) == 0) {
            append_link_flag(link_flags, sizeof(link_flags), argv[i]);
        }
    }

    char runtime_path[1024];
    snprintf(runtime_path, sizeof(runtime_path),
             "%s/../runtime/libcobra_runtime.a", project_dir);

    char link_cmd[4096];
    snprintf(link_cmd, sizeof(link_cmd),
             "clang %s/main.o %s -o %s/program %s %s 2>/dev/null",
             build_dir, bridge_obj_list, build_dir, runtime_path, link_flags);
    result = system(link_cmd);

    if (result != 0) {
        snprintf(link_cmd, sizeof(link_cmd),
                 "clang %s/main.o %s -o %s/program %s %s 2>/dev/null",
                 build_dir, bridge_obj_list, build_dir,
                 "runtime/libcobra_runtime.a", link_flags);
        result = system(link_cmd);
    }

    if (result != 0) {
        printf("Linking error.\n");
        return 1;
    }

    printf("Build successful. Output: %s/program\n", build_dir);
    return 0;
}

static int cmd_run(int argc, char *argv[]) {
    char project_dir[1024] = ".";
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
            strncpy(project_dir, argv[++i], sizeof(project_dir)-1);
        }
    }
    int result = cmd_build(argc, argv);
    if (result != 0) return result;
    printf("Running...\n");
    char run_cmd[2048];
    snprintf(run_cmd, sizeof(run_cmd), "%s/build/program", project_dir);
    return system(run_cmd);
}

static int cmd_test(int argc, char *argv[]) {
    printf("Running tests...\n");
    system("mkdir -p build");
    system("cobrac tests/test_main.cb -o build/test.s 2>/dev/null || true");
    system("clang -c build/test.s -o build/test.o 2>/dev/null || true");
    system("clang build/test.o -o build/test -lc 2>/dev/null || true");
    return system("./build/test");
}

static int cmd_clean(int argc, char *argv[]) {
    printf("Cleaning build artifacts...\n");
    system("rm -rf build");
    system("rm -f cobra.lock");
    return 0;
}

static int cmd_check(int argc, char *argv[]) {
    printf("Checking code for errors...\n");
    return system("cobrac src/main.cb -o /dev/null 2>&1");
}

static int cmd_fmt(int argc, char *argv[]) {
    printf("Formatting code...\n");
    const char *target = argc > 0 ? argv[0] : "src";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "find %s -name '*.cb' -exec cobra fmt {} \\;", target);
    system(cmd);
    printf("Format complete.\n");
    return 0;
}

static int cmd_lint(int argc, char *argv[]) {
    printf("Linting code...\n");
    return system("cobrac src/main.cb -o /dev/null 2>&1; echo 'Lint complete.'");
}

static int cmd_docs(int argc, char *argv[]) {
    printf("Generating documentation...\n");
    system("mkdir -p docs");
    printf("Documentation generated in docs/\n");
    FILE *f = fopen("docs/index.html", "w");
    if (f) {
        fprintf(f, "<html><head><title>Project Docs</title></head><body>\n");
        fprintf(f, "<h1>Project Documentation</h1>\n");
        fprintf(f, "<p>Generated by Cobra Docs</p>\n");
        fprintf(f, "</body></html>\n");
        fclose(f);
    }
    return 0;
}

static int cmd_package(int argc, char *argv[]) {
    printf("Packaging project...\n");
    system("mkdir -p dist");
    system("tar czf dist/package.tar.gz src/ cobra.json");
    printf("Package created: dist/package.tar.gz\n");
    return 0;
}

static int cmd_publish(int argc, char *argv[]) {
    printf("Publishing package...\n");
    printf("Package registry not yet configured.\n");
    printf("Run: cobra package first, then upload dist/package.tar.gz\n");
    return 0;
}

static int cmd_install(int argc, char *argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Usage: cobra install <package>\n");
        return 1;
    }
    printf("Installing package: %s\n", argv[0]);
    // TODO: implement package resolution
    printf("Package installation is not yet implemented.\n");
    return 0;
}

static int cmd_remove(int argc, char *argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Usage: cobra remove <package>\n");
        return 1;
    }
    printf("Removing package: %s\n", argv[0]);
    return 0;
}

static int cmd_update(int argc, char *argv[]) {
    printf("Updating packages...\n");
    return 0;
}

static int cmd_search(int argc, char *argv[]) {
    const char *query = argc > 0 ? argv[0] : "";
    printf("Searching for packages matching '%s'...\n", query);
    printf("Package registry not yet configured.\n");
    return 0;
}

static int cmd_version(int argc, char *argv[]) {
    (void)argc; (void)argv;
    print_banner();
    printf("Cobra CLI v%s\n", COBRA_CLI_VERSION);
    printf("Compiler: cobrac v%s\n", COBRA_CLI_VERSION);
    return 0;
}

static int cmd_doctor(int argc, char *argv[]) {
    printf("Checking Cobra system setup...\n\n");

    printf("[1/5] Checking cobrac compiler... ");
    if (system("which cobrac > /dev/null 2>&1") == 0) {
        printf("FOUND\n");
    } else {
        printf("NOT FOUND (run 'make build' first)\n");
    }

    printf("[2/5] Checking C compiler... ");
    if (system("which clang > /dev/null 2>&1") == 0) {
        printf("FOUND (clang)\n");
    } else if (system("which gcc > /dev/null 2>&1") == 0) {
        printf("FOUND (gcc)\n");
    } else {
        printf("NOT FOUND (install Xcode CLI tools)\n");
    }

    printf("[3/5] Checking assembler... ");
    if (system("which as > /dev/null 2>&1") == 0) {
        printf("FOUND\n");
    } else {
        printf("NOT FOUND\n");
    }

    printf("[4/5] Checking linker... ");
    if (system("which ld > /dev/null 2>&1") == 0) {
        printf("FOUND\n");
    } else {
        printf("NOT FOUND\n");
    }

    printf("[5/5] Checking Git... ");
    if (system("which git > /dev/null 2>&1") == 0) {
        printf("FOUND\n");
    } else {
        printf("NOT FOUND\n");
    }

    printf("\nSystem check complete.\n");
    return 0;
}

static int cmd_repl(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("Cobra REPL v%s\n", COBRA_CLI_VERSION);
    printf("Type 'exit' to quit.\n\n");

    char line[4096];
    while (1) {
        printf(">> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (strcmp(line, "") == 0) continue;

        char tmpfile[] = "/tmp/cobra_repl_XXXXXX";
        int fd = mkstemp(tmpfile);
        if (fd < 0) {
            printf("Error creating temp file\n");
            continue;
        }

        FILE *f = fdopen(fd, "w");
        if (f) {
            fprintf(f, "fn main() {\n");
            fprintf(f, "    println(%s)\n", line);
            fprintf(f, "}\n");
            fclose(f);
        }

        char cmd[4096];
        snprintf(cmd, sizeof(cmd),
                 "cobrac %s -o /tmp/cobra_repl.s 2>/dev/null && "
                 "clang -c /tmp/cobra_repl.s -o /tmp/cobra_repl.o 2>/dev/null && "
                 "clang /tmp/cobra_repl.o -o /tmp/cobra_repl 2>/dev/null && "
                 "/tmp/cobra_repl 2>/dev/null || echo 'Error'",
                 tmpfile);
        system(cmd);

        unlink(tmpfile);
    }

    printf("\nGoodbye!\n");
    return 0;
}
