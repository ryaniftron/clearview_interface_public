
band_to_frequency_table = {  
    'R1': 5658,
    'R2': 5695,
    'R3': 5732,
    'R4': 5769,
    'R5': 5806,
    'R6': 5843,
    'R7': 5880,
    'R8': 5917,
    'F1': 5740,
    'F2': 5760,
    'F3': 5780,
    'F4': 5800,
    'F5': 5820,
    'F6': 5840,
    'F7': 5860,
    'F8': 5880,
    'E1': 5705,
    'E2': 5685,
    'E3': 5665,
    'E4': 5645,
    'E5': 5885,
    'E6': 5905,
    'E7': 5925,
    'E8': 5945,
    'B1': 5733,
    'B2': 5752,
    'B3': 5771,
    'B4': 5790,
    'B5': 5809,
    'B6': 5828,
    'B7': 5847,
    'B8': 5866,
    'A1': 5865,
    'A2': 5845,
    'A3': 5825,
    'A4': 5805,
    'A5': 5785,
    'A6': 5765,
    'A7': 5745,
    'A8': 5725,
    'L1': 5362,
    'L2': 5399,
    'L3': 5436,
    'L4': 5473,
    'L5': 5510,
    'L6': 5547,
    'L7': 5584,
    'L8': 5621,
    'U0': 5300,
    'U1': 5325,
    'U2': 5348,
    'U3': 5366,
    'U4': 5384,
    'U5': 5402,
    'U6': 5420,
    'U7': 5438,
    'U8': 5456,
    'U9': 5985,
    'D1': 5660,
    'D2': 5695,
    'D3': 5735,
    'D4': 5770,
    'D5': 5805,
    'D6': 5878,
    'D7': 5914,
    'D8': 5839,
}

frequency_to_band_table = {}
for b in band_to_frequency_table:
    f = band_to_frequency_table[b]
    frequency_to_band_table[f]=b


def frequency_to_band(frequency):
    try:
        return frequency_to_band_table[frequency]
    except KeyError:
        return None

def band_to_frequency(band):
    try:
        return band_to_frequency_table[band]
    except KeyError:
        return None

