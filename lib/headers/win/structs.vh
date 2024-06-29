
cstruct libc_jmp_buf {
    data: inline [ptr, 5]
}

cstruct libc_timezone {
    tz_minuteswest: i32 // Minutes west of GMT
    tz_dsttime: i32 // Nonzero if DST is ever in effect
}

cstruct libc_addrinfo_fix {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: i64
    ai_canonname: ptr
    ai_addr: ptr
    ai_next: ptr
}
