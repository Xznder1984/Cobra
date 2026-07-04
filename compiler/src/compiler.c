#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "typechecker.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

Compiler *compiler_create(void) {
    Compiler *c = calloc(1, sizeof(Compiler));
    c->source_path = NULL;
    c->output_path = NULL;
    c->source = NULL;
    c->diags = diag_list_create();
    c->optimize_level = 0;
    c->emit_ir = 0;
    c->verbose = 0;
    c->use_python = 0;
    c->use_cargo = 0;
    c->project_dir = NULL;
    c->bridge_dir = NULL;
    c->link_flags = NULL;
    c->link_flag_count = 0;
    c->link_flag_capacity = 0;
    c->python_pkgs = NULL;
    c->python_pkg_count = 0;
    c->cargo_crates = NULL;
    c->cargo_crate_count = 0;
    c->bridge_c_srcs = NULL;
    c->bridge_c_src_count = 0;
    c->bridge_cb_srcs = NULL;
    c->bridge_cb_src_count = 0;
    return c;
}

void compiler_free(Compiler *c) {
    if (c) {
        free(c->source_path);
        free(c->output_path);
        free(c->source);
        free(c->project_dir);
        free(c->bridge_dir);
        for (int i = 0; i < c->link_flag_count; i++) free(c->link_flags[i]);
        free(c->link_flags);
        for (int i = 0; i < c->python_pkg_count; i++) free(c->python_pkgs[i]);
        free(c->python_pkgs);
        for (int i = 0; i < c->cargo_crate_count; i++) free(c->cargo_crates[i]);
        free(c->cargo_crates);
        for (int i = 0; i < c->bridge_c_src_count; i++) free(c->bridge_c_srcs[i]);
        free(c->bridge_c_srcs);
        for (int i = 0; i < c->bridge_cb_src_count; i++) free(c->bridge_cb_srcs[i]);
        free(c->bridge_cb_srcs);
        diag_list_free(c->diags);
        free(c);
    }
}

static char *change_extension(const char *path, const char *new_ext) {
    const char *dot = strrchr(path, '.');
    size_t base_len = dot ? (size_t)(dot - path) : strlen(path);
    size_t ext_len = strlen(new_ext);
    char *result = malloc(base_len + ext_len + 1);
    strncpy(result, path, base_len);
    result[base_len] = '\0';
    strcat(result, new_ext);
    return result;
}

static char *get_dir_path(const char *path) {
    char *dir = strdup(path);
    char *slash = strrchr(dir, '/');
    if (slash) *slash = '\0';
    else { free(dir); dir = strdup("."); }
    return dir;
}

static void add_bridge_c_src(Compiler *c, const char *path) {
    c->bridge_c_srcs = realloc(c->bridge_c_srcs, sizeof(char*) * (c->bridge_c_src_count + 1));
    c->bridge_c_srcs[c->bridge_c_src_count++] = strdup(path);
}

static void add_bridge_cb_src(Compiler *c, const char *path) {
    c->bridge_cb_srcs = realloc(c->bridge_cb_srcs, sizeof(char*) * (c->bridge_cb_src_count + 1));
    c->bridge_cb_srcs[c->bridge_cb_src_count++] = strdup(path);
}

static char *read_file_into_str(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    char *buf = malloc((size_t)(len + 1));
    if (buf) {
        size_t n = fread(buf, 1, (size_t)len, fp);
        buf[n] = 0;
    }
    fclose(fp);
    return buf;
}

// ── Python Bridge Generation ──────────────────────────────────────────

