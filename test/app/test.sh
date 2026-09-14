#!/bin/bash -l

export arg=$1
export testdir=$(dirname $0)
export codon="$arg/codon"

# argv test
[ "$($codon run "$testdir/argv.codon" aa bb cc)" == "aa,bb,cc" ] || exit 1

# build test
$codon build -release -o "$arg/test_binary" "$testdir/build.codon"
[ "$($arg/test_binary)" == "hello" ] || exit 2

# library test
$codon build -relocation-model=pic -o "$arg/libcodon_export_test.so" "$testdir/export.codon"
gcc "$testdir/test.c" -L"$arg" -Wl,-rpath,"$arg" -lcodon_export_test -o "$arg/test_binary"
[ "$($arg/test_binary)" == "abcabcabc" ] || exit 3

# exit code test
check_exit() {
	local expected_status=$1 expected_message=$2 exit_code
	shift 2
	"$@" > "$arg/exit_stdout" 2> "$arg/exit_stderr"
	exit_code=$?
	[[ $exit_code -eq $expected_status ]] || exit 4
	[[ ! -s "$arg/exit_stdout" ]] || exit 4
	printf '%s' "$expected_message" | cmp - "$arg/exit_stderr" || exit 4
}

for mode in -debug -release; do
	"$codon" build "$mode" -o "$arg/test_exit" "$testdir/exit.codon" || exit 4
	for invocation in run binary; do
		if [[ "$invocation" == run ]]; then
			command=("$codon" run "$mode" "$testdir/exit.codon")
		else
			command=("$arg/test_exit")
		fi
		check_exit 42 "" "${command[@]}"
		check_exit 0 "" "${command[@]}" default
		check_exit 0 "" "${command[@]}" integer 0
		check_exit 1 "" "${command[@]}" integer 1
		check_exit 0 "" "${command[@]}" caught "caught message"
		for method in message raise; do
			for message in "input file not found" "" $'line\nbreak' $'caf\xc3\xa9'; do
				check_exit 1 "$message"$'\n' "${command[@]}" "$method" "$message"
			done
		done
	done
done

# input test
[ "$($codon run "$testdir/input.codon" < "$testdir/input.txt")" == "input: aa bb,,cc,X" ] || exit 5
