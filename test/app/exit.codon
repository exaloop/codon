import sys

if len(sys.argv) == 1:
	sys.exit(42)
elif sys.argv[1] == "message":
	sys.exit(sys.argv[2])
elif sys.argv[1] == "raise":
	raise SystemExit(sys.argv[2])
elif sys.argv[1] == "default":
	raise SystemExit()
elif sys.argv[1] == "integer":
	sys.exit(int(sys.argv[2]))
elif sys.argv[1] == "caught":
	try:
		sys.exit(sys.argv[2])
	except SystemExit:
		pass
