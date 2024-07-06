
cstruct libc_timespec {
    tv_sec: int
    tv_nsec: int
}

cstruct libc_stat {
    st_dev: uint
    st_ino: uint
    st_nlink: uint
    st_mode: u32
    st_uid: u32
    st_gid: u32
    __pad0: i32
    st_rdev: uint
    st_size: int
    st_blksize: int
    st_blocks: int
    st_atim: inline libc_timespec
    st_mtim: inline libc_timespec
    st_ctim: inline libc_timespec
    __glibc_reserved: inline [int, 3]
}

cstruct libc_anon_struct_1 {
    __val: inline [uint, 16]
}

cstruct libc_jmp_buf {
    __jmpbuf: inline [int, 8]
    __mask_was_saved: i32
    __saved_mask: inline libc_anon_struct_1
}

cstruct libc_dirent {
    d_ino: uint
    d_off: int
    d_reclen: u16
    d_type: u8
    d_name: inline [i8, 256]
}

cstruct libc_pollfd {
    fd: i32
    events: i16
    revents: i16
}

cstruct libc_timeval {
    tv_sec: int
    tv_usec: int
}

cstruct libc_sockaddr {
    sa_family: u16
    sa_data: inline [i8, 14]
}

cstruct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: u32
    ai_addr: libc_sockaddr
    ai_canonname: cstring
    ai_next: libc_addrinfo
}

cstruct libc_timezone {
    tz_minuteswest: i32
    tz_dsttime: i32
}

cstruct libc_epoll_data {
    ptr: ptr
    fd: i32
    u32: u32
    u64: uint
}

cstruct libc_epoll_event {
    events: u32
    data: inline libc_epoll_data
}

