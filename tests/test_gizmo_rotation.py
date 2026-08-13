import re
import unittest
from pathlib import Path


SOURCE = Path(__file__).parents[1].joinpath("index.html").read_text(encoding="utf-8")


def function_body(name):
    match = re.search(rf"  function {name}\([^)]*\)\{{", SOURCE)
    if not match:
        raise AssertionError(f"missing function {name}")
    next_function = re.search(r"\n  function ", SOURCE[match.end():])
    end = match.end() + next_function.start() if next_function else len(SOURCE)
    return SOURCE[match.end():end]


class GizmoRotationTests(unittest.TestCase):
    def test_yaw_value_inverts_threejs_geometric_rotation_sign(self):
        value_conversion = function_body("gizmoRotationValueDegrees")
        geometry_conversion = function_body("gizmoRotationGeometryDegrees")
        self.assertIn("mode === 'rotZ' ? -geometricDegrees : geometricDegrees", value_conversion)
        self.assertIn("mode === 'rotZ' ? -valueDegrees : valueDegrees", geometry_conversion)

    def test_yaw_value_is_applied_with_positive_heading_sign(self):
        self.assertIn("it.heading=start.heading+itemDegrees", SOURCE)
        self.assertIn("d.label=`${valueDegrees.toFixed(1)}°`", SOURCE)
        self.assertNotIn("it.heading=start.heading-itemDeg", SOURCE)

    def test_absolute_snap_uses_the_displayed_yaw_convention(self):
        self.assertIn("valueDegrees=snapValue(initial+valueDegrees,state.rotationSnap)-initial", SOURCE)
        self.assertIn("itemDegrees=snapValue(initial+rawValueDegrees,state.rotationSnap)-initial", SOURCE)


if __name__ == "__main__":
    unittest.main()
