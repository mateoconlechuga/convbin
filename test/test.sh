#!/bin/bash
# Copyright 2017-2026 Matt "MateoConLechuga" Waltz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

set -u

total_tests=0
passed_tests=0
failed_tests=0
failed_names=()

run_test() {
    local name="$1"
    local command="$2"

    total_tests=$((total_tests + 1))
    printf '[test] %s\n' "$name"

    if bash -c "$command"; then
        passed_tests=$((passed_tests + 1))
        printf '[pass] %s\n' "$name"
    else
        failed_tests=$((failed_tests + 1))
        failed_names+=("$name")
        printf '[fail] %s\n' "$name"
    fi
}

run_test_expect_fail() {
    local name="$1"
    local command="$2"

    total_tests=$((total_tests + 1))
    printf '[test] %s\n' "$name"

    if bash -c "$command"; then
        failed_tests=$((failed_tests + 1))
        failed_names+=("$name")
        printf '[fail] %s (unexpected success)\n' "$name"
    else
        passed_tests=$((passed_tests + 1))
        printf '[pass] %s\n' "$name"
    fi
}

# Test: Convert 64k binary input to 8xp.
run_test "bin_64k_to_8xp" "../bin/convbin --iformat bin --input inputs/random_64k.bin --oformat 8xp --output THEISTE.8xp --name TESTTEST"

# Test: Convert 128k binary input to 8xp (split appvars path).
run_test "bin_128k_to_8xp_split" "../bin/convbin --iformat bin --input inputs/random_128k.bin --oformat 8xp --output THEISTE.8xp --name TESTTEST"

# Test: Build auto-extracting group from multiple 8x inputs.
run_test "8x_multi_to_8xg_auto_extract" "../bin/convbin --iformat 8x --input inputs/demo.8xp --input inputs/fileioc.8xv --input inputs/libload.8xv --oformat 8xg-auto-extract --output test.8xg.test"

# Test: Extract TI program payload to raw binary.
run_test "8x_to_bin_demo" "../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat bin --output test.bin.test"

# Test: Extract TI appvar payload to raw binary.
run_test "8x_to_bin_appvar" "../bin/convbin --iformat 8x --input inputs/fileioc.8xv --oformat bin --output test.bin.test"

# Test: Convert large binary to C source.
run_test "bin_to_c" "../bin/convbin --iformat bin --input inputs/large.bin --oformat c --output test.c.test --name TEST"

# Test: Convert large binary to assembly source.
run_test "bin_to_asm" "../bin/convbin --iformat bin --input inputs/large.bin --oformat asm --output test.asm.test --name TEST"

# Test: Convert 8x program to ICE source.
run_test "8x_to_ice" "../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat ice --output test.ice.test --name TEST"

# Test: Convert 8x program to standard 8xp.
run_test "8x_to_8xp" "../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat 8xp --output test.8xp.test --name TEST"

# Test: Convert small binary to standard 8xp.
run_test "small_bin_to_8xp" "../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xp --output test.8xp.test --name TEST"

# Test: Convert 8x program to compressed 8xp using default compressor.
run_test "8xp_compressed_default" "../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat 8xp-compressed --output test.8xp --name TEST"

# Test: Convert 8x program to compressed 8xp using zx0 compressor.
run_test "8xp_compressed_zx0" "../bin/convbin --iformat 8x --input inputs/demo.8xp -e zx0 --oformat 8xp-compressed --output test.8xp --name TEST"

# Test: Apply zx7 compression when generating C source.
run_test "compress_zx7_c" "../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.c.test --name TEST --compress zx7"

# Test: Apply zx7 compression when generating assembly source.
run_test "compress_zx7_asm" "../bin/convbin --iformat bin --input inputs/large.bin --oformat asm --output test.asm.test --name TEST --compress zx7"

# Test: Apply zx7 compression when generating ICE source.
run_test "compress_zx7_ice" "../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat ice --output test.ice.test --name TEST --compress zx7"

# Test: Validate warning/ignore behavior for compression with plain 8xp output.
run_test "compress_ignored_8xp" "../bin/convbin --iformat 8x --input inputs/libload.8xv --oformat 8xp --output test.8xp.test --name TEST --compress zx7"

