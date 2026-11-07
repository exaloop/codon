import numpy as np
import numba as nb


# Adapted from https://gist.github.com/oysstu/68072c44c02879a2abf94ef350d1c7c6
@nb.jit(nopython=True, parallel=False, fastmath=True)
def crc16(data, poly=0x8408):
    '''
    CRC-16-CCITT Algorithm
    '''
    crc = 0xFFFF
    for b in data:
        cur_byte = 0xFF & b
        for _ in range(0, 8):
            if (crc & 0x0001) ^ (cur_byte & 0x0001):
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1
            cur_byte >>= 1
    crc = (~crc & 0xFFFF)
    crc = (crc << 8) | ((crc >> 8) & 0xFF)

    return crc & 0xFFFF
