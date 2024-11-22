use std::time::SystemTime;

pub fn timestamp_now_ms() -> u64 {
    match SystemTime::now().duration_since(SystemTime::UNIX_EPOCH) {
        Ok(time) => time.as_millis() as u64,
        Err(_) => 0
    }
}