# Test: Parse CSV input into C output.
run_test "csv_to_c" "../bin/convbin --iformat csv --input inputs/csv.csv --oformat c --output test.c.test --name TEST"

# Test: Parse CSV input into assembly output.
run_test "csv_to_asm" "../bin/convbin --iformat csv --input inputs/csv.csv --oformat asm --output test.asm.test --name TEST"

# Test: Parse CSV input into ICE output.
run_test "csv_to_ice" "../bin/convbin --iformat csv --input inputs/csv.csv --oformat ice --output test.ice.test --name TEST"

# Test: Parse CSV input into 8xp output.
run_test "csv_to_8xp" "../bin/convbin --iformat csv --input inputs/csv.csv --oformat 8xp --output test.8xp.test --name TEST"

# Test: Build standard zip archive from two files.
run_test "bundle_zip" "../bin/convbin --input inputs/small.bin --input inputs/large.bin --oformat zip --output test.zip.test"

# Test: Build b83 bundle from two files.
run_test "bundle_b83" "../bin/convbin --input inputs/small.bin --input inputs/large.bin --oformat b83 --output test.b83.test"

# Test: Build b84 bundle from two files.
run_test "bundle_b84" "../bin/convbin --input inputs/small.bin --input inputs/large.bin --oformat b84 --output test.b84.test"

# Test: Verify long variable names are accepted/truncated as needed.
run_test "long_var_name_c" "../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.8xp.test --name super_long_name_that_apparently_people_like_to_use_for_their_variable_names_even_though_it_is_basically_unreadable_but_hey_who_cares_the_user_gets_what_the_user_wants"

# Test: Uppercase behavior is stable when --name appears before --uppercase.
run_test "uppercase_order_name_then_upper" "../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.upper_a.c.test --name mixedCase --uppercase"

# Test: Output symbol should be uppercase in first ordering case.
run_test "uppercase_assert_a" "grep -q '^unsigned char MIXEDCASE\\[' test.upper_a.c.test"

# Test: Uppercase behavior is stable when --uppercase appears before --name.
run_test "uppercase_order_upper_then_name" "../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.upper_b.c.test --uppercase --name mixedCase"

# Test: Output symbol should be uppercase in second ordering case.
run_test "uppercase_assert_b" "grep -q '^unsigned char MIXEDCASE\\[' test.upper_b.c.test"

# Test: Invalid input format on later input should fail.
run_test_expect_fail "invalid_second_input_format" "../bin/convbin --iformat bin --input inputs/small.bin --iformat nope --input inputs/small.bin --oformat bin --output test.invalid_input.test"

# Test: Group outputs should require 8x input format.
run_test_expect_fail "group_requires_8x_inputs" "../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xg --output test.invalid_group.8xg --name TEST"

# Test: Exact 4 MiB payload should be accepted.
run_test "exact_max_size_create" "truncate -s 4194304 test.input.max.bin"

# Test: Convert exact 4 MiB payload.
run_test "exact_max_size_convert" "../bin/convbin --iformat bin --input test.input.max.bin --oformat bin --output test.output.max.bin"

# Test: Exact 4 MiB output size should match input size.
run_test "exact_max_size_assert" "[ \"\$(wc -c < test.output.max.bin)\" = '4194304' ]"

# Test: Split appvar generation should succeed.
run_test "split_appvar_generate" "../bin/convbin --iformat bin --input inputs/random_128k.bin --oformat 8xv-split --output test.split.8xv --name TEST"

# Test: First split appvar output file should exist.
run_test "split_appvar_assert_file" "[ -f test.split.0.8xv ]"

# Test: 8xp split-to-appvars should support more than 10 appvars.
run_test "8xp_split_over_10_generate" "../bin/convbin --iformat bin --input inputs/random_128k.bin --oformat 8xp --output test.split10.8xp --name TEST --maxvarsize 4096"

# Test: 8xp split should create at least an 11th appvar output.
run_test "8xp_split_over_10_assert_file" "[ -f test.split10.8xp.10.8xv ]"

