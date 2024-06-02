
struct libc_jmp_buf {
    data: inline [ptr, 5]
}

struct libc_timezone {
    tz_minuteswest: i32 // Minutes west of GMT
    tz_dsttime: i32 // Nonzero if DST is ever in effect
}
