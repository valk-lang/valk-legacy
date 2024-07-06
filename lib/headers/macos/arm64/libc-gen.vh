
cstruct libc_timespec {
    tv_sec: int
    tv_nsec: int
}

cstruct libc_stat {
    st_dev: i32
    st_mode: u16
    st_nlink: u16
    st_ino: uint
    st_uid: u32
    st_gid: u32
    st_rdev: i32
    st_atimespec: inline libc_timespec
    st_mtimespec: inline libc_timespec
    st_ctimespec: inline libc_timespec
    st_birthtimespec: inline libc_timespec
    st_size: int
    st_blocks: int
    st_blksize: i32
    st_flags: u32
    st_gen: u32
    st_lspare: i32
    st_qspare: inline [int, 2]
}

cstruct libc_dirent {
    d_ino: uint
    d_seekoff: uint
    d_reclen: u16
    d_namlen: u16
    d_type: u8
    d_name: inline [i8, 1024]
}

cstruct libc_timeval {
    tv_sec: int
    tv_usec: i32
}

cstruct libc_pollfd {
    fd: i32
    events: i16
    revents: i16
}

cstruct libc_sockaddr {
    sa_len: u8
    sa_family: u8
    sa_data: inline [i8, 14]
}

cstruct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: u32
    ai_canonname: cstring
    ai_addr: libc_sockaddr
    ai_next: libc_addrinfo
}

cstruct libc_timezone {
    tz_minuteswest: i32
    tz_dsttime: i32
}

