
#if OS == linux

link_dynamic "pthread"
link_dynamic "c"
link_dynamic ":libc_nonshared.a"
link_dynamic ":ld-linux-x86-64.so.2"

header "linux/abi"
#if ARCH == arm64
header "linux/arm64/libc-enums"
header "linux/arm64/libc-gen"
#else
header "linux/x64/libc-enums"
header "linux/x64/libc-gen"
#end
header "pthread"

#elif OS == macos

link_dynamic "System"

header "macos/abi"
#if ARCH == arm64
header "macos/arm64/enum"
header "macos/arm64/libc-enums"
header "macos/arm64/libc-gen"
#elif ARCH == x64
header "macos/x64/enum"
header "macos/x64/libc-enums"
header "macos/x64/libc-gen"
#end
header "pthread"

#elif OS == win

link_static "kernel32"
link_static "ws2_32"

#if STATIC
link_dynamic "ucrt" // c99 functions (dynamic)
link_dynamic "msvcrt" // Microsoft visual c++ runtime (dynamic)
link_dynamic "libvcruntime" // Microsoft visual c++ runtime (static)

// link_static "libucrt" // c99 functions (static)
// link_static "libcmt" // lib-c (static)
// link_static "libcpmt" // lib-c++ (static)
// link_static "libvcruntime" // Microsoft visual c++ runtime (static)
#else
link_dynamic "ucrt" // c99 functions (dynamic)
link_dynamic "msvcrt" // Microsoft visual c++ runtime (dynamic)
link_dynamic "libvcruntime" // Microsoft visual c++ runtime (static)
#end

header "win/structs"
header "win/abi"
header "win/x64/enum"
header "win/x64/libc-enums"
header "win/x64/libc-gen"

#end