static void generate_python_bridge(Compiler *c, const char *pkg) {
    char bridge_c[4096];
    snprintf(bridge_c, sizeof(bridge_c), "%s/python_%s_bridge.c", c->bridge_dir, pkg);

    FILE *fp = fopen(bridge_c, "w");
    if (!fp) {
        if (c->verbose) printf("  Warning: could not create Python bridge %s\n", bridge_c);
        return;
    }

    fprintf(fp, "#include <Python.h>\n\n");
    fprintf(fp, "static int _cobra_py_init_%s_done = 0;\n\n", pkg);

    // Init function
    fprintf(fp, "void _cobra_py_init_%s(void) {\n", pkg);
    fprintf(fp, "    if (!_cobra_py_init_%s_done) {\n", pkg);
    fprintf(fp, "        if (!Py_IsInitialized()) Py_Initialize();\n");
    fprintf(fp, "        PyRun_SimpleString(\"import %s\\n\");\n", pkg);
    fprintf(fp, "        _cobra_py_init_%s_done = 1;\n", pkg);
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n\n");

    // Generic Python call: module.func(arg1, arg2, ...) as JSON string → result as string
    fprintf(fp, "const char *_cobra_py_call_%s("
                "const char *funcname, const char *args_json) {\n", pkg);
    fprintf(fp, "    _cobra_py_init_%s();\n", pkg);
    fprintf(fp, "    static char result_buf[65536];\n");
    fprintf(fp, "    result_buf[0] = 0;\n");

    // Split "module.func" into module and func
    fprintf(fp, "    const char *dot = strchr(funcname, '.');\n");
    fprintf(fp, "    if (!dot) return \"[error: need module.func format]\";\n");
    fprintf(fp, "    size_t modlen = (size_t)(dot - funcname);\n");
    fprintf(fp, "    char modname[256];\n");
    fprintf(fp, "    snprintf(modname, sizeof(modname), \"%%.*s\", (int)modlen, funcname);\n");
    fprintf(fp, "    const char *fname = dot + 1;\n\n");

    fprintf(fp, "    PyObject *pModule = PyImport_ImportModule(modname);\n");
    fprintf(fp, "    if (!pModule) { PyErr_Print(); return \"[error importing module]\"; }\n");
    fprintf(fp, "    PyObject *pFunc = PyObject_GetAttrString(pModule, fname);\n");
    fprintf(fp, "    if (!pFunc || !PyCallable_Check(pFunc)) {\n");
    fprintf(fp, "        PyErr_Print(); Py_XDECREF(pFunc); Py_DECREF(pModule);\n");
    fprintf(fp, "        return \"[error getting function]\";\n");
    fprintf(fp, "    }\n\n");

    // Parse JSON args and call
    fprintf(fp, "    PyObject *pArgs = PyTuple_New(0);\n");
    fprintf(fp, "    PyObject *pResult = PyObject_CallObject(pFunc, pArgs);\n");
    fprintf(fp, "    Py_DECREF(pArgs);\n\n");

    fprintf(fp, "    if (!pResult) {\n");
    fprintf(fp, "        PyErr_Print();\n");
    fprintf(fp, "        Py_DECREF(pFunc); Py_DECREF(pModule);\n");
    fprintf(fp, "        return \"[error calling function]\";\n");
    fprintf(fp, "    }\n\n");

    // Convert result to string
    fprintf(fp, "    PyObject *pStr = PyObject_Str(pResult);\n");
    fprintf(fp, "    if (pStr) {\n");
    fprintf(fp, "        const char *s = PyUnicode_AsUTF8(pStr);\n");
    fprintf(fp, "        if (s) strncpy(result_buf, s, sizeof(result_buf)-1);\n");
    fprintf(fp, "        Py_DECREF(pStr);\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    Py_DECREF(pResult);\n");
    fprintf(fp, "    Py_DECREF(pFunc); Py_DECREF(pModule);\n");
    fprintf(fp, "    return result_buf;\n");
    fprintf(fp, "}\n");

    // Integer result version
    fprintf(fp, "long long _cobra_py_call_int_%s("
                "const char *funcname, const char *args_json) {\n", pkg);
    fprintf(fp, "    _cobra_py_init_%s();\n", pkg);
    fprintf(fp, "    const char *dot = strchr(funcname, '.');\n");
    fprintf(fp, "    if (!dot) return 0;\n");
    fprintf(fp, "    char modname[256];\n");
    fprintf(fp, "    snprintf(modname, sizeof(modname), \"%%.*s\", (int)(dot-funcname), funcname);\n");
    fprintf(fp, "    const char *fname = dot + 1;\n");
    fprintf(fp, "    PyObject *pModule = PyImport_ImportModule(modname);\n");
    fprintf(fp, "    if (!pModule) { PyErr_Print(); return 0; }\n");
    fprintf(fp, "    PyObject *pFunc = PyObject_GetAttrString(pModule, fname);\n");
    fprintf(fp, "    if (!pFunc || !PyCallable_Check(pFunc)) {\n");
    fprintf(fp, "        Py_XDECREF(pFunc); Py_DECREF(pModule); return 0;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    PyObject *pArgs = PyTuple_New(0);\n");
    fprintf(fp, "    PyObject *pResult = PyObject_CallObject(pFunc, pArgs);\n");
    fprintf(fp, "    Py_DECREF(pArgs);\n");
    fprintf(fp, "    long long result = 0;\n");
    fprintf(fp, "    if (pResult) { result = PyLong_AsLongLong(pResult); Py_DECREF(pResult); }\n");
    fprintf(fp, "    Py_DECREF(pFunc); Py_DECREF(pModule);\n");
    fprintf(fp, "    return result;\n");
    fprintf(fp, "}\n\n");

    // Double (float) result version
    fprintf(fp, "double _cobra_py_call_float_%s("
                "const char *funcname, const char *args_json) {\n", pkg);
    fprintf(fp, "    _cobra_py_init_%s();\n", pkg);
    fprintf(fp, "    const char *dot = strchr(funcname, '.');\n");
    fprintf(fp, "    if (!dot) return 0.0;\n");
    fprintf(fp, "    char modname[256];\n");
    fprintf(fp, "    snprintf(modname, sizeof(modname), \"%%.*s\", (int)(dot-funcname), funcname);\n");
    fprintf(fp, "    const char *fname = dot + 1;\n");
    fprintf(fp, "    PyObject *pModule = PyImport_ImportModule(modname);\n");
    fprintf(fp, "    if (!pModule) { PyErr_Print(); return 0.0; }\n");
    fprintf(fp, "    PyObject *pFunc = PyObject_GetAttrString(pModule, fname);\n");
    fprintf(fp, "    if (!pFunc || !PyCallable_Check(pFunc)) {\n");
    fprintf(fp, "        Py_XDECREF(pFunc); Py_DECREF(pModule); return 0.0;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    PyObject *pArgs = PyTuple_New(0);\n");
    fprintf(fp, "    PyObject *pResult = PyObject_CallObject(pFunc, pArgs);\n");
    fprintf(fp, "    Py_DECREF(pArgs);\n");
    fprintf(fp, "    double result = 0.0;\n");
    fprintf(fp, "    if (pResult) { result = PyFloat_AsDouble(pResult); Py_DECREF(pResult); }\n");
    fprintf(fp, "    Py_DECREF(pFunc); Py_DECREF(pModule);\n");
    fprintf(fp, "    return result;\n");
    fprintf(fp, "}\n");

    fclose(fp);
    if (c->verbose) printf("  Generated Python bridge: %s\n", bridge_c);
    add_bridge_c_src(c, bridge_c);
}

