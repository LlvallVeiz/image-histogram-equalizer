#!/usr/bin/env bash
# run_tests.sh -- Build and run the unit test suite, then smoke-test the CLI.
set -euo pipefail
cd "$(dirname "$0")/.."

make run_tests
./run_tests

# Build the CLI binary before the smoke tests.
make
./ihe --version
./ihe he examples/low_contrast.pgm /tmp/ihe_smoke_he.pgm
./ihe clahe examples/dark.pgm /tmp/ihe_smoke_clahe.pgm --tiles 8x8 --clip 2.0
./ihe match examples/gradient.pgm examples/bimodal.pgm /tmp/ihe_smoke_match.pgm
./ihe match examples/gradient.pgm /tmp/ihe_smoke_match_u.pgm --target uniform
./ihe stats examples/bimodal.pgm
./ihe compare examples/low_contrast.pgm /tmp/ihe_smoke_he.pgm
./ihe he --batch examples --out-dir /tmp/ihe_smoke_batch
echo "CLI smoke tests passed."
