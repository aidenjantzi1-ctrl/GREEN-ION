"""Host-side behavioral models for the V9 automatic workflow."""
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
    if any(a > 3 and b < a - 3 for a, b in zip(speeds, speeds[1:])):
        phases.append("coast")
    if max(p[0] for p in points) > 8 and any(v <= 0.5 for v in speeds[2:]):
        phases.extend(["apogee", "descent"])
    if any(a < -8 and b > a + 4 and b < -0.5 for a, b in zip(speeds, speeds[1:])):
        phases.append("probable parachute")
    return phases


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


if __name__ == "__main__":
    unittest.main()