static void generate_python_cb_decls(Compiler *c, const char *pkg) {
    char bridge_cb[4096];
    snprintf(bridge_cb, sizeof(bridge_cb), "%s/python_%s_bridge.cb", c->bridge_dir, pkg);

    FILE *fp = fopen(bridge_cb, "w");
    if (!fp) {
        if (c->verbose) printf("  Warning: could not create Python CB decls %s\n", bridge_cb);
        return;
    }

    // Only emit extern fn declarations (no fn helpers to avoid parse issues)
    // Params omitted — codegen passes call args positionally via registers
    fprintf(fp, "// Auto-generated Cobra bindings for Python package: %s\n", pkg);
    fprintf(fp, "// Call _cobra_py_init_%s() before using Python functions\n\n", pkg);
    fprintf(fp, "extern fn _cobra_py_init_%s()\n", pkg);
    fprintf(fp, "extern fn _cobra_py_call_%s()\n", pkg);
    fprintf(fp, "extern fn _cobra_py_call_int_%s()\n", pkg);
    fprintf(fp, "extern fn _cobra_py_call_float_%s()\n", pkg);

    fclose(fp);
    if (c->verbose) printf("  Generated Python CB declarations: %s\n", bridge_cb);
    add_bridge_cb_src(c, bridge_cb);
}

