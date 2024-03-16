
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
<tr><td width="50%">

* [Basic example](#basic-exmaple)
* [Multiple files](#multiple-files)
* [Namespaces](#namespaces)
* [Packages](#packages)
* [Types](#types)
* [Variables](#variables)

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

To organize your code, we group files into different directories. Each namespace in Volt represent 1 directory. To create a namespace, you must define it in your config file `volt.json` which should be located in the root of your project.

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
let a = 5             // int (default)
let b : uint = 5      // uint
let c : u8 = "test"   // Compile error
let d : u8 = b @as u8 // Casting
```
