
<div align="center">
<p>
    <img width="170" src="https://raw.githubusercontent.com/valk-lang/valk/master/misc/valk.svg">
</p>
</div>

# Valk programming language

[Website](https://valk-lang.dev) | [Documentation](https://github.com/valk-lang/valk/blob/main/docs/docs.md) | [Roadmap](https://github.com/valk-lang/valk/blob/main/ROADMAP.md) | [Discord](https://discord.gg/RwEGqdSERA)

Valk is a programming language aimed to be fast & simple at the same time. It can be used for high & mid level programming. Valk is unique because of its new way of doing garbage collection. Its runtime is much faster than go and in some cases rust, while also using less memory. On top of that, a GC allows us to keep the language very simple like python. You get the best of both worlds.

**Features**: Fastest GC ⚡ (no stop-the-world), Coroutines, No undefined behaviour, Great package management, Generics, Fast compile times, Cross compiling, linking c-libraries.

**Coroutines** are purely for concurrency. Threads can be used for parallelism.


## Install

```sh
curl -s https://valk-lang.dev/install.sh | bash -s latest
```

## Basic example

```rust
// main.va
fn main() {
    println("Hello Valk! 🎉")
}
```

```sh
valk build main.va -o ./main
./main
```

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm@15 && brew link llvm@15`

Ubuntu / Debian: `sudo apt-get install llvm-15 clang-15 lld libcurl4-openssl-dev`

```bash
git clone https://github.com/valk-lang/valk.git
cd valk
make
```

## Supported platforms

| OS | Linux | Macos | Windows |
|--|--|--|--|
| x64 | ✅ | ✅ | ✅ |
| arm64 | ❌ | ✅ | ❌ |

We plan to support x64 & arm64 on linux, macos and windows first. More will be added later.

## Benchmarks

<div align="center"><p>
    <img src="https://raw.githubusercontent.com/valk-lang/valk/master/misc/valk-bintree.png">
</p>
The binary object tree test revolves around creating large amount of objects in a tree structure, iterating over them and doing some calculations.
</div>

---

<div align="center"><p>
    <img src="https://raw.githubusercontent.com/valk-lang/valk/master/misc/valk-http.png">
</p></div>

## Is Valk faster than Rust?

No, it should run at the same speed. The only advantage Valk has is with creating and free-ing objects, which is important for many applications. Valk creates objects in tiny incremental blocks and keeps these blocks around for a few milliseconds when they are empty. This reduces the amount of `malloc` and `free` calls we have to make.

Doesnt the GC slow Valk down? Yes, it does. But the blocks optimization gives us much more performance gain than loss. The GC is also super fast and has to do much less tracing compared to other GCs.

TL;DR; Valk will sometimes be faster, sometimes be slower. Each language has it's own strength.

## Language design facts

- Each thread handles it's own memory, but you can still share your variables with other threads. Even when your thread has ended, the memory will still be valid and automatically freed once other threads no longer use it.

- Valk does not force the user to use mutexes/locks for shared memory. Therefore the program can crash when you use multiple threads to modify the same data at the same time. Reading the same data with multiple threads is fine.
- Unlike other languages, our GC has no randomness. Every run is exactly the same as the run before to the last byte. So there are no fluctuations in cpu usage and memory usage. (Except when using shared memory over multiple threads)

- The GC does not guess which value on the stack is a GC'able pointer. Instead we create a custom stack and the compiler knows where to store which pointer at compile time.

## Contributions

Once we hit version 0.1.0, we want to look for people who can help with the standard library & 3rd party packages. If you want to contribute, just hop into the discord and post in general chat or send a private message to the discord owner.

## References

Binary tree benchmark code: [https://programming-language-benchmarks.vercel.app/problem/binarytrees](https://programming-language-benchmarks.vercel.app/problem/binarytrees)

