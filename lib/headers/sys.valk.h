
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
link_dynamic "ucrt" // dynamic c-runtime
link_dynamic "vcruntime" // dynamic c-runtime
link_dynamic "msvcrt" // dynamic c-runtime startup
link_dynamic "msvcprt" // dynamic c++
link_dynamic "oldnames"
// link_static "libucrt" // static c-runtime
// link_static "libvcruntime" // static c-runtime
// link_static "libcmt" // static c-runtime startup
// link_static "libcpmt" // static c++
#else
link_dynamic "ucrt" // dynamic c-runtime
link_dynamic "vcruntime" // dynamic c-runtime
link_dynamic "msvcrt" // dynamic c-runtime startup
link_dynamic "msvcprt" // dynamic c++
link_dynamic "oldnames"
#end

header "win/structs"
header "win/abi"
header "win/x64/enum"
header "win/x64/libc-enums"
header "win/x64/libc-gen"

#end
