<img src="branding/logo.svg" alt="Cobra Logo" width="400"/>

# Cobra Programming Language

**The Python-inspired language with C-level performance.**

Cobra is a modern, systems-level programming language that combines the simplicity and readability of Python with the raw performance of C and Rust.

```cobra
fn main() {
    println("Hello, World!")
}
```

## Features

- **Pythonic Syntax** — Clean, indentation-based syntax that's easy to read and write
- **Blazing Fast** — Compiled directly to native machine code via x86-64 assembly
- **Memory Safe** — Optional memory safety guarantees
- **Rich Type System** — Static typing with full type inference
- **Zero-Cost Abstractions** — High-level constructs compile to optimal machine code
- **Built-in Package Manager** — Download, manage, and publish packages
- **Cross-Platform** — macOS and Linux support
- **Modern Concurrency** — Async/await, coroutines, and threads

## Quick Start

### Install

```bash
curl -fsSL https://cobra-lang.org/install.sh | sh
```

### Or Build from Source

```bash
git clone https://github.com/Xzander1984/Cobra.git
cd Cobra
make build
```

### Create a Project

```bash
cobra new myproject
cd myproject
cobra run
```

### Hello World

```cobra
// hello.cb
fn main() {
    println("Hello, World!")
}
```

```bash
cobra build
./build/program
```

## Examples

### Fibonacci

```cobra
fn fibonacci(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

fn main() {
    for i in 0..10 {
        println("fib(" + i + ") = " + fibonacci(i))
    }
}
```

### Structs

```cobra
struct Person {
    name: str
    age: int
}

fn main() {
    let person = Person{
        name: "Alice",
        age: 30
    }
    println(person.name)
}
```

## Project Structure

```
cobra/
├── compiler/        # Cobra compiler (C)
├── cli/             # Command-line interface
├── runtime/         # Runtime library
├── stdlib/          # Standard library
├── package-manager/ # Package manager
├── vscode-extension/ # VS Code extension
├── website/         # Official website
├── branding/        # Logos and branding
├── docs/            # Documentation
├── examples/        # Example projects
├── tests/           # Tests
├── installer/       # Installation scripts
└── benchmarks/      # Performance benchmarks
```

## CLI Commands

| Command | Description |
|---------|-------------|
| `cobra new` | Create a new project |
| `cobra init` | Initialize a project |
| `cobra build` | Build the project |
| `cobra run` | Build and run |
| `cobra test` | Run tests |
| `cobra clean` | Clean artifacts |
| `cobra check` | Check for errors |
| `cobra fmt` | Format code |
| `cobra lint` | Lint code |
| `cobra docs` | Generate docs |
| `cobra repl` | Interactive REPL |
| `cobra doctor` | System check |

## Performance

Cobra targets native performance that competes with C, Rust, and Zig through:
- Ahead-of-time (AOT) compilation
- LLVM-quality optimizations
- Minimal runtime overhead
- Zero-cost abstractions
- Efficient memory management

## Documentation

Full documentation is available in the [docs](docs/) directory:
- [Getting Started](docs/getting-started.md)
- [Language Reference](docs/language.md)
- [Examples](examples/)

## VS Code Extension

Install the Cobra VS Code extension for syntax highlighting, code snippets, and language support.

## Contributing

Contributions are welcome! See our [roadmap](docs/roadmap.md) for planned features.

## License

MIT License — see [LICENSE](LICENSE) for details.

## Community

- [GitHub](https://github.com/Xzander1984/Cobra)
- Documentation: [https://cobra-lang.org](https://cobra-lang.org)
