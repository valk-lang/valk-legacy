
cstruct libc_timespec {
    tv_sec: i64
    tv_nsec: i64
}

cstruct libc_timeval {
    tv_sec: i64
    tv_usec: i32
}

cstruct libc_sockaddr {
    sa_len: i8
    sa_family: i8
    sa_data: inline [i8, 14]
}

cstruct libc_pollfd {
    fd: i32
    events: i16
    revents: i16
}

cstruct libc_stat {
    st_dev: i32
    st_ino: i16
    st_mode: i16
    st_nlink: i64
    st_uid: i32
    st_gid: i32
    st_rdev: i32
    st_atime: inline libc_timespec
    st_mtime: inline libc_timespec
    st_xtime: inline libc_timespec
    st_ctime: inline libc_timespec
    st_size: i64
    st_blocks: i64
    st_blksize: i32
    st_flags: i32
    st_gen: i32
    st_lspare: i32
    __unused: inline [i64, 2]
}

cstruct libc_dirent {
    unknown1: i64
    unknown2: i64
    unknown3: i16
    d_reclen: i16
    d_type: i8
    d_name: inline [i8, 1024]
}

cstruct libc_timezone {
    tz_minuteswest: i32
    tz_dsttime: i32
}

cstruct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: i32
    ai_canonname: ptr
    ai_addr: ptr
    ai_next: ptr
}

cstruct libc_jmp_buf {
    prop_0: inline [i32, 48]
}