# Test: 8xp at exact maxvarsize should not split into appvars.
run_test "8xp_exact_boundary_cleanup" "rm -f test.nosplit.8xp test.nosplit.8xp.*.8xv"

# Test: Prepare exact-boundary input at minimum allowed maxvarsize (4096 bytes).
run_test "8xp_exact_boundary_prepare" "truncate -s 4096 test.exact_boundary.bin"

# Test: Convert exactly maxvarsize bytes to 8xp.
run_test "8xp_exact_boundary_generate" "../bin/convbin --iformat bin --input test.exact_boundary.bin --oformat 8xp --output test.nosplit.8xp --name TEST --maxvarsize 4096"

# Test: Exact-boundary 8xp should not emit split appvar files.
run_test "8xp_exact_boundary_no_split_assert" "[ ! -e test.nosplit.8xp.0.8xv ]"

# Test: 8xp one-byte-over maxvarsize should split.
run_test "8xp_over_boundary_cleanup" "rm -f test.oversplit.8xp test.oversplit.8xp.*.8xv"

# Test: Prepare over-boundary input that exceeds payload max after 2-byte header skip (4099 bytes).
run_test "8xp_over_boundary_prepare" "truncate -s 4099 test.over_boundary.bin"

# Test: Convert 4099-byte input with 4096-byte maxvarsize to force split.
run_test "8xp_over_boundary_generate" "../bin/convbin --iformat bin --input test.over_boundary.bin --oformat 8xp --output test.oversplit.8xp --name TEST --maxvarsize 4096"

# Test: Over-boundary 8xp should emit second split appvar.
run_test "8xp_over_boundary_split_assert" "[ -f test.oversplit.8xp.1.8xv ]"

# Test: maxvarsize below minimum should fail.
run_test_expect_fail "maxvarsize_too_small" "../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xp --output test.invalid_small.8xp --name TEST --maxvarsize 1024"

# Test: maxvarsize above maximum should fail.
run_test_expect_fail "maxvarsize_too_large" "../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xp --output test.invalid_large.8xp --name TEST --maxvarsize 70000"

# Test: 8ek output with multiple inputs should fail.
run_test_expect_fail "8ek_multiple_inputs_fail" "../bin/convbin --iformat elf --input inputs/DEMO.obj --input inputs/DEMO.obj --oformat 8ek --output test.invalid_multi.8ek --name DEMO"

# Test: 8xv-split should reject requests requiring more than 99 appvars.
run_test "8xv_split_over_99_prepare" "truncate -s 500000 test.too_many.bin"

# Test: Trigger >99 appvar requirement in 8xv-split mode.
run_test_expect_fail "8xv_split_over_99_fail" "../bin/convbin --iformat bin --input test.too_many.bin --oformat 8xv-split --output test.too_many.8xv --name TEST --maxvarsize 4096"

# Test: 8xv-split should allow exactly 99 appvars.
run_test "8xv_split_exact_99_prepare" "truncate -s 405504 test.split99.bin"

# Test: Generate exactly 99 appvars in 8xv-split mode.
run_test "8xv_split_exact_99_generate" "../bin/convbin --iformat bin --input test.split99.bin --oformat 8xv-split --output test.split99.8xv --name TEST --maxvarsize 4096"

# Test: Last valid 8xv-split appvar index (.98) should exist.
run_test "8xv_split_exact_99_assert_last" "[ -f test.split99.98.8xv ]"

# Test: Out-of-range 8xv-split appvar index (.99) should not exist for 99-way split.
run_test "8xv_split_exact_99_assert_no_extra" "[ ! -e test.split99.99.8xv ]"

# Test: 8xp split path should allow exactly 99 appvars (payload excludes 2-byte token prefix).
run_test "8xp_split_exact_99_prepare" "truncate -s 405506 test.8xp99.bin"

# Test: Generate exactly 99 appvars in 8xp split mode.
run_test "8xp_split_exact_99_generate" "../bin/convbin --iformat bin --input test.8xp99.bin --oformat 8xp --output test.8xp99.8xp --name TEST --maxvarsize 4096"

