
# Documentation

## Install

Currently you can only build from source. Prebuilt binaries will be coming soon.

macOS: `brew install llvm@15 && brew link llvm@15`

Ubuntu / Debian: `sudo apt-get install llvm-15 clang-15 lld libcurl4-openssl-dev`

```
git clone https://github.com/volt-lang/volt.git
cd volt
make
```

## Table of contents

<table>
<tr><td>

* [Basic example](#basic-example)
* [Multiple files](#multiple-files)
* [Namespaces](#namespaces)
* [Packages](#packages)
* [Types](#types)
* [Variables](#variables)
* [Functions](#functions)
* [Classes](#classes)

</td><td>

* [Definitions](#definitions)
    * [Functions](#functions)
        * [ErrorHandling](#error-handling)
    * [Classes](#classes)
    * [Globals](#globals)
* [Tokens](#tokens)
    * [Let](#variables)
    * [If/Else](#if-else)
    * [While](#while)
    * [Throw](#error-handling)

</td></tr>
</table>

## Basic example

```rust
// main.vo
fn main() {
    println("Hello world!" + " 🎉")
}
```

```sh
volt build main.vo -o ./main
./main
```

## Multiple files

To build multiple files into a program, you simply add them to the build command. However, we recommend to use only 1 file (e.g. main.vo) and put all other files into a namespace. (See next chapter)

```sh
volt build file-1.vo file-2.vo -o ./main
./main
```

## Namespaces

To organize your code we group files into different directories. Each namespace represents 1 directory. To create a namespace, you must define it in your config file `volt.json`. Which should be located in the root of your project.

```json
{
    "namespaces": {
        "my_namespace": "src/my-namespace",
        "models": "src/database/models",
        "controllers": "src/controllers"
    }
}
```

```rust
// main.vo
use my_namespace

fn main() {
    // Calling a function from another namespace
    my_namespace:thumbs_up()
}
```

```rust
// src/my-namespace/demo.vo
fn thumbs_up() {
    println("👍")
}
```

## Packages

Work in progress.

## Types

Types: `String` `Array` `Map`

Integer types: `i8` `i16` `i32` `i64` `int` `u8` `u16` `u32` `u64` `uint`

Float types: (WIP)

Other: `ptr` <- raw pointer (unsafe)

## Variables

```rust
// let {name} [: {type}] = {value}
let a = 5            // int (default)
let b : uint = 5     // uint
let c : u8 = "test"  // Compile error
let d = b @as u8     // Casting
```

## Definitions

### Functions

```rust
fn add(arg1: int, arg2: int (5)) int {
    return arg1 + arg2
}

fn main() {
    add(1) // result: 6
    add(2, 2) // result: 4
}
```

### Error handling

```rust
fn add(value: int) int !too_big {
    if value > 100 {
        throw too_big
    }
    return value + 10
}

fn main() {
    // Alternative value in case of an error
    let x = add(10) ? 0 // result: 20
    x = add(200) ? 5 // result: 5

    // Alternative value using scope
    x = add(200) ? <{
        println("We had an error 😢")
        return 210
    }
    // result: 210

    // Exit the function on error
    x = add(200) ! {
        println("We had an error 😢")
        return // main has a void return type, so we use an empty return
    }
    // Single line
    x = add(200) ! return
    // Break / continue loops on error
    while true {
        x = add(200) ! break // or continue
        x = add(200) ! {
            println("error, stop the loop")
            break
        }
    }
}
```

### Classes

```rust
class MyClass {
    a: String
    b: String ("default value")
    c: uint (100)
}

fn main() {
    let obj = MyClass {
        a: "TEST"
    }
    println(obj.a)   // output: TEST
    obj.a = "UPDATE" // Set property
    println(obj.a)   // output: UPDATE
    println(obj.b)   // output: default value
    println(obj.c)   // output: 100
}
```

## Tokens

### If Else

```rust
if a == b : ...code...     // Single line
if a == b { ...code... }
else if a == c { ... }
else { ... }
```

### While

```rust
let x = 0
while x++ < 5 {
    // 1 - 5
    if x == 1 : continue
    if x == 4 : break
    print(x)
}
// output: 2 3
```
