# Cobra Language Reference

Cobra is a modern, compiled programming language with a focus on safety, performance, and readability. It uses indentation-based syntax (like Python) with static typing and compiles to native code via a custom compiler that targets assembly.

---

## Comments

Cobra supports three comment styles:

```cobra
# Line comment (hash style)

// Line comment (C-style)

/* Block comment
   spanning multiple lines */
```

Block comments can span multiple lines but do not nest.

---

## Variables

### `let` — Immutable bindings

```cobra
let x = 42
let name: str = "Cobra"
let pi: f64 = 3.14159
```

Variables declared with `let` are **immutable** by default — they cannot be reassigned.

### `var` — Mutable bindings

```cobra
var count = 0
var message: str = "hello"
count = count + 1
```

The `var` keyword declares a mutable variable. The `mut` modifier can also be used with `let`:

```cobra
let mut counter = 0
counter = counter + 1
```

### `const` — Compile-time constants

```cobra
const MAX_SIZE = 1024
const APP_NAME: str = "MyApp"
```

Constants are immutable, evaluated at compile time, and must have a known value at the point of declaration.

### Type annotations

Type annotations are optional when the type can be inferred:

```cobra
let x = 42        # inferred as int
let y: i64 = 42   # explicit type
let z: u8 = 255   # unsigned 8-bit
```

---

## Types

### Primitive Types

| Type    | Description                    | Size    |
| ------- | ------------------------------ | ------- |
| `int`   | Platform-dependent integer     | 32/64bit |
| `i8`    | Signed 8-bit integer           | 1 byte  |
| `i16`   | Signed 16-bit integer          | 2 bytes |
| `i32`   | Signed 32-bit integer          | 4 bytes |
| `i64`   | Signed 64-bit integer          | 8 bytes |
| `u8`    | Unsigned 8-bit integer         | 1 byte  |
| `u16`   | Unsigned 16-bit integer        | 2 bytes |
| `u32`   | Unsigned 32-bit integer        | 4 bytes |
| `u64`   | Unsigned 64-bit integer        | 8 bytes |
| `f32`   | 32-bit floating point          | 4 bytes |
| `f64`   | 64-bit floating point          | 8 bytes |
| `bool`  | Boolean (`true` / `false`)     | 1 byte  |
| `str`   | String (immutable UTF-8)       | pointer |
| `char`  | Single character               | 4 bytes |

### Literal syntax

```cobra
42              # integer
3.14            # float
3.14e-2         # scientific notation
true            # boolean
false           # boolean
"hello world"   # string
'x'             # character
nil             # null value
```

### Compound Types

- **Arrays**: `[type; size]` — fixed-size
- **Slices**: `[type]` — dynamically sized view
- **Tuples**: `(type1, type2, ...)`
- **Pointers**: `*type`
- **Optionals**: `?type` — nullable wrapper
- **Result**: `Result<type, error>` — error handling

---

## Functions

Functions are defined with the `fn` keyword:

```cobra
fn greet(name: str) {
    println("Hello, " + name)
}
```

### Parameters and return types

```cobra
fn add(a: int, b: int) -> int {
    return a + b
}

fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}
```

### Expression body syntax

Short functions can use `=>` instead of a block:

```cobra
fn square(x: int) -> int => x * x
```

### Default parameter values

```cobra
fn greet(name: str = "World") {
    println("Hello, " + name)
}
```

### Async functions

```cobra
async fn fetch_data(url: str) -> str {
    // ...
}

fn main() {
    let data = await fetch_data("https://example.com")
}
```

### Public functions

```cobra
pub fn exported_function() {
    // accessible outside the module
}
```

### The `main` function

Every executable program must have a `main` function:

```cobra
fn main() {
    println("Hello, World!")
}
```

---

## Control Flow

### `if` / `elif` / `else`

```cobra
let x = 42

if x > 100 {
    println("Large")
} elif x > 50 {
    println("Medium")
} else {
    println("Small")
}
```

Conditions do **not** require parentheses, but the body must be indented after a colon.

### If-expressions

```cobra
let status = if x > 0 { "positive" } else { "non-positive" }
```

### `while` loops

```cobra
let i = 0
while i < 5 {
    println(i)
    i = i + 1
}
```

### `for` loops

```cobra
# Exclusive range (0..n)
for i in 0..5 {
    println(i)
}

# Inclusive range (0..=n)
for i in 0..=5 {
    println(i)
}

# Over a list
for item in items {
    println(item)
}
```

### `break` and `continue`

```cobra
for i in 0..100 {
    if i == 5 {
        break
    }
    if i % 2 == 0 {
        continue
    }
    println(i)
}
```

### `match` — Pattern matching

```cobra
match value {
    1 => println("one")
    2 => println("two")
    _ => println("other")
}
```

Match arms use `=>` to separate patterns from expressions. The underscore `_` is a wildcard that matches anything.

Match arms can have guard conditions:

```cobra
match x {
    n if n > 0 => println("positive")
    n if n < 0 => println("negative")
    _ => println("zero")
}
```

---

## Structs

```cobra
struct Person {
    name: str
    age: int
    email: str
}
```

### Creating instances

```cobra
let person = Person{
    name: "Alice",
    age: 30,
    email: "alice@example.com"
}
```

### Accessing fields

```cobra
println(person.name)
println(person.age)
```

### Public structs

```cobra
pub struct Point {
    x: f64
    y: f64
}
```

---

## Classes

```cobra
class Animal {
    name: str
    species: str

    fn describe(self) -> str {
        return self.name + " is a " + self.species
    }
}
```

Classes support fields and methods. Methods receive `self` as their first parameter.

