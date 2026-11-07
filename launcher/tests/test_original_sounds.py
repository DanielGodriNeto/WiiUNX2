import runpy
import struct
import unittest
from pathlib import Path

generator = runpy.run_path(
    str(Path(__file__).resolve().parents[1] / "tools" / "Generate-Original-Sounds.py"))
SAMPLE_RATE = generator["SAMPLE_RATE"]
make_cues = generator["make_cues"]


class OriginalSoundTests(unittest.TestCase):
    def test_cues_are_deterministic_and_distinct(self):
        first = make_cues()
        second = make_cues()
        self.assertEqual(first, second)
        self.assertEqual(set(first), {"Select.wav", "Klick.wav", "Enter & Back.wav"})
        self.assertEqual(len(set(first.values())), len(first))

    def test_cues_are_short_percussive_pcm_clicks(self):
        expected_durations = {
            "Select.wav": 0.014,
            "Klick.wav": 0.032,
            "Enter & Back.wav": 0.019,
        }
        for name, samples in make_cues().items():
            with self.subTest(name=name):
                self.assertEqual(len(samples), round(expected_durations[name] * SAMPLE_RATE) * 2)
                values = [sample[0] for sample in struct.iter_unpack("<h", samples)]
                self.assertTrue(any(values))
                self.assertGreater(max(values), 0)
                self.assertLess(min(values), 0)
                peak = max(abs(value) for value in values)
                self.assertGreater(peak, 15000)
                self.assertLess(peak, 30000)
                early_rms = sum(value * value for value in values[:SAMPLE_RATE // 200]) ** 0.5
                late = values[SAMPLE_RATE // 1000:]
                late_rms = (sum(value * value for value in late) / len(late)) ** 0.5
                self.assertGreater(early_rms / (len(values[:SAMPLE_RATE // 200]) ** 0.5),
                                   late_rms * 1.5)


if __name__ == "__main__":
    unittest.main()
