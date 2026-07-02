# Getting Started with Cobra

Welcome to Cobra! This guide will help you install the Cobra compiler, create your first project, and learn the basic workflow.

---

## Installation

### Prerequisites

- **macOS** or **Linux**
- A C compiler (Clang or GCC)
- Make
- Git (optional, for building from source)

### Option 1: Install via script (recommended)

```bash
curl -fsSL https://cobra-lang.org/install.sh | sh
```

This downloads the latest prebuilt binary for your platform and installs it to `/usr/local/bin`.

You can customize the installation:

```bash
# Install a specific version
COBRA_VERSION="v0.1.0" curl -fsSL https://cobra-lang.org/install.sh | sh

# Install to a custom directory
COBRA_INSTALL_DIR="$HOME/.local/bin" curl -fsSL https://cobra-lang.org/install.sh | sh
```

### Option 2: Build from source

```bash
# Clone the repository
git clone https://github.com/cobra-lang/cobra.git
cd cobra

# Build everything
make build

# Install to /usr/local/bin
make install
```

### Option 3: Download a release

Download the latest release tarball for your platform from the [releases page](https://github.com/cobra-lang/cobra/releases), then extract and install:

```bash
tar xzf cobra-linux-x86_64.tar.gz
sudo cp cobra /usr/local/bin/
```

### Verify installation

```bash
cobra --version
```

You should see the Cobra banner and version information.

```bash
cobra doctor
```

This checks that all required tools (compiler, assembler, linker) are available.

---

## Your First Cobra Program

Create a file called `hello.cb`:

```cobra
// Hello World in Cobra

fn main() {
    println("Hello, World!")
    println("Welcome to Cobra!")
}
```

### Building and running

Use the Cobra CLI to compile and run:

```bash
cobra run hello.cb
```

Or compile and run in separate steps:

```bash
cobra build hello.cb
./build/program
```

Behind the scenes, `cobra build`:
1. Compiles `.cb` source to assembly (via `cobrac`)
2. Assembles with Clang/GCC
3. Links into a native executable

---

## Creating Projects

### Using `cobra new`

```bash
cobra new my-project
cd my-project
```

This creates:

```
my-project/
├── cobra.json       # Project manifest
├── src/
│   └── main.cb      # Entry point
└── tests/
    └── test_main.cb # Tests
```

### Using `cobra init`

To add Cobra to an existing directory:

```bash
mkdir my-project
cd my-project
cobra init
```

### Project structure

A typical Cobra project:

```
my-project/
├── cobra.json          # Project configuration
├── src/
│   ├── main.cb        # Entry point
│   ├── utils.cb       # Utility functions
│   └── modules/       # Submodules
├── tests/
│   └── main.cb        # Test file
├── build/             # Build output (generated)
└── docs/              # Documentation
```

### The manifest file (`cobra.json`)

```json
{
    "name": "my-project",
    "version": "0.1.0",
    "description": "A Cobra project",
    "author": "Your Name",
    "license": "MIT",
    "dependencies": {}
}
```

---

## CLI Commands

| Command      | Description                              |
| ------------ | ---------------------------------------- |
| `new`        | Create a new Cobra project               |
| `init`       | Initialize Cobra in current directory    |
| `build`      | Build the current project                |
| `run`        | Build and run the current project        |
| `test`       | Run tests                                |
| `clean`      | Remove build artifacts                   |
| `check`      | Check code for errors (no output)        |
| `fmt`        | Format source code                       |
| `lint`       | Lint source code                         |
| `docs`       | Generate documentation                   |
| `package`    | Package project for distribution         |
| `publish`    | Publish package to registry              |
| `install`    | Install a package                        |
| `remove`     | Remove a package                         |
| `update`     | Update packages                          |
| `search`     | Search the package registry              |
| `version`    | Show version information                 |
| `doctor`     | Check system setup                       |
| `repl`       | Start interactive REPL                   |
| `help`       | Show help message                        |

---

## Building and Running

### Basic workflow

```bash
# Build the project
cobra build

# Build and run
cobra run

# Check for errors without generating output
cobra check

# Clean build artifacts
cobra clean
```

### Compiler options

The compiler (`cobrac`) can be used directly for more control:

```bash
cobrac src/main.cb -o build/output.s   # Compile to assembly
cobrac -O2 src/main.cb                 # Optimize with level 2
cobrac -v src/main.cb                  # Verbose output
```

Compiler flags:

| Option       | Description                   |
| ------------ | ----------------------------- |
| `-o <file>`  | Set output file               |
| `-O<level>`  | Optimization level (0-3)      |
| `-S`         | Generate assembly only        |
| `-v`         | Verbose output                |
| `--help`     | Show help                     |
| `--version`  | Show version                  |

---

## Package Management

### Installing packages

```bash
cobra install package-name
```

### Creating a package

```bash
cobra new my-library
cd my-library
# ... develop your library ...
cobra package
```

This creates a distributable archive in `dist/`.

### Publishing

```bash
cobra publish
```

*Note: Package registry integration is under development.*

---

## REPL (Interactive Mode)

Cobra includes a REPL for experimentation:

```bash
cobra repl
```

Example session:

```
>> "Hello" + " World"
Hello World
>> 2 + 2
4
>> exit
Goodbye!
```

---

## Testing

Create test files in the `tests/` directory:

```bash
cobra test
```

Test files follow the same syntax as regular Cobra programs.

---

## Formatting and Linting

### Format code

```bash
cobra fmt
```

Formats all `.cb` files in the `src/` directory.

### Lint code

```bash
cobra lint
```

Checks code style and reports issues.

---

## Next Steps

- Read the [Language Reference](language.md) for complete syntax details
- Explore the [examples](../examples/) directory for sample programs
- Check the standard library documentation for built-in functionality

---

## Troubleshooting

### `cobrac: command not found`

Ensure the compiler binary is in your PATH:

```bash
export PATH="$PATH:/usr/local/bin"
```

### Build fails with assembler errors

Make sure Clang or GCC is installed:

```bash
# macOS
xcode-select --install

# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora
sudo dnf install gcc
```
