#!/usr/bin/env python3
"""Summarize BMI160 experiment CSV logs using only the Python standard library."""

import argparse
import csv
import math
import sys
from collections import Counter

GRAVITY_M_S2 = 9.80665
RADIANS_PER_DEGREE = math.pi / 180.0
TIMESTAMP_TOLERANCE_S = 0.0021
REQUIRED_COLUMNS = {
    "timestamp_ms",
    "dt_s",
    "accel_x_raw_lsb",
    "gyro_x_raw_lsb",
    "accel_x_m_s2",
    "gyro_x_rad_s",
    "corrected_accel_x_m_s2",
    "velocity_x_m_s",
    "displacement_x_m",
    "displacement_y_m",
    "displacement_z_m",
    "calibration_valid",
    "state_flags",
    "missed_samples",
    "serial_dropped_rows",
    "segment_id",
}


def accel_counts_to_m_s2(counts, counts_per_g=16384.0):
    return counts * GRAVITY_M_S2 / counts_per_g


def gyro_counts_to_rad_s(counts, counts_per_degree_per_second=131.0):
    return counts * RADIANS_PER_DEGREE / counts_per_degree_per_second


def mean_vector(vectors):
    vectors = list(vectors)
    if not vectors or any(len(vector) != 3 for vector in vectors):
        raise ValueError("mean requires at least one three-axis vector")
    return tuple(sum(vector[axis] for vector in vectors) / len(vectors)
                 for axis in range(3))


def read_log(path):
    with open(path, encoding="utf-8", newline="") as source:
        data_lines = (line for line in source
                      if line.strip() and not line.lstrip().startswith("#"))
        reader = csv.DictReader(data_lines)
        if reader.fieldnames is None or not REQUIRED_COLUMNS.issubset(reader.fieldnames):
            missing = sorted(REQUIRED_COLUMNS - set(reader.fieldnames or ()))
            raise ValueError("missing required CSV columns: " + ", ".join(missing))

        rows = []
        for line_number, row in enumerate(reader, start=2):
            try:
                parsed = {
                    "timestamp_s": float(row["timestamp_ms"]) / 1000.0,
                    "dt_s": float(row["dt_s"]),
                    "calibration_valid": int(row["calibration_valid"]),
                    "state_flags": row["state_flags"],
                    "missed_samples": int(row["missed_samples"]),
                    "serial_dropped_rows": int(row["serial_dropped_rows"]),
                    "segment_id": row["segment_id"],
                    "displacement": tuple(
                        float(row[f"displacement_{axis}_m"])
                        for axis in "xyz"
                    ),
                }
            except (TypeError, ValueError, KeyError) as error:
                raise ValueError(f"invalid CSV row near line {line_number}: {error}") from error
            if not all(math.isfinite(value) for value in
                       (parsed["timestamp_s"], parsed["dt_s"], *parsed["displacement"])):
                raise ValueError(f"non-finite number near line {line_number}")
            rows.append(parsed)
    validate_timestamps(rows)
    return rows


def validate_timestamps(rows):
    for index, row in enumerate(rows):
        if row["dt_s"] < 0 or (index > 0 and row["dt_s"] <= 0):
            raise ValueError(f"non-positive dt_s at sample {index}")
        if index == 0:
            continue
        previous = rows[index - 1]
        timestamp_delta = row["timestamp_s"] - previous["timestamp_s"]
        if timestamp_delta <= 0:
            raise ValueError(f"timestamps are not strictly increasing at sample {index}")
        if abs(timestamp_delta - row["dt_s"]) > TIMESTAMP_TOLERANCE_S:
            raise ValueError(f"timestamp/dt mismatch at sample {index}")


def _axis_range(rows, axis):
    values = [row["displacement"][axis] for row in rows]
    return max(values) - min(values)


def make_report(rows, reference_end=None):
    if not rows:
        raise ValueError("CSV contains no sample rows")
    first, last = rows[0], rows[-1]
    elapsed = last["timestamp_s"] - first["timestamp_s"]
    frequency = (len(rows) - 1) / elapsed if elapsed > 0 and len(rows) > 1 else 0.0
    dt_values = [row["dt_s"] for row in rows[1:]]
    stationary = [row for row in rows if "ZUPT_APPLIED" in row["state_flags"]]
    segments = Counter(row["segment_id"] for row in rows)
    missed = max(row["missed_samples"] for row in rows)
    dropped = max(row["serial_dropped_rows"] for row in rows)
    lines = [
        f"Vzorky: {len(rows)}",
        f"Časový rozsah: {elapsed:.3f} s",
        f"Frekvence zaznamenaných řádků podle časových značek: {frequency:.2f} Hz",
    ]
    if missed or dropped:
        lines.append(
            "POZOR: log hlásí ztracené vzorky/řádky; frekvence řádků CSV není "
            "frekvencí všech vzorků senzoru."
        )
    if dt_values:
        lines.append(f"dt min/max: {min(dt_values):.6f} / {max(dt_values):.6f} s")
    else:
        lines.append("dt min/max: pouze jeden vzorek")
    lines.extend([
        f"Kalibrace platná v řádcích: {sum(row['calibration_valid'] == 1 for row in rows)} / {len(rows)}",
        f"Nahlášené vynechané vzorky (max kumulativně): {missed}",
        f"Nahlášené zahozené CSV řádky (max kumulativně): {dropped}",
        "Segmenty: " + ", ".join(
            f"{segment}={count}" for segment, count in sorted(segments.items())
        ),
    ])
    for segment in sorted(segments):
        segment_rows = [row for row in rows if row["segment_id"] == segment]
        lines.append(
            f"Rozsah posunu segmentu {segment} (m): "
            + ", ".join(f"{axis.upper()}={_axis_range(segment_rows, index):.6f}"
                        for index, axis in enumerate("xyz"))
        )
    if stationary:
        lines.append(
            "Rozsah odhadovaného posunu v řádcích ZUPT_APPLIED (m): "
            + ", ".join(f"{axis.upper()}={_axis_range(stationary, index):.6f}"
                        for index, axis in enumerate("xyz"))
        )
    else:
        lines.append("Statický drift: žádné řádky ZUPT_APPLIED; označ statický úsek při měření.")
    lines.append(
        "Konečný odhad posunu (m): "
        + ", ".join(f"{axis.upper()}={last['displacement'][index]:.6f}"
                    for index, axis in enumerate("xyz"))
    )
    if reference_end is not None:
        errors = tuple(last["displacement"][i] - reference_end[i] for i in range(3))
        lines.append(
            "Chyba odhad − skutečnost (m): "
            + ", ".join(f"{axis.upper()}={errors[index]:+.6f}"
                        for index, axis in enumerate("xyz"))
        )
    return "\n".join(lines)


def parse_reference_end(value):
    try:
        coordinates = tuple(float(part.strip()) for part in value.split(","))
    except ValueError as error:
        raise argparse.ArgumentTypeError("use X,Y,Z in metres") from error
    if len(coordinates) != 3 or not all(math.isfinite(item) for item in coordinates):
        raise argparse.ArgumentTypeError("use three finite coordinates X,Y,Z in metres")
    return coordinates


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", help="CSV captured from the Arduino serial monitor")
    parser.add_argument(
        "--reference-end-m",
        type=parse_reference_end,
        help="known endpoint X,Y,Z in metres, measured independently",
    )
    args = parser.parse_args(argv)
    try:
        rows = read_log(args.csv_file)
        print(make_report(rows, args.reference_end_m))
    except (OSError, UnicodeError, ValueError, csv.Error) as error:
        print(f"Chyba: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
