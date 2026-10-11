import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from analyze_log import (
    accel_counts_to_m_s2,
    gyro_counts_to_rad_s,
    mean_vector,
    make_report,
    read_log,
)


HEADER = [
    "timestamp_ms", "dt_s", "accel_x_raw_lsb", "gyro_x_raw_lsb",
    "accel_x_m_s2", "gyro_x_rad_s", "corrected_accel_x_m_s2",
    "velocity_x_m_s", "displacement_x_m", "displacement_y_m",
    "displacement_z_m", "calibration_valid", "state_flags",
    "missed_samples", "serial_dropped_rows", "segment_id",
]


def csv_row(timestamp, dt, displacement, flags="RUNNING_RAW", segment="0"):
    return [
        timestamp, dt, 0, 0, 0, 0, 0, 0,
        displacement, 0, 0, 1, flags, 0, 0, segment,
    ]


class AnalysisTests(unittest.TestCase):
    def write_log(self, rows):
        temporary = tempfile.NamedTemporaryFile(mode="w", newline="", encoding="utf-8",
                                                delete=False)
        self.addCleanup(Path(temporary.name).unlink, missing_ok=True)
        temporary.write("# BMI160 test metadata\n")
        writer = csv.writer(temporary)
        writer.writerow(HEADER)
        writer.writerows(rows)
        temporary.close()
        return read_log(temporary.name)

    def test_sensor_scaling_and_axis_sign(self):
        self.assertAlmostEqual(accel_counts_to_m_s2(16384), 9.80665)
        self.assertAlmostEqual(gyro_counts_to_rad_s(131), 3.141592653589793 / 180)
        self.assertAlmostEqual(accel_counts_to_m_s2(-16384), -9.80665)
        self.assertAlmostEqual(gyro_counts_to_rad_s(-131), -3.141592653589793 / 180)

    def test_baseline_is_vector_mean(self):
        self.assertEqual(mean_vector([(1, 2, 3), (3, 4, 5)]), (2, 3, 4))
        with self.assertRaises(ValueError):
            mean_vector([])

    def test_reads_comments_and_reports_marked_segments_and_reference_error(self):
        rows = self.write_log([
            csv_row(1000, 0, 0, "RUNNING_ZUPT_HEURISTIC|ZUPT_APPLIED", "1"),
            csv_row(1010, 0.01, 0.012, "RUNNING_ZUPT_HEURISTIC|ZUPT_APPLIED", "1"),
        ])
        report = make_report(rows, (0.01, 0, 0))
        self.assertIn("Frekvence zaznamenaných řádků", report)
        self.assertIn("1=2", report)
        self.assertIn("Rozsah posunu segmentu 1", report)
        self.assertIn("X=+0.002000", report)
        self.assertIn("ZUPT_APPLIED", report)

    def test_warns_when_sensor_or_serial_samples_were_lost(self):
        rows = self.write_log([
            csv_row(1000, 0, 0),
            csv_row(1010, 0.01, 0),
        ])
        rows[-1]["missed_samples"] = 1
        self.assertIn("není frekvencí všech vzorků senzoru", make_report(rows))

    def test_rejects_non_monotonic_timestamp_and_wrong_dt(self):
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            self.write_log([
                csv_row(1000, 0, 0),
                csv_row(1000, 0.01, 0),
            ])
        with self.assertRaisesRegex(ValueError, "timestamp/dt mismatch"):
            self.write_log([
                csv_row(1000, 0, 0),
                csv_row(1010, 0.02, 0),
            ])


if __name__ == "__main__":
    unittest.main()
