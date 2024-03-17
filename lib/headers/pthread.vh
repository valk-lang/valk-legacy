
struct pthread_t {
	data: uint
}

//enum CONST {
	//mutex_size: 64, // It's actually smaller, but the c code is just a mess, just take 64 bytes
//}

fn pthread_create(thread: pthread_t, attr: ?ptr, entry: ptr, data: ?ptr) i32;

fn pthread_mutex_init(mutex: ptr, attr: ?ptr) i32;
fn pthread_mutex_destroy(mutex: ptr) i32;
fn pthread_mutex_lock(mutex: ptr) i32;
fn pthread_mutex_unlock(mutex: ptr) i32;