# Test: Last valid 8xp split appvar index (.98) should exist.
run_test "8xp_split_exact_99_assert_last" "[ -f test.8xp99.8xp.98.8xv ]"

# Test: Out-of-range 8xp split appvar index (.99) should not exist for 99-way split.
run_test "8xp_split_exact_99_assert_no_extra" "[ ! -e test.8xp99.8xp.99.8xv ]"

# Test: 8xp split path should reject 100 appvars.
run_test "8xp_split_over_99_prepare" "truncate -s 409602 test.8xp100.bin"

# Test: 8xp split should fail when more than 99 appvars are required.
run_test_expect_fail "8xp_split_over_99_fail" "../bin/convbin --iformat bin --input test.8xp100.bin --oformat 8xp --output test.8xp100.8xp --name TEST --maxvarsize 4096"

# Test: Group mode should reject mixed per-input formats where a later input is non-8x.
run_test_expect_fail "group_later_input_not_8x_fail" "../bin/convbin --iformat 8x --input inputs/demo.8xp --iformat bin --input inputs/small.bin --oformat 8xg --output test.invalid_group_later.8xg --name TEST"

# Test: Invalid input compression on later input should fail validation.
run_test_expect_fail "invalid_second_input_compression" "../bin/convbin --iformat bin --input inputs/small.bin --icompress nope --input inputs/small.bin --oformat bin --output test.invalid_icompress.test"

# Test: Required output name should be enforced for 8xp.
run_test_expect_fail "missing_name_for_8xp" "../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xp --output test.noname.8xp"

# Test: Append mode should concatenate two outputs.
run_test "append_mode_cleanup" "rm -f test.append.bin"

# Test: First append write.
run_test "append_mode_first_write" "../bin/convbin --iformat bin --input inputs/small.bin --oformat bin --output test.append.bin --append"

# Test: Second append write.
run_test "append_mode_second_write" "../bin/convbin --iformat bin --input inputs/small.bin --oformat bin --output test.append.bin --append"

# Test: Appended file size should be double input size.
run_test "append_mode_size_assert" "[ \"\$(wc -c < test.append.bin)\" = '904' ]"

# Test: Prepare payload larger than the historical 4 MiB cap.
run_test "input_over_max_prepare" "truncate -s 4194305 test.too_big.bin"

# Test: Bin-to-bin should allow inputs larger than the historical 4 MiB cap.
run_test "input_over_max_convert" "../bin/convbin --iformat bin --input test.too_big.bin --oformat bin --output test.too_big.out"

# Test: Oversized bin-to-bin conversion should preserve exact size.
run_test "input_over_max_size_assert" "[ \"\$(wc -c < test.too_big.out)\" = '4194305' ]"

# Test: Bin-to-bin conversion should allow inputs larger than the historical 4 MiB cap.
run_test "bin_large_unbounded_prepare" "truncate -s 5242880 test.large_unbounded.bin"

# Test: Convert 5 MiB binary to binary output.
run_test "bin_large_unbounded_convert" "../bin/convbin --iformat bin --input test.large_unbounded.bin --oformat bin --output test.large_unbounded.out"

# Test: Large unbounded bin conversion should preserve exact size.
run_test "bin_large_unbounded_size_assert" "[ \"\$(wc -c < test.large_unbounded.out)\" = '5242880' ]"

# Test: Empty CSV should convert deterministically to empty binary output.
run_test "csv_empty_prepare" "printf '' > test.empty.csv"

# Test: Convert empty CSV to binary output.
run_test "csv_empty_to_bin" "../bin/convbin --iformat csv --input test.empty.csv --oformat bin --output test.empty.bin"

# Test: Empty CSV binary output should be zero bytes.
run_test "csv_empty_to_bin_size_assert" "[ \"\$(wc -c < test.empty.bin)\" = '0' ]"

echo
echo "========== Test Summary =========="
echo "Total:  $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $failed_tests"

if [ "$failed_tests" -ne 0 ]; then
    echo "Failed tests:"
    for name in "${failed_names[@]}"; do
        echo "  - $name"
    done
    exit 1
fi
