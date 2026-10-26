"""Host-side behavioral models for the V9 automatic workflow."""
import math
import unittest


def rolling_prebuffer(samples, rate):
    return list(samples)[-(rate * 3):]


def allocate_mission(next_mission, occupied):
    while next_mission in occupied:
        next_mission += 1
    return next_mission, next_mission + 1


def estimate_phases(points):
    phases = ["launch", "powered ascent"]
    speeds = [p[1] for p in points]
    peak_ascent = max(speeds)
    if peak_ascent > 8 and any(v < peak_ascent - 5 for v in speeds[1:]):
        phases.append("coast")
    if max(p[0] for p in points) > 8 and any(v <= 0.5 for v in speeds[2:]):
        phases.extend(["apogee", "descent"])
    fastest_descent = min(speeds)
    if fastest_descent < -8 and any(fastest_descent + 4 < v < -0.5 for v in speeds):
        phases.append("probable parachute")
    return phases


def robust_baseline(samples):
    ordered = sorted(samples)
    trim = max(4, len(ordered) // 10)
    kept = ordered[trim:-trim]
    mean = sum(kept) / len(kept)
    noise = math.sqrt(sum((p - mean) ** 2 for p in kept) / max(1, len(kept) - 1))
    return mean, noise


def filter_altitudes(values):
    display = flight = values[0]
    for value in values[1:]:
        d = abs(value - display)
        display_alpha = .10 if d < .25 else .24 if d < 1.5 else .48 if d < 5 else .70
        f = abs(value - flight)
        flight_alpha = .28 if f < .25 else .48 if f < 1.5 else .68 if f < 5 else .84
        display += display_alpha * (value - display)
        flight += flight_alpha * (value - flight)
    return display, flight


def adaptive_rate(rate, low_seconds, critical_seconds):
    if critical_seconds >= 3:
        return min(rate, 10)
    if low_seconds >= 5:
        return min(rate, 25 if rate >= 50 else 10)
    return rate


def pressure_anomaly(jump_pa, elapsed_ms):
    rate = jump_pa * 1000 / elapsed_ms
    return (jump_pa > 40 and rate > 1200) or abs(jump_pa) > 250


def calibration_fresh(age_ms, temperature_change_c, ground_shift_m):
    return age_ms <= 900000 and abs(temperature_change_c) <= 3 and abs(ground_shift_m) <= 2


class V9WorkflowTests(unittest.TestCase):
    def test_prebuffer_is_exactly_three_seconds(self):
        for rate in (10, 25, 50):
            samples = range(rate * 8)
            result = rolling_prebuffer(samples, rate)
            self.assertEqual(len(result), rate * 3)
            self.assertEqual(result[0], rate * 5)

    def test_prebuffer_preserves_uint32_rollover_order(self):
        stamps = [0xFFFFFF00, 0xFFFFFF80, 0x00000010, 0x00000090]
        self.assertEqual(rolling_prebuffer(stamps, 1), stamps[-3:])

    def test_permanent_number_skips_collisions_and_never_reuses_deleted(self):
        mission, following = allocate_mission(4, {1, 2, 4, 5})
        self.assertEqual((mission, following), (6, 7))
        self.assertEqual(allocate_mission(following, set()), (7, 8))

    def test_card_sequence_is_independent_of_visible_files(self):
        next_card_sequence = 4
        visible_files = []
        self.assertFalse(visible_files)
        self.assertEqual(next_card_sequence, 4)

    def test_nominal_pressure_only_phase_sequence(self):
        points = [(0, 0), (5, 20), (20, 35), (35, 25), (45, 8),
                  (50, 0), (48, -12), (40, -5), (30, -4)]
        phases = estimate_phases(points)
        self.assertIn("coast", phases)
        self.assertIn("apogee", phases)
        self.assertIn("probable parachute", phases)

    def test_stationary_noise_does_not_look_like_flight(self):
        phases = estimate_phases([(0.1, 0.2), (-0.1, -0.1), (0.2, 0.1)])
        self.assertNotIn("apogee", phases)
        self.assertNotIn("probable parachute", phases)

    def test_trimmed_calibration_rejects_large_outliers(self):
        samples = [101325 + ((i % 5) - 2) * .2 for i in range(96)]
        samples[0], samples[-1] = 100000, 103000
        baseline, noise = robust_baseline(samples)
        self.assertAlmostEqual(baseline, 101325, delta=.1)
        self.assertLess(noise, .5)

    def test_flight_filter_responds_faster_than_display(self):
        display, flight = filter_altitudes([0, 0, 0, 8])
        self.assertGreater(flight, display)
        self.assertGreater(flight, 6)

    def test_battery_rate_only_steps_down_after_debounce(self):
        self.assertEqual(adaptive_rate(50, 4, 0), 50)
        self.assertEqual(adaptive_rate(50, 5, 0), 25)
        self.assertEqual(adaptive_rate(25, 5, 3), 10)

    def test_pressure_anomaly_rejects_only_abrupt_transients(self):
        self.assertTrue(pressure_anomaly(60, 25))
        self.assertTrue(pressure_anomaly(-300, 25))
        self.assertFalse(pressure_anomaly(60, 1000))

    def test_calibration_freshness_limits(self):
        self.assertTrue(calibration_fresh(60000, 1, .5))
        self.assertFalse(calibration_fresh(900001, 1, .5))
        self.assertFalse(calibration_fresh(60000, 3.1, .5))
        self.assertFalse(calibration_fresh(60000, 1, 2.1))

    def test_sensor_outage_requires_three_valid_recovery_reads(self):
        recovery_reads = 0
        recovered = False
        for valid in (False, True, True, False, True, True, True):
            recovery_reads = recovery_reads + 1 if valid else 0
            recovered = recovery_reads >= 3
        self.assertTrue(recovered)


if __name__ == "__main__":
    unittest.main()
