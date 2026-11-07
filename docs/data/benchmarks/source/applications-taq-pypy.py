# Parses TAQ file and performs volume peak detection
from sys import argv
from time import time
from statistics import mean, stdev

# https://stackoverflow.com/questions/22583391/peak-signal-detection-in-realtime-timeseries-data
def find_peaks(y):
    lag = 100
    threshold = 10.0
    influence = 0.5

    t = len(y)
    signals = [0. for _ in range(t)]

    if t <= lag:
        return signals

    filtered_y = [y[i] if i < lag else 0. for i in range(t)]
    avg_filter = [0. for _ in range(t)]
    std_filter = [0. for _ in range(t)]
    avg_filter[lag] = mean(y[:lag])
    std_filter[lag] = stdev(y[:lag])

    for i in range(lag, t):
        if abs(y[i] - avg_filter[i-1]) > threshold * std_filter[i-1]:
            signals[i] = +1 if y[i] > avg_filter[i-1] else -1
            filtered_y[i] = influence*y[i] + (1 - influence)*filtered_y[i-1]
        else:
            signals[i] = 0
            filtered_y[i] = y[i]

        avg_filter[i] = mean(filtered_y[i-lag:i])
        std_filter[i] = stdev(filtered_y[i-lag:i])

    return signals

def process_data(series):
    grouped = {}
    for bucket, volume in series:
        grouped[bucket] = grouped.get(bucket, 0) + volume

    y = [float(t[1]) for t in sorted(grouped.items())]
    return y, find_peaks(y)

BUCKET_SIZE = 1_000_000_000
t0 = time()

data = {}
with open(argv[1]) as f:
    header = True

    for line in f:
        if header:
            header = False
            continue

        x = line.split('|')
        if x[0] == 'END' or x[4] == 'ENDP':
            continue

        timestamp = int(x[0])
        symbol = x[2]
        volume = int(x[4])

        series = data.setdefault(symbol, [])
        series.append((timestamp // BUCKET_SIZE, volume))

for symbol, series in data.items():
    y, signals = process_data(series)
    print(symbol, sum(signals))

t1 = time()
print(t1 - t0)