// ── Rust Bridge Generation ────────────────────────────────────────────

static void generate_rust_bridge(Compiler *c, const char *pkg, const char *bridge_name) {
    const char *cargo_dir = c->project_dir ? c->project_dir : ".";
    char toml_path[4096];
    snprintf(toml_path, sizeof(toml_path), "%s/Cargo.toml", cargo_dir);

    // Create / update Cargo.toml
    FILE *fp = fopen(toml_path, "r");
    if (!fp) {
        FILE *f = fopen(toml_path, "w");
        if (f) {
            fprintf(f, "[package]\n");
            fprintf(f, "name = \"%s\"\n", bridge_name);
            fprintf(f, "version = \"0.1.0\"\n");
            fprintf(f, "edition = \"2021\"\n\n");
            fprintf(f, "[lib]\n");
            fprintf(f, "crate-type = [\"cdylib\"]\n\n");
            fprintf(f, "[dependencies]\n");
            fprintf(f, "%s = \"*\"\n", pkg);
            fprintf(f, "\n[build-dependencies]\n");
            fprintf(f, "cbindgen = \"*\"\n");
            fclose(f);
        }
    } else {
        fclose(fp);
    }

    // Ensure src/lib.rs exists
    char src_dir[4096];
    snprintf(src_dir, sizeof(src_dir), "%s/src", cargo_dir);
    struct stat st;
    if (stat(src_dir, &st) != 0) mkdir(src_dir, 0755);

    char lib_rs[4096];
    snprintf(lib_rs, sizeof(lib_rs), "%s/src/lib.rs", cargo_dir);
    fp = fopen(lib_rs, "r");
    if (!fp) {
        FILE *f = fopen(lib_rs, "w");
        if (f) {
            fprintf(f, "// Auto-generated Cobra-Rust bridge for `%s`\n", pkg);
            fprintf(f, "// Add #[no_mangle] pub extern \"C\" fn wrappers here.\n\n");
            fprintf(f, "// Example:\n");
            fprintf(f, "// #[no_mangle]\n");
            fprintf(f, "// pub extern \"C\" fn %s_version() -> *const u8 {\n", bridge_name);
            fprintf(f, "//     b\"%s v0.1.0\\0\" as *const u8\n", bridge_name);
            fprintf(f, "// }\n");
            fclose(f);
        }
    } else {
        fclose(fp);
    }

    // Create build.rs for cbindgen
    char build_rs[4096];
    snprintf(build_rs, sizeof(build_rs), "%s/build.rs", cargo_dir);
    fp = fopen(build_rs, "r");
    if (!fp) {
        FILE *f = fopen(build_rs, "w");
        if (f) {
            fprintf(f, "extern crate cbindgen;\n");
            fprintf(f, "use std::env;\n\n");
            fprintf(f, "fn main() {\n");
            fprintf(f, "    let crate_dir = env::var(\"CARGO_MANIFEST_DIR\").unwrap();\n");
            fprintf(f, "    let config = cbindgen::Config {\n");
            fprintf(f, "        language: cbindgen::Language::C,\n");
            fprintf(f, "        ..Default::default()\n");
            fprintf(f, "    };\n");
            fprintf(f, "    cbindgen::Builder::new()\n");
            fprintf(f, "        .with_crate(crate_dir)\n");
            fprintf(f, "        .with_config(config)\n");
            fprintf(f, "        .generate()\n");
            fprintf(f, "        .expect(\"Unable to generate bindings\")\n");
            fprintf(f, "        .write_to_file(\"target/bindings.h\");\n");
            fprintf(f, "}\n");
            fclose(f);
        }
    } else {
        fclose(fp);
    }

    if (c->verbose) printf("  Rust bridge scaffolding ready for '%s'\n", pkg);

    // Try cbindgen — run cargo build, then parse bindings.h if it appears
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "cd %s && cargo build --release 2>/dev/null", cargo_dir);
    int r = system(cmd);
    if (r != 0 && c->verbose) {
        printf("  Warning: cargo build for '%s' failed (exit %d)\n", pkg, r);
    }

    // Parse generated bindings.h
    char bindings_h[4096];
    snprintf(bindings_h, sizeof(bindings_h), "%s/target/bindings.h", cargo_dir);
    char *h_content = read_file_into_str(bindings_h);
    if (h_content) {
        if (c->verbose) printf("  Parsing cbindgen output: %s\n", bindings_h);
        char cb_bindings[4096];
        snprintf(cb_bindings, sizeof(cb_bindings), "%s/cargo_%s_bridge.cb", c->bridge_dir, pkg);
        FILE *f = fopen(cb_bindings, "w");
        if (f) {
            fprintf(f, "// Auto-generated Cobra bindings for Rust crate: %s\n", pkg);
            fprintf(f, "// Generated via cbindgen from bridge crate: %s\n\n", bridge_name);
            // Parse function declarations from C header
            char *line = h_content;
            char *next;
            while (line && *line) {
                next = strchr(line, '\n');
                if (next) *next = 0;
                // Look for function declarations
                if (strstr(line, "(*") || strstr(line, "typedef")) {
                    // skip function pointers and typedefs
                } else if (strstr(line, "(") && strstr(line, ")")) {
                    // Check for return type
                    if (strstr(line, "extern") || (line[0] != '#' && line[0] != '/' && line[0] != '{' && line[0] != '}')) {
                        // Simple parse: "return_type func_name(params) -> Cobra extern fn
                        fprintf(f, "// %s\n", line);
                    }
                }
                line = next ? next + 1 : NULL;
            }
            fclose(f);
            add_bridge_cb_src(c, cb_bindings);
        }
        free(h_content);
    }
}

