#!/bin/bash -l

export arg=$1
export testdir=$(dirname $0)
export codon="$arg/codon"

# argv test
[ "$($codon run "$testdir/argv.codon" aa bb cc)" == "aa,bb,cc" ] || exit 1

# build test
$codon build -release -o "$arg/test_binary" "$testdir/build.codon"
[ "$($arg/test_binary)" == "hello" ] || exit 2

"$codon" build -release -linker-flags=--pie -o "$arg/test_pie" "$testdir/build.codon" || exit 2
[[ "$("$arg/test_pie")" == "hello" ]] || exit 2

if [[ "$(uname -s)" == "Linux" ]]; then
	check_executable() {
		local expected_type=$1 optimization=$2
		shift 2
		"$codon" build "$optimization" "$@" -o "$arg/test_pie" "$testdir/build.codon" || exit 2
		[[ "$("$arg/test_pie")" == "hello" ]] || exit 2
		readelf -hW "$arg/test_pie" | grep -Eq "Type: +$expected_type " || exit 2
		if [[ "$expected_type" == "DYN" ]]; then
			readelf -dW "$arg/test_pie" | grep -Eq 'FLAGS_1.*PIE' || exit 2
			if readelf -dW "$arg/test_pie" | grep -q TEXTREL; then
				exit 2
			fi
		fi
	}
	check_executable EXEC -release
	check_executable EXEC -release --relocation-model=pic
	check_executable DYN -debug --pie
	check_executable DYN -release --pie
	check_executable DYN -release --pie --relocation-model=static
	check_executable DYN -release -linker-flags=-pie
	check_executable DYN -release --relocation-model=pic -linker-flags=-pie
	check_executable DYN -release --relocation-model=static -linker-flags=-pie
	check_executable DYN -release -linker-flags=--pie
	check_executable DYN -release --relocation-model=static -linker-flags=--pie
	check_executable EXEC -release -linker-flags='-pie -no-pie'
	check_executable DYN -release -linker-flags='-no-pie -pie'
	check_executable EXEC -release -linker-flags='--pie -no-pie'
	check_executable DYN -release -linker-flags='-no-pie --pie'
	check_executable DYN -release --pie -linker-flags=-no-pie

	if [[ "$(uname -m)" == "x86_64" ]]; then
		"$codon" build -release -obj --relocation-model=static \
			-o "$arg/test_pie_static.o" "$testdir/build.codon" || exit 2
		if g++ -pie "$arg/test_pie_static.o" -L"$arg" -Wl,-rpath,"$arg" \
			-lcodonrt -lomp -lpthread -ldl -lz -lm -lc -o "$arg/test_pie_static" \
			> "$arg/pie_stdout" 2> "$arg/pie_stderr"; then
			exit 2
		fi
		grep -Eq 'relocation R_X86_64_32S?.*PIE' "$arg/pie_stderr" || exit 2
	fi

	if "$codon" build --pie -obj -o "$arg/test_pie.o" "$testdir/build.codon" \
		> "$arg/pie_stdout" 2> "$arg/pie_stderr"; then
		exit 2
	fi
	grep -q -- '--pie requires executable output' "$arg/pie_stderr" || exit 2
	for flag in -pie --pie; do
		if "$codon" build -lib "-linker-flags=$flag" -o "$arg/test_pie.so" "$testdir/build.codon" \
			> "$arg/pie_stdout" 2> "$arg/pie_stderr"; then
			exit 2
		fi
		grep -q 'PIE is only supported for executables' "$arg/pie_stderr" || exit 2
	done
fi

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
