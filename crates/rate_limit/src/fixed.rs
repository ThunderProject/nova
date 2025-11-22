use chrono::{DateTime, Duration, Utc};

pub struct RateLimiter {
    capacity: i64,
    tokens: i64,
    refill_interval: Duration,
    last_refill: DateTime<Utc>,
}

impl Default for RateLimiter {
    fn default() -> Self {
        RateLimiter::new(100, Duration::hours(1))
    }
}

impl RateLimiter {
    pub fn new(capacity: i64, refill_interval: Duration) -> Self {
        Self {
            capacity,
            tokens: capacity,
            refill_interval,
            last_refill: Utc::now(),
        }
    }

    pub fn try_acquire(&mut self) -> bool {
        let now = Utc::now();
        let elapsed = now - self.last_refill;

        self.refill(elapsed);
        self.last_refill = now;

        match self.tokens >= 1 {
            true => {
                self.tokens -= 1;
                true
            }
            _ => false,
        }
    }

    fn refill(&mut self, elapsed: Duration) {
        if elapsed >= self.refill_interval {
            self.tokens = self.capacity
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn awllows_up_to_capacity() {
        let mut rate_limiter = RateLimiter::new(3, Duration::seconds(10));

        // rate limit not reacheed so these should succeed
        assert!(rate_limiter.try_acquire());
        assert!(rate_limiter.try_acquire());
        assert!(rate_limiter.try_acquire());

        // rate limit reached so this one should fail
        assert!(!rate_limiter.try_acquire());
    }

    #[test]
    fn denies_when_empty() {
        let mut rate_limiter = RateLimiter::new(1, Duration::seconds(5));

        assert!(rate_limiter.try_acquire());
        assert!(!rate_limiter.try_acquire());
    }

    #[test]
    fn refills_after_interval_passed() {
        let mut rate_limiter = RateLimiter::new(2, Duration::seconds(5));

        // use all tokens
        assert!(rate_limiter.try_acquire());
        assert!(rate_limiter.try_acquire());
        assert!(!rate_limiter.try_acquire());

        // Move time by 6 seconds should cause a reset since the refill count is 5 seconds
        rate_limiter.last_refill = rate_limiter.last_refill - Duration::seconds(6);

        assert!(rate_limiter.try_acquire());
        assert!(rate_limiter.try_acquire());
        assert!(!rate_limiter.try_acquire());
    }

    #[test]
    fn does_not_refill_too_early() {
        let mut rate_limiter = RateLimiter::new(2, Duration::seconds(5));

        // use all tokens
        assert!(rate_limiter.try_acquire());
        assert!(rate_limiter.try_acquire());
        assert!(!rate_limiter.try_acquire());

        // Move time by 3 seconds should not be enough for reset since the refill count is 5 seconds
        rate_limiter.last_refill = rate_limiter.last_refill - Duration::seconds(3);

        // should still fail, bucket not refilled
        assert!(!rate_limiter.try_acquire());
    }

    #[test]
    fn check_default_impl_works() {
        let mut rate_limiter = RateLimiter::default();

        for _ in 0..100 {
            assert!(rate_limiter.try_acquire());
        }

        assert!(!rate_limiter.try_acquire());
    }
}
