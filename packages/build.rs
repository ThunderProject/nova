use std::path::Path;
use std::process::{Command, ExitStatus};
use clap::{ArgGroup, Parser};

#[derive(Parser, Debug)]
#[command(
    name = "Nova Build Tool",
    version = "0.1",
    author = "Nova Team",
    about = "Build tool for Nova packages",
    long_about = None,
)]
#[command(group(
    ArgGroup::new("mode")
        .required(true)
        .args(["debug", "release"])
))]
#[command(group(
    ArgGroup::new("scope")
        .required(true)
        .args(["all", "package"])
))]
struct Args {
    #[arg(long)]
    debug: bool,

    #[arg(long)]
    release: bool,

    #[arg(long)]
    all: bool,

    #[arg(long)]
    package: Option<String>,

    #[arg(long)]
    tests: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Config {
    Debug,
    Release,
}

impl Config {
    fn as_str(self) -> &'static str {
        match self {
            Self::Debug => "Debug",
            Self::Release => "Release",
        }
    }

    fn preset(self) -> &'static str {
        match self {
            Self::Debug => "debug",
            Self::Release => "release",
        }
    }

    fn build_preset(self) -> &'static str {
        match self {
            Self::Debug => "build-debug",
            Self::Release => "build-release",
        }
    }

    fn test_preset(self) -> &'static str {
        match self {
            Self::Debug => "test-debug",
            Self::Release => "test-release",
        }
    }
}

fn main() {
    if let Err(error) = run() {
        eprintln!("error: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    ensure_workspace_root()?;

    let cli = Args::parse();

    let config = match (cli.debug, cli.release) {
        (true, false) => Config::Debug,
        (false, true) => Config::Release,
        _ => return Err("choose exactly one of --debug or --release".to_string()),
    };

    let target = if cli.all {
        None
    } else {
        let package = cli
            .package
            .as_deref()
            .ok_or_else(|| "missing --package <name> or --all".to_string())?;

        Some(package_to_target(package)?)
    };

    run_command(
        "conan",
        &[
            "install",
            ".",
            "--build=missing",
            "-s",
            &format!("build_type={}", config.as_str()),
        ],
    )?;

    let cmake_build_dir = format!("build/{}/cmake", config.as_str());
    let toolchain_file = format!(
        "build/{}/generators/conan_toolchain.cmake",
        config.as_str()
    );

    run_command(
        "cmake",
        &[
            "-S",
            ".",
            "-B",
            &cmake_build_dir,
            "-G",
            "Ninja",
            &format!("-DCMAKE_TOOLCHAIN_FILE={toolchain_file}"),
            &format!("-DCMAKE_BUILD_TYPE={}", config.as_str()),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ],
    )?;

    match target {
        None => {
            run_command("cmake", &["--build", &cmake_build_dir])?;
        }
        Some(target_name) => {
            run_command(
                "cmake",
                &["--build", &cmake_build_dir, "--target", target_name],
            )?;
        }
    }

    if cli.tests {
        run_command("ctest", &["--test-dir", &cmake_build_dir, "--output-on-failure"])?;
    }

    Ok(())
}

fn ensure_workspace_root() -> Result<(), String> {
    let required = ["conanfile.py", "CMakeLists.txt"];
    for item in required {
        if !Path::new(item).exists() {
            return Err(format!(
                "run this from the packages/ workspace root; missing '{}'",
                item
            ));
        }
    }
    Ok(())
}

fn package_to_target(package: &str) -> Result<&'static str, String> {
    match package {
        "core" => Ok("yourproj_core"),
        "sync" => Ok("yourproj_sync"),
        other => Err(format!("unknown package '{other}'")),
    }
}

fn run_command(program: &str, args: &[&str]) -> Result<(), String> {
    println!("> {} {}", program, args.join(" "));

    let status: ExitStatus = Command::new(program)
        .args(args)
        .status()
        .map_err(|e| format!("failed to run '{}': {}", program, e))?;

    if !status.success() {
        return Err(format!(
            "command failed with status {}: {} {}",
            status,
            program,
            args.join(" ")
        ));
    }

    Ok(())
}