// ── Package Resolution ────────────────────────────────────────────────

static void resolve_python_pkg(Compiler *c, const char *pkg) {
    if (c->verbose) printf("  Resolving: use python %s\n", pkg);

    // Check if package is already importable (stdlib or installed)
    char check_cmd[4096];
    snprintf(check_cmd, sizeof(check_cmd),
             "python3 -c \"import %s\" 2>/dev/null", pkg);
    int already = system(check_cmd);

    if (already != 0) {
        char cmd[4096];
        // Only try pip once, with timeout
#if defined(__APPLE__) || defined(__linux__)
        snprintf(cmd, sizeof(cmd),
                 "pip install %s --user -q 2>/dev/null & "
                 "PID=$!; sleep 10; kill $PID 2>/dev/null; wait $PID 2>/dev/null || true",
                 pkg);
#else
        snprintf(cmd, sizeof(cmd),
                 "pip install %s --user -q 2>/dev/null || true", pkg);
#endif
        int r = system(cmd);
        if (r != 0 && c->verbose) {
            printf("  Warning: pip install %s failed\n", pkg);
        }
    } else {
        if (c->verbose) printf("  Package '%s' already available\n", pkg);
    }

    c->python_pkgs = realloc(c->python_pkgs, sizeof(char*) * (c->python_pkg_count + 1));
    c->python_pkgs[c->python_pkg_count++] = strdup(pkg);
    c->use_python = 2;

    // Generate bridge
    if (c->bridge_dir) {
        mkdir(c->bridge_dir, 0755);
        generate_python_bridge(c, pkg);
        generate_python_cb_decls(c, pkg);
    }
}

static void resolve_cargo_crate(Compiler *c, const char *pkg) {
    if (c->verbose) printf("  Resolving: use cargo %s\n", pkg);
    const char *cargo_dir = c->project_dir ? c->project_dir : ".";
    char bridge_name[512];
    snprintf(bridge_name, sizeof(bridge_name), "cobra_bridge_%s", pkg);

    // Generate bridge scaffolding
    if (c->bridge_dir) {
        mkdir(c->bridge_dir, 0755);
        generate_rust_bridge(c, pkg, bridge_name);
    } else {
        // Basic scaffold without bridge dir
        char toml_path[4096];
        snprintf(toml_path, sizeof(toml_path), "%s/Cargo.toml", cargo_dir);
        FILE *fp = fopen(toml_path, "r");
        if (!fp) {
            FILE *f = fopen(toml_path, "w");
            if (f) {
                fprintf(f, "[package]\n");
                fprintf(f, "name = \"%s\"\n", bridge_name);
                fprintf(f, "version = \"0.1.0\"\n");
                fprintf(f, "edition = \"2021\"\n\n");
                fprintf(f, "[lib]\n");
                fprintf(f, "crate-type = [\"cdylib\"]\n\n");
                fprintf(f, "[dependencies]\n");
                fprintf(f, "%s = \"*\"\n", pkg);
                fclose(f);
            }
        } else {
            fclose(fp);
        }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "cd %s && cargo build --release 2>/dev/null", cargo_dir);
        system(cmd);
    }

    c->cargo_crates = realloc(c->cargo_crates, sizeof(char*) * (c->cargo_crate_count + 1));
    c->cargo_crates[c->cargo_crate_count++] = strdup(bridge_name);
    c->use_cargo = 2;
}

// ── Main Compile ──────────────────────────────────────────────────────

int compiler_compile_file(Compiler *c, const char *source_path, const char *output_path) {
    c->source_path = str_dup(source_path);
    c->source = read_file(source_path);

    if (!c->source) {
        diag_add(c->diags, DIAG_ERROR, 0, 0,
                 "Could not read source file '%s'", source_path);
        return 0;
    }

    if (!output_path) {
        c->output_path = change_extension(source_path, ".s");
    } else {
        c->output_path = str_dup(output_path);
    }

    if (!c->project_dir) {
        c->project_dir = get_dir_path(source_path);
    }

    if (!c->bridge_dir) {
        char buf[4096];
        snprintf(buf, sizeof(buf), "%s/cobra-bridge", c->project_dir);
        c->bridge_dir = strdup(buf);
    }

    if (c->verbose) {
        printf("Compiling: %s\n", source_path);
        printf("Output: %s\n", c->output_path);
        printf("Bridge dir: %s\n", c->bridge_dir);
    }

    return compiler_compile(c);
}

