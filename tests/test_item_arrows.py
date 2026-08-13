import re
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]
SOURCE = ROOT.joinpath("index.html").read_text(encoding="utf-8")
OTHER_ITEMS = ROOT.joinpath("resources/items_lists/otheritems.js").read_text(encoding="utf-8")


class ItemArrowTests(unittest.TestCase):
    EXPECTED_ITEMS = (
        "start, Blue Team",
        "start, Red Team",
        "start, player",
        "start, dmatch",
        "start, primary, player",
        "start, primary, dmatch",
        "start, primary, Blue Team",
        "start, primary, Red Team",
    )

    def function_body(self, name):
        match = re.search(rf"  (?:async )?function {name}\([^)]*\)\{{", SOURCE)
        self.assertIsNotNone(match, f"missing function {name}")
        next_function = re.search(r"\n  (?:async )?function ", SOURCE[match.end():])
        end = match.end() + next_function.start() if next_function else len(SOURCE)
        return SOURCE[match.end():end]

    def item_block(self, name):
        match = re.search(rf'^begin "{re.escape(name)}"\s*(.*?)^end\b', OTHER_ITEMS, re.M | re.S)
        self.assertIsNotNone(match, f"missing item definition: {name}")
        return match.group(1)

    def test_requested_definitions_enable_arrow_below_tint(self):
        self.assertEqual(OTHER_ITEMS.count("\tarrow 1"), len(self.EXPECTED_ITEMS))
        for name in self.EXPECTED_ITEMS:
            block = self.item_block(name)
            self.assertRegex(block, r"\ttint #[0-9A-Fa-f]{6}\n\tarrow 1\n")

    def test_parser_accepts_only_positive_finite_arrow_values(self):
        parser = self.function_body("parseItemsDef")
        self.assertIn('if (k === "arrow")', parser)
        self.assertIn("Number.isFinite(numericArrow) && numericArrow > 0", parser)
        self.assertIn("cur.arrow = numericArrow", parser)
        helper = self.function_body("getItemArrowSize")
        self.assertIn("Number.isFinite(arrow) && arrow > 0 ? arrow : 0", helper)

    def test_supplemental_arrow_overrides_base_only_when_present(self):
        merge = self.function_body("mergeItemCatalogDefs")
        self.assertIn('Object.prototype.hasOwnProperty.call(supplementKv, "arrow")', merge)
        self.assertIn("supplement.arrow : base.arrow", merge)

    def test_arrow_is_always_white_and_value_is_length_in_meters(self):
        self.assertIn("const ITEM_ARROW_UNIT_LENGTH_METERS = 1", SOURCE)
        self.assertIn("const ITEM_ARROW_COLOR = 0xffffff", SOURCE)
        update = self.function_body("updateItemArrow")
        self.assertIn("arrow.material.color.setHex(ITEM_ARROW_COLOR)", update)
        self.assertIn("arrow.scale.set(lengthMeters, lengthMeters, 1)", update)
        self.assertNotIn("getItemTint", update)
        self.assertNotIn("selectedIds", update)
        self.assertIn("isItemVisibilityFilterMatch(it)", update)
        self.assertIn("getItemArrowHeadingRadians(it.heading)", update)
        create = self.function_body("createItemArrow")
        self.assertIn("arrow.raycast = () => null", create)

    def test_arrow_heading_uses_the_mis_rotation_direction_and_axis_offset(self):
        heading = self.function_body("getItemArrowHeadingRadians")
        self.assertIn("(-(Number(heading) || 0) - 180) * Math.PI / 180", heading)
        self.assertIn("opposite direction from Three.js positive Z", heading)
        self.assertIn("compensate for the arrow geometry's +X axis", heading)

    def test_arrows_sync_independently_from_glb_entries(self):
        sync = self.function_body("syncItemArrows")
        self.assertIn("for (const it of state.items || [])", sync)
        self.assertNotIn("getRenderableGlbEntries", sync)
        draw = self.function_body("drawGlbItems")
        self.assertIn("syncItemArrows();", draw)
        clear = self.function_body("clearGlbItemInstances")
        self.assertIn("clearItemArrows();", clear)


if __name__ == "__main__":
    unittest.main()
