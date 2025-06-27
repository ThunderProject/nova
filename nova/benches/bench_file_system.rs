use criterion::{criterion_group, criterion_main, BatchSize, Criterion};
use tempfile::tempdir;
use std::fs;
use std::fs::create_dir_all;
use std::path::Path;
use nova::fs::file_system::file_system::FileSystem;
fn bench_sync_clear_dir(c: &mut Criterion) {
    c.bench_function("clear_dir (nested)", |b| {
        b.iter_batched(
            || {
                let tmp = tempdir().unwrap();
                let root = tmp.path().join("work");
                fs::create_dir(&root).unwrap();

                create_nested_structure(&root, 3, 4, 10);

                (tmp, root)
            },
            |(_tmp, root)| {
                FileSystem::clear_dir(&root).unwrap();
            },
            BatchSize::SmallInput,
        );
    });
}
fn create_nested_structure(base: &Path, depth: usize, breadth: usize, files_per_dir: usize) {
    fn helper(dir: &Path, depth: usize, breadth: usize, files_per_dir: usize) {
        if depth == 0 {
            return;
        }
        
        for i in 0..files_per_dir {
            let file = dir.join(format!("file_{}.txt", i));
            fs::write(&file, b"test").unwrap();
        }
        
        for i in 0..breadth {
            let sub = dir.join(format!("subdir_{}", i));
            create_dir_all(&sub).unwrap();
            helper(&sub, depth - 1, breadth, files_per_dir);
        }
    }

    helper(base, depth, breadth, files_per_dir);
}

fn bench_par_clear_dir(c: &mut Criterion) {
    c.bench_function("clear_dir_par (nested)", |b| {
        b.iter_batched(
            || {
                let tmp = tempdir().unwrap();
                let root = tmp.path().join("work");
                fs::create_dir(&root).unwrap();

                create_nested_structure(&root, 3, 4, 10);

                (tmp, root) 
            },
            |(_tmp, root)| {
                FileSystem::clear_dir_par(&root).unwrap();
            },
            BatchSize::SmallInput,
        );
    });
}

criterion_group!(benches, bench_par_clear_dir, bench_sync_clear_dir);
criterion_main!(benches);