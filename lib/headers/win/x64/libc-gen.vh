
cstruct libc_stat {
    st_dev: u32
    st_ino: u16
    st_mode: u16
    st_nlink: i16
    st_uid: i16
    st_gid: i16
    st_rdev: u32
    st_size: int
    st_atime: int
    st_mtime: int
    st_ctime: int
}

cstruct libc_FILETIME {
    dwLowDateTime: uint
    dwHighDateTime: uint
}

cstruct libc_WIN32_FIND_DATAA {
    dwFileAttributes: uint
    ftCreationTime: inline libc_FILETIME
    ftLastAccessTime: inline libc_FILETIME
    ftLastWriteTime: inline libc_FILETIME
    nFileSizeHigh: uint
    nFileSizeLow: uint
    dwReserved0: uint
    dwReserved1: uint
    cFileName: inline [i8, 260]
    cAlternateFileName: inline [i8, 14]
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
    ai_addrlen: uint
    ai_canonname: cstring
    ai_addr: libc_sockaddr
    ai_next: libc_addrinfo
}

cstruct libc_timeval {
    tv_sec: int
    tv_usec: int
}

cstruct libc_pollfd {
    fd: uint
    events: i16
    revents: i16
}

cstruct libc_timespec {
    tv_sec: int
    tv_nsec: int
}

