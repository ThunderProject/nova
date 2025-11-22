use std::any::{Any, TypeId};
use std::sync::Arc;
use dashmap::DashMap;
use once_cell::sync::Lazy;

/// Type alias for a thread-safe factory function that produces a boxed `Any` value.
/// - Each registered type gets a corresponding factory.
type Factory = dyn Fn() -> Box<dyn Any + Send + Sync> + Send + Sync;

/// A thread-safe inversion of control (IoC) container.
/// - Supports singleton registration and resolution of services.
/// - Uses `TypeId` to map Rust types to their instances/factories.
/// - Fully Thread-safe via `DashMap` and `Arc`.
pub struct Container {
    factories: DashMap<TypeId, Arc<Factory>>,
    instances: DashMap<TypeId, Arc<dyn Any + Send + Sync>>,
}

impl Default for Container {
    fn default() -> Self {
        Self::new()
    }
}

impl Container {
    pub fn new() -> Container {
        Self {
            factories: DashMap::new(),
            instances: DashMap::new(),
        }
    }

    /// Registers a factory function for type `T` in the container.
    ///
    /// - `T` must be `'static + Send + Sync`.
    /// - The factory will only be invoked the first time `T` is resolved (singleton semantics).
    pub fn register<T: 'static + Send + Sync>(&self, factory: impl Fn() -> T + Send + Sync + 'static) {
        let wrapper = move || -> Box<dyn Any + Send + Sync> { Box::new(factory()) };
        self.factories.insert(TypeId::of::<T>(), Arc::new(wrapper));
    }

    /// Resolves an instance of type `T`.
    ///
    /// - Returns a shared `Arc<T>` singleton.
    /// - If `T` was not previously resolved, the registered factory is invoked.
    /// - Panics if:
    ///     * `T` has not been registered (should not happen unless misused).
    ///     * The factory returns an unexpected type (should not happen unless misused).
    pub fn resolve<T: 'static + Send + Sync>(&self) -> Arc<T> {
        let type_id = TypeId::of::<T>();

        match self.instances.get(&type_id) {
            Some(instance) => {
                instance.downcast_ref::<Arc<T>>()
                    .expect("Type mismatch in singleton container")
                    .clone()
            }
            None => {
                let factory = self
                    .factories
                    .get(&type_id)
                    .expect("Type not registered in singleton container");

                let boxed = factory();

                let typed = boxed
                    .downcast::<T>()
                    .unwrap_or_else(|_| panic!("Factory returned wrong type for {:?}", std::any::type_name::<T>()));

                let arc: Arc<T> = Arc::new(*typed);

                self.instances.insert(type_id, Arc::new(arc.clone()) as Arc<dyn Any + Send + Sync>);

                arc
            }
        }
    }
}

static INSTANCE: Lazy<Container> = Lazy::new(Container::new);

pub fn ioc() -> &'static Container {
    &INSTANCE
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::thread;
    use crate::ioc;

    #[derive(Debug)]
    struct Dummy {
        id: usize,
    }

    #[derive(Debug)]
    struct DummyUnique {}

    #[derive(Debug)]
    struct ThreadSafeDummy {
        id: usize,
    }

    #[test]
    fn test_register_and_resolve() {
        ioc::singleton::ioc().register(|| Dummy { id: 123 });

        let dummy = ioc().resolve::<Dummy>();
        assert_eq!(dummy.id, 123);
    }

    #[test]
    fn test_singleton_uniqueness() {
        ioc().register(|| DummyUnique { });

        let a = ioc().resolve::<DummyUnique>();
        let b = ioc().resolve::<DummyUnique>();

        assert!(Arc::ptr_eq(&a, &b));
    }

    #[test]
    fn test_thread_safety() {
        static COUNTER: AtomicUsize = AtomicUsize::new(0);

        ioc().register(|| {
            COUNTER.fetch_add(1, Ordering::SeqCst);
            ThreadSafeDummy { id: 42 }
        });

        let handles: Vec<_> = (0..10)
            .map(|_| {
                thread::spawn(|| {
                    let instance = ioc().resolve::<ThreadSafeDummy>();
                    assert_eq!(instance.id, 42);
                })
            })
            .collect();

        for handle in handles {
            handle.join().unwrap();
        }

        assert_eq!(COUNTER.load(Ordering::SeqCst), 1);
    }

    #[test]
    #[should_panic(expected = "Type not registered")]
    fn test_resolve_unregistered_type_panics() {
        #[derive(Debug)]
        struct NotRegistered;

        let _ = ioc().resolve::<NotRegistered>();
    }
}