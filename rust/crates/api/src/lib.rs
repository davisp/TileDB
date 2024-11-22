
#[cxx::bridge]
mod ffi {
    #[namespace = "tiledb::rust"]
    extern "Rust" {
        fn timestamp_now_ms() -> u64;
    }
}


pub fn timestamp_now_ms() -> u64 {
    tiledb_utils::timestamp_now_ms()
}
