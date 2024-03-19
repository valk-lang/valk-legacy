
<div align="center">
<p>
    <img width="170" src="https://raw.githubusercontent.com/volt-lang/volt/master/misc/logo.png">
</p>
</div>

# Volt programming language

[Website](https://voltx.dev) | [Documentation](https://github.com/volt-lang/volt/blob/main/docs/README.md) | [Roadmap](https://github.com/volt-lang/volt/blob/main/ROADMAP.md) | [Discord](https://discord.gg/RwEGqdSERA)


Volt is a programming language aimed to be fast & simple at the same time. It can be used for high & low level programming. Volt is unique because of its new way of doing garbage collection. It's much faster than go and in some cases rust, while also using less memory. On top of that, a GC allows us to keep the language very simple like python. You get the best of both worlds.

**Features**: Super fast non-blocking GC 🙌, No undefined behaviour, Great package management, Generics, Fast compile times, Cross compiling, Using c-libraries.

## Install

```sh
// SOON : You can only build from source at the moment
```

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

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm@15 && brew link llvm@15`

Ubuntu / Debian: `sudo apt-get install llvm-15 clang-15 lld libcurl4-openssl-dev`

```bash
git clone https://github.com/volt-lang/volt.git
cd volt
make
```

## How can it have faster/similar performance as Rust?

Volt is only faster in the way it creates and manages objects, which most programs revolve around. Objects are created using pools. These pools are much faster than using malloc and free all the time (and uses less memory). A GC always has some overhead, but the overall performance gain is much higher than the loss. Which results in Volt being faster than Rust sometimes. Note that our way of doing GC is very different than other languages. Each thread manages its own memory and we only trace when we absolutely have to. 9 out of 10 times we simply clean up the pools instead.

## Benchmarks

<div align="center"><p>
    <img src="https://raw.githubusercontent.com/volt-lang/volt/master/misc/volt-bintree.png">
    <img src="https://raw.githubusercontent.com/volt-lang/volt/master/misc/volt-http.png">
</p></div>


## References

Bintree code used for benchmarks: [https://programming-language-benchmarks.vercel.app/problem/binarytrees](https://programming-language-benchmarks.vercel.app/problem/binarytrees)
