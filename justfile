set shell := ["bash", "-eu", "-o", "pipefail", "-c"]
set positional-arguments

root := justfile_directory()
packages := root / "packages"
build_tool := packages / "build.py"

# Show available commands.
default:
    @just --list

# Build Nova.
#
# Arguments are order-independent:
#   just build
#   just build all release
#   just build release all
#   just build debug tests
#   just build di release
#   just build release di tests
#   just build fetch release
#   just build run release
#
# Known arguments:
#   debug, release, all, tests, fetch, run/renderer
#
# Any other single argument is treated as a package name.
build *args:
    #!/usr/bin/env bash
    set -euo pipefail

    cd "{{packages}}"

    flags=()
    package=""

    for arg in {{args}}; do
        case "$arg" in
            debug)
                flags+=("--debug")
                ;;
            release)
                flags+=("--release")
                ;;
            all)
                flags+=("--all")
                ;;
            test|tests)
                flags+=("--tests")
                ;;
            fetch)
                flags+=("--fetch")
                ;;
            run|renderer|run-renderer)
                flags+=("--run-renderer")
                ;;
            *)
                if [[ -n "$package" ]]; then
                    echo "error: multiple package names: '$package' and '$arg'" >&2
                    exit 2
                fi

                package="$arg"
                ;;
        esac
    done

    if [[ -n "$package" ]]; then
        flags+=("--package" "$package")
    fi

    exec python3 "{{build_tool}}" "${flags[@]}"

# Build and run nova_renderer.
run *args:
    @just build run {{args}}

# Build and run tests.
#
# Examples:
#   just test
#   just test "Scoped profiler"
#   just test "Scoped profiler" output
#   just test output
#   just test debug "Scoped profiler" output
#
# Arguments are order-independent:
#   debug           Use Debug build.
#   release         Use Release build (default).
#   output          Show output from successful tests.
#   verbose, -v     Alias for output.
#
# Any other argument is treated as the CTest test-name regex.
test *args:
    #!/usr/bin/env bash
    set -euo pipefail

    config="Release"
    pattern=""
    verbose=false

    for arg in "$@"; do
        case "$arg" in
            debug)
                config="Debug"
                ;;
            release)
                config="Release"
                ;;
            output|verbose|-v)
                verbose=true
                ;;
            *)
                if [[ -n "$pattern" ]]; then
                    echo "error: multiple test patterns: '$pattern' and '$arg'" >&2
                    exit 2
                fi

                pattern="$arg"
                ;;
        esac
    done

    cd "{{packages}}"

    build_dir="build/${config}/cmake"

    cmake --build "$build_dir"

    ctest_args=(
        --test-dir "$build_dir"
        --output-on-failure
    )

    if [[ -n "$pattern" ]]; then
        ctest_args+=(
            --tests-regex "$pattern"
        )
    fi

    if [[ "$verbose" == true ]]; then
        ctest_args+=("--verbose")
    fi

    exec ctest "${ctest_args[@]}"

# Run Conan fetch, then configure/build.
fetch *args:
    @just build fetch {{args}}

# Build one package.
pkg name *args:
    @just build {{name}} {{args}}

# Open the Nova log in lnav.
log:
    @log="${XDG_STATE_HOME:-$HOME/.local/state}/nova/nova.log"; \
    if [[ ! -f "$log" ]]; then \
        echo "error: Nova log not found: $log" >&2; \
        exit 1; \
    fi; \
    exec lnav "$log"

