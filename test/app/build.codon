counter = 0

@export
def counter_address() -> Ptr[int]:
	return __ptr__(counter)

@noinline
def check_counter(expected: int):
	if counter != expected:
		raise ValueError("counter mismatch")

counter_address()[0] = 7
assert counter == 7
try:
	check_counter(0)
	assert False
except ValueError as error:
	assert str(error) == "counter mismatch"
print('hello')
