"""Generate distinct, synthetic UI cues without using reference recordings."""
import argparse
import math
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 48000


def synthesize(duration, start_hz, end_hz, partials, level, decay):
    count = round(duration * SAMPLE_RATE)
    samples = bytearray()
    phase = 0.0
    normalization = sum(abs(weight) for _, weight, _ in partials)
    for index in range(count):
        progress = index / max(1, count - 1)
        frequency = start_hz + (end_hz - start_hz) * progress
        phase += 2.0 * math.pi * frequency / SAMPLE_RATE
        attack = min(1.0, index / (SAMPLE_RATE * 0.003))
        release = min(1.0, (count - 1 - index) / (SAMPLE_RATE * 0.018))
        envelope = level * attack * release * math.exp(-decay * progress)
        value = sum(weight * math.sin(phase * harmonic + offset)
                    for harmonic, weight, offset in partials) / normalization
        sample = max(-1.0, min(1.0, value * envelope))
        samples.extend(struct.pack("<h", round(sample * 32767)))
    return bytes(samples)


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
    cues = {
        "Select.wav": synthesize(0.055, 465, 575, [(1, 1.0, 0.0)], 0.18, 1.2),
        "Klick.wav": synthesize(0.082, 735, 865,
                                [(1, 1.0, 0.0), (2, 0.12, 0.3)], 0.2, 1.5),
        "Enter & Back.wav": synthesize(0.105, 610, 405,
                                       [(1, 1.0, 0.0), (2, 0.16, -0.2)], 0.18, 1.1),
    }
    for name, samples in cues.items():
        write_wave(output / name, samples)
        print(f"Generated {output / name} ({len(samples)} bytes PCM)")


if __name__ == "__main__":
    main()
