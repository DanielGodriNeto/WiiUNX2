"""Generate distinct, synthetic UI cues without using reference recordings."""
import argparse
import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 48000


def synthesize_click(duration, resonances, level, seed, noise_decay):
    count = round(duration * SAMPLE_RATE)
    samples = bytearray()
    phases = [0.0] * len(resonances)
    noise = random.Random(seed)
    for index in range(count):
        elapsed = index / SAMPLE_RATE
        value = noise.uniform(-1.0, 1.0) * math.exp(-elapsed / noise_decay)
        for resonance_index, (frequency, weight, decay) in enumerate(resonances):
            phases[resonance_index] += 2.0 * math.pi * frequency / SAMPLE_RATE
            value += weight * math.sin(phases[resonance_index]) * math.exp(-elapsed / decay)
        sample = max(-1.0, min(1.0, value * level))
        samples.extend(struct.pack("<h", round(sample * 32767)))
    return bytes(samples)


def make_cues():
    return {
        "Select.wav": synthesize_click(
            0.014, [(2450, 0.34, 0.0018), (3870, 0.20, 0.0011)],
            0.46, 19, 0.00075),
        "Klick.wav": synthesize_click(
            0.032, [(1180, 0.38, 0.0055), (2630, 0.22, 0.0030)],
            0.48, 29, 0.0011),
        "Enter & Back.wav": synthesize_click(
            0.019, [(1760, 0.36, 0.0030), (3210, 0.18, 0.0017)],
            0.47, 41, 0.00085),
    }


def write_wave(path, samples):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(samples)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path,
                        default=Path(__file__).resolve().parents[1] / "build" / "original-ui-cues")
    output = parser.parse_args().output_dir
    for name, samples in make_cues().items():
        write_wave(output / name, samples)
        print(f"Generated {output / name} ({len(samples)} bytes PCM)")


if __name__ == "__main__":
    main()