### Creating instances

```cobra
let animal = Animal{
    name: "Buddy",
    species: "Dog"
}
println(animal.describe())
```

### Public classes

```cobra
pub class Vehicle {
    // ...
}
```

---

## Enums

```cobra
enum Color {
    Red
    Green
    Blue
    Yellow
}
```

Enums define a type with a fixed set of named variants.

### Public enums

```cobra
pub enum Direction {
    North
    South
    East
    West
}
```

---

## Traits

```cobra
trait Printable {
    fn print(self)
}
```

Traits define shared behavior (method signatures) that types can implement.

### Implementing a trait

```cobra
impl Person: Printable {
    fn print(self) {
        println(self.name)
    }
}
```

---

## Operators

### Arithmetic

| Operator | Description      |
| -------- | ---------------- |
| `+`      | Addition         |
| `-`      | Subtraction      |
| `*`      | Multiplication   |
| `/`      | Division         |
| `%`      | Modulo/Remainder |

### Comparison

| Operator | Description            |
| -------- | ---------------------- |
| `==`     | Equal to               |
| `!=`     | Not equal to           |
| `<`      | Less than              |
| `>`      | Greater than           |
| `<=`     | Less than or equal     |
| `>=`     | Greater than or equal  |

### Logical

| Operator | Description |
| -------- | ----------- |
| `and`    | Logical AND |
| `or`     | Logical OR  |
| `not`    | Logical NOT |
| `&&`     | Logical AND |
| `\|\|`   | Logical OR  |
| `!`      | Logical NOT |

### Bitwise

| Operator | Description    |
| -------- | -------------- |
| `&`      | Bitwise AND    |
| `\|`     | Bitwise OR     |
| `^`      | Bitwise XOR    |
| `<<`     | Left shift     |
| `>>`     | Right shift    |

### Assignment

| Operator | Description       |
| -------- | ----------------- |
| `=`      | Assignment        |
| `+=`     | Add and assign    |
| `-=`     | Subtract and assign |
| `*=`     | Multiply and assign |
| `/=`     | Divide and assign |

### Increment / Decrement

| Operator | Description    |
| -------- | -------------- |
| `++`     | Increment by 1 |
| `--`     | Decrement by 1 |

### Other

| Operator | Description                        |
| -------- | ---------------------------------- |
| `.`      | Member access / method call        |
| `..`     | Exclusive range                    |
| `..=`    | Inclusive range                    |
| `->`     | Return type annotation             |
| `=>`     | Match arm / expression body        |
| `as`     | Type coercion                      |
| `is`     | Type check                         |

### Operator precedence (lowest to highest)

| Level | Operators                |
| ----- | ------------------------ |
| 1     | `or`, `\|\|`            |
| 2     | `and`, `&&`             |
| 3     | `\|`                    |
| 4     | `^`                     |
| 5     | `&`                     |
| 6     | `==`, `!=`              |
| 7     | `<`, `>`, `<=`, `>=`   |
| 8     | `<<`, `>>`              |
| 9     | `+`, `-`                |
| 10    | `*`, `/`, `%`           |

---

## Pattern Matching

The `match` expression enables powerful pattern matching:

```cobra
match value {
    pattern1 => expression1
    pattern2 if guard_condition => expression2
    pattern3 => {
        # multi-line block
        expression3
    }
    _ => default_expression
}
```

### Patterns

- **Literal patterns**: match exact values (`1`, `"hello"`, `true`)
- **Variable patterns**: bind matched value to a name (`x`, `n`)
- **Wildcard pattern**: matches anything (`_`)
- **Guard conditions**: additional filter (`if condition`)

---

## Error Handling

### `try` expressions

```cobra
let result = try risky_operation()
```

### `throw` statements

```cobra
fn divide(a: int, b: int) -> int {
    if b == 0 {
        throw "Division by zero"
    }
    return a / b
}
```

### Result types

Functions that can fail should return `Result<T, E>`:

```cobra
fn parse_int(s: str) -> Result<int, str> {
    // ...
}
```

---

## Modules and Imports

### Importing modules

```cobra
import math
import io.file
```

### Public visibility

Use `pub` to make items accessible outside the module:

```cobra
pub fn exported_function() { }
pub struct PublicType { }
pub const API_VERSION = 2
```

By default, everything is private to the module.

### Project structure

```
myproject/
├── cobra.json           # Project manifest
├── src/
│   └── main.cb          # Entry point
└── tests/
    └── test_main.cb     # Tests
```

### `cobra.json` manifest

```json
{
    "name": "myproject",
    "version": "0.1.0",
    "description": "A Cobra project",
    "author": "",
    "license": "MIT",
    "dependencies": {}
}
```

---

## Concurrency

### `async` functions

Declare asynchronous functions with the `async` keyword:

```cobra
async fn fetch_data(url: str) -> str {
    # async implementation
}
```

### `await` expressions

Call async functions with `await`:

```cobra
let data = await fetch_data("https://example.com")
```

---

## Other Features

### `defer`

Schedule code to run when the current scope exits:

```cobra
fn process_file() {
    let file = open("data.txt")
    defer close(file)
    # file is automatically closed when this function returns
}
```

### `unsafe`

Mark code blocks for unsafe operations:

```cobra
unsafe {
    # raw pointer dereference, etc.
}
```

### Type aliases

```cobra
type Age = int
type UserId = i64
```

### `self` and `super`

- `self` refers to the current instance (in methods)
- `super` refers to the parent class

### Type checking and coercion

```cobra
if x is str {
    # x is a string
}

let num = "42" as int   # type coercion
```
