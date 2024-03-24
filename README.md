
<div align="center">
<p>
    <img width="170" src="https://raw.githubusercontent.com/vali-lang/vali/master/misc/vali.svg">
</p>
</div>

# Vali programming language

[Website](https://vali.dev) | [Documentation](https://github.com/vali-lang/vali/blob/main/docs/docs.md) | [Roadmap](https://github.com/vali-lang/vali/blob/main/ROADMAP.md) | [Discord](https://discord.gg/RwEGqdSERA)


Vali is a programming language aimed to be fast & simple at the same time. It can be used for high & low level programming. Vali is unique because of its new way of doing garbage collection. It's much faster than go and in some cases rust, while also using less memory. On top of that, a GC allows us to keep the language very simple like python. You get the best of both worlds.

**Features**: Super fast non-blocking GC ⚡, No undefined behaviour, Great package management, Generics, Fast compile times, Cross compiling, linking c-libraries.

## Install

```sh
// SOON : You can only build from source at the moment
```

## Basic example

```rust
// main.va
fn main() {
    println("Hello Vali! 🎉")
}
```

```sh
vali build main.va -o ./main
./main
```

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm@15 && brew link llvm@15`

Ubuntu / Debian: `sudo apt-get install llvm-15 clang-15 lld libcurl4-openssl-dev`

```bash
git clone https://github.com/vali-lang/vali.git
cd vali
make
```

## How can it have faster/similar performance as Rust?

Vali is only faster in the way it creates and manages objects, which most programs revolve around. Objects are created using pools. These pools are much faster than using malloc and free all the time (and uses less memory). A GC always has some overhead, but the overall performance gain is much higher than the loss. Which results in Vali being faster than Rust sometimes. Note that our way of doing GC is very different than other languages. Each thread manages its own memory and we only trace when we absolutely have to. 9 out of 10 times we simply clean up the pools instead.

## Benchmarks

<div align="center"><p>
    <img src="https://raw.githubusercontent.com/vali-lang/vali/master/misc/vali-bintree.png">
    <img src="https://raw.githubusercontent.com/vali-lang/vali/master/misc/vali-http.png">
</p></div>

## Good to know

- Each thread handles it's own memory, but you can still share your variables with other threads. Even when your thread has ended, the memory will still be valid and automatically freed once other threads no longer use it.

- Vali does not force the user to use mutexes/locks for shared memory. Therefore the program can crash when you use multiple threads to modify the same data at the same time. Reading the same data with multiple threads is fine.

- Unlike other languages, our GC has no randomness. Every run is exactly the same as the run before to the last byte. So there are no fluctuations in cpu usage and memory usage. (Except when using shared memory over multiple threads)

## Contributions

Once we hit version 0.1.0, we want to look for people who can help with: the standard library, packages and supporting SIMD. If you want to contribute, just hop into the discord and post in general chat or send a private message to the discord owner.

## References

Binary tree benchmark code: [https://programming-language-benchmarks.vercel.app/problem/binarytrees](https://programming-language-benchmarks.vercel.app/problem/binarytrees)