int compiler_compile(Compiler *c) {
    if (!c->source) {
        diag_add(c->diags, DIAG_ERROR, 0, 0, "No source to compile");
        return 0;
    }

    // Phase 1: Pre-parse to scan for use directives
    if (c->verbose) printf("Phase 1/5: Scanning for use directives...\n");
    Lexer *pre_lexer = lexer_create(c->source, c->diags);
    Parser *pre_parser = parser_create(pre_lexer, c->diags);
    Node *pre_ast = parser_parse(pre_parser);
    if (pre_ast && pre_ast->type == NODE_MODULE) {
        for (int i = 0; i < pre_ast->data.module.statements.count; i++) {
            Node *stmt = pre_ast->data.module.statements.items[i];
            if (stmt && stmt->type == NODE_USE_DECL) {
                const char *name = stmt->data.use_decl.name;
                const char *pkg = stmt->data.use_decl.package;
                if (name) {
                    if (strcmp(name, "python") == 0) {
                        if (pkg) {
                            resolve_python_pkg(c, pkg);
                        } else {
                            c->use_python = 1;
                            if (c->verbose) printf("  Detected: use python\n");
                        }
                    } else if (strcmp(name, "cargo") == 0) {
                        if (pkg) {
                            resolve_cargo_crate(c, pkg);
                        } else {
                            c->use_cargo = 1;
                            if (c->verbose) printf("  Detected: use cargo\n");
                        }
                    }
                }
            }
        }
    }
    node_free(pre_ast);
    parser_free(pre_parser);
    lexer_free(pre_lexer);

    // Clear pre-parse diagnostics — will be regenerated in the real parse
    c->diags->count = 0;
    c->diags->printed_up_to = 0;

    // Build full source: bridge CB declarations + user source
    char *full_source = NULL;
    size_t full_len = 0;

    if (c->bridge_cb_src_count > 0) {
        for (int i = 0; i < c->bridge_cb_src_count; i++) {
            char *decl = read_file_into_str(c->bridge_cb_srcs[i]);
            if (decl) {
                size_t dlen = strlen(decl);
                full_source = realloc(full_source, full_len + dlen + 1);
                memcpy(full_source + full_len, decl, dlen);
                full_len += dlen;
                full_source[full_len] = 0;
                free(decl);
            }
        }
    }

    // Append user source
    size_t slen = strlen(c->source);
    full_source = realloc(full_source, full_len + slen + 1);
    memcpy(full_source + full_len, c->source, slen);
    full_len += slen;
    full_source[full_len] = 0;

    if (c->verbose) {
        printf("  Full source length: %zu bytes (bridge: %zu, user: %zu)\n",
               full_len, full_len - slen, slen);
        printf("  Full source preview: %.300s\n", full_source);
    }

    char *saved_source = c->source;
    c->source = full_source;

    if (c->verbose) printf("Phase 2/5: Lexing...\n");
    Lexer *lexer = lexer_create(c->source, c->diags);
    if (c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    if (c->verbose) printf("Phase 3/5: Parsing...\n");
    Parser *parser = parser_create(lexer, c->diags);
    Node *ast = parser_parse(parser);
    if (parser->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    if (c->verbose) printf("Phase 4/5: Semantic Analysis...\n");
    SemanticAnalyzer *semantic = semantic_create(c->diags);
    semantic_analyze(semantic, ast);
    if (semantic->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    if (c->verbose) printf("Phase 5/5: Type Checking...\n");
    TypeChecker *tc = tc_create(c->diags);
    tc_check(tc, ast);
    if (tc->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    tc_free(tc);

    if (c->verbose) printf("Code Generation...\n");
    CodeGenerator *cg = cg_create(c->diags);
    int result = cg_generate(cg, ast, c->output_path);
    if (!result || cg->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    int has_errors = 0;
    for (int i = 0; i < c->diags->count; i++) {
        if (c->diags->items[i].level == DIAG_ERROR) { has_errors = 1; break; }
    }
    if (has_errors) result = 0;

    cg_free(cg);
    semantic_free(semantic);
    node_free(ast);
    parser_free(parser);
    lexer_free(lexer);

    c->source = saved_source;
    free(full_source);

    if (c->verbose && result) {
        printf("Compilation successful.\n");
    }

    return result;
}

// ── Link Flags ────────────────────────────────────────────────────────

const char **compiler_get_link_flags(Compiler *c, int *count) {
    if (c->link_flag_count > 0) {
        *count = c->link_flag_count;
        return (const char **)c->link_flags;
    }

    if (c->use_python) {
        FILE *fp = popen("python3-config --ldflags --embed 2>/dev/null || python3-config --ldflags 2>/dev/null", "r");
        if (fp) {
            char buf[4096] = {0};
            if (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\n")] = 0;
                char *token = strtok(buf, " \t");
                while (token) {
                    if (token[0] == '-' && (token[1] == 'l' || token[1] == 'L')) {
                        if (c->link_flag_count >= c->link_flag_capacity) {
                            c->link_flag_capacity = c->link_flag_capacity ? c->link_flag_capacity * 2 : 8;
                            c->link_flags = realloc(c->link_flags, sizeof(char*) * c->link_flag_capacity);
                        }
                        c->link_flags[c->link_flag_count++] = strdup(token);
                    }
                    token = strtok(NULL, " \t");
                }
            }
            pclose(fp);
        } else {
            if (c->verbose) printf("Warning: python3-config not found\n");
            if (c->link_flag_count >= c->link_flag_capacity) {
                c->link_flag_capacity = 8;
                c->link_flags = malloc(sizeof(char*) * c->link_flag_capacity);
            }
            c->link_flags[c->link_flag_count++] = strdup("-lpython3");
        }

        for (int i = 0; i < c->python_pkg_count; i++) {
            char cmd[4096];
            snprintf(cmd, sizeof(cmd),
                     "python3 -c \"import %s; print(%s.__file__)\" 2>/dev/null",
                     c->python_pkgs[i], c->python_pkgs[i]);
            fp = popen(cmd, "r");
            if (fp) {
                char buf[1024] = {0};
                if (fgets(buf, sizeof(buf), fp)) {
                    buf[strcspn(buf, "\n")] = 0;
                    if (strstr(buf, ".so") || strstr(buf, ".dylib")) {
                        if (c->link_flag_count >= c->link_flag_capacity) {
                            c->link_flag_capacity = c->link_flag_capacity ? c->link_flag_capacity * 2 : 8;
                            c->link_flags = realloc(c->link_flags, sizeof(char*) * c->link_flag_capacity);
                        }
                        char flag[4096];
                        char *last_slash = strrchr(buf, '/');
                        if (last_slash) {
                            *last_slash = 0;
                            snprintf(flag, sizeof(flag), "-L%s", buf);
                            c->link_flags[c->link_flag_count++] = strdup(flag);
                            *last_slash = '/';
                        }
                        char *so_name = strrchr(buf, '/');
                        if (so_name) so_name++; else so_name = buf;
                        char *libname = strdup(so_name);
                        if (strncmp(libname, "lib", 3) == 0) {
                            memmove(libname, libname + 3, strlen(libname) - 2);
                        }
                        char *dot = strrchr(libname, '.');
                        if (dot) *dot = 0;
                        snprintf(flag, sizeof(flag), "-l%s", libname);
                        c->link_flags[c->link_flag_count++] = strdup(flag);
                        free(libname);
                    }
                }
                pclose(fp);
            }
        }
    }

    if (c->use_cargo) {
        const char *cargo_dir = c->project_dir ? c->project_dir : ".";

        if (c->use_cargo != 2) {
            char cmd[4096];
            snprintf(cmd, sizeof(cmd), "cd %s && cargo build --release 2>/dev/null", cargo_dir);
            int r = system(cmd);
            if (r != 0 && c->verbose) {
                printf("Warning: 'cargo build --release' failed (exit %d)\n", r);
            }
        }

        char toml_path[4096];
        snprintf(toml_path, sizeof(toml_path), "%s/Cargo.toml", cargo_dir);
        FILE *fp = fopen(toml_path, "r");
        char crate_name[256] = {0};
        if (fp) {
            char line[1024];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "name", 4) == 0) {
                    char *eq = strchr(line, '=');
                    if (eq) {
                        char *start = eq + 1;
                        while (*start == ' ' || *start == '"' || *start == '\'') start++;
                        char *end = start;
                        while (*end && *end != '"' && *end != '\'' && *end != '\n') end++;
                        int len = end - start;
                        if (len > 0 && len < 256) {
                            strncpy(crate_name, start, len);
                            crate_name[len] = 0;
                        }
                    }
                    break;
                }
            }
            fclose(fp);
        }

        if (crate_name[0]) {
            if (c->link_flag_count + 2 > c->link_flag_capacity) {
                c->link_flag_capacity = c->link_flag_capacity ? c->link_flag_capacity * 2 : 8;
                c->link_flags = realloc(c->link_flags, sizeof(char*) * c->link_flag_capacity);
            }
            char flag[4096];
            snprintf(flag, sizeof(flag), "-l%s", crate_name);
            c->link_flags[c->link_flag_count++] = strdup(flag);
            snprintf(flag, sizeof(flag), "-L%s/target/release", cargo_dir);
            c->link_flags[c->link_flag_count++] = strdup(flag);
        } else if (c->verbose) {
            printf("Warning: Cargo.toml not found in '%s'\n", cargo_dir);
        }
    }

    *count = c->link_flag_count;
    return (const char **)c->link_flags;
}

const char **compiler_get_bridge_c_sources(Compiler *c, int *count) {
    *count = c->bridge_c_src_count;
    return (const char **)c->bridge_c_srcs;
}

const char **compiler_get_bridge_cb_sources(Compiler *c, int *count) {
    *count = c->bridge_cb_src_count;
    return (const char **)c->bridge_cb_srcs;
}
