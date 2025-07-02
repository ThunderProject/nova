use std::any::{Any, TypeId};
use std::sync::Arc;
use dashmap::DashMap;
use once_cell::sync::Lazy;


type Factory = dyn Fn() -> Box<dyn Any + Send + Sync> + Send + Sync;

pub struct Container {
    factories: DashMap<TypeId, Arc<Factory>>,
    instances: DashMap<TypeId, Arc<dyn Any + Send + Sync>>,
}

impl Container {
    pub fn new() -> Container {
        Self {
            factories: DashMap::new(),
            instances: DashMap::new(),
        }
    }

    pub fn register<T: 'static + Send + Sync>(&self, factory: impl Fn() -> T + Send + Sync + 'static) {
        let wrapper = move || -> Box<dyn Any + Send + Sync> { Box::new(factory()) };
        self.factories.insert(TypeId::of::<T>(), Arc::new(wrapper));
    }

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
    struct DummyUnique {
        id: usize,
    }

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
        ioc().register(|| DummyUnique { id: 999 });

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

    #[test]
    #[should_panic(expected = "Factory returned wrong type")]
    fn test_type_mismatch_panics() {
        use std::any::Any;

        struct BadFactory;
        impl BadFactory {
            fn register_broken() {
                let type_id = std::any::TypeId::of::<Dummy>();
                let factory: Arc<dyn Fn() -> Box<dyn Any + Send + Sync> + Send + Sync> =
                    Arc::new(|| Box::new(123usize) as Box<dyn Any + Send + Sync>);
                ioc().factories.insert(type_id, factory);
            }
        }

        BadFactory::register_broken();
        let _ = ioc().resolve::<Dummy>();
    }
}