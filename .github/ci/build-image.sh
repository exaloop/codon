#!/bin/sh
set -eu

ARCH=${1:?Usage: build-image.sh ARCH}
cd "$(dirname "$0")"

attempt=1
max_attempts=5
delay=15
while :; do
  if docker build -t local -f "Dockerfile.${ARCH}" .; then
    exit 0
  else
    exit_code=$?
  fi
  if [ "$attempt" -ge "$max_attempts" ]; then
    exit "$exit_code"
  fi
  printf 'Docker image build failed (attempt %s/%s, exit %s); retrying in %ss\n' \
    "$attempt" "$max_attempts" "$exit_code" "$delay" >&2
  sleep "$delay"
  attempt=$((attempt + 1))
  delay=$((delay * 2))
done
