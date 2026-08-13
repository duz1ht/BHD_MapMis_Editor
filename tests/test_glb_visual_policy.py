import re
import unittest
from pathlib import Path

SOURCE = Path(__file__).parents[1].joinpath("index.html").read_text(encoding="utf-8")


class GlbVisualPolicyTests(unittest.TestCase):
    def function_body(self, name):
        match = re.search(rf"  (?:async )?function {name}\([^)]*\)\{{", SOURCE)
        self.assertIsNotNone(match, f"missing function {name}")
        next_function = re.search(r"\n  (?:async )?function ", SOURCE[match.end():])
        end = match.end() + next_function.start() if next_function else len(SOURCE)
        return SOURCE[match.end():end]

    def test_color_precedence_is_explicit_and_applied_in_order(self):
        self.assertIn(
            "base material, DEF tint,\n  // waypoint/area-trigger color, then temporary view/selection visuals",
            SOURCE,
        )
        body = self.function_body("syncGlbItems")
        calls = [
            "applySolidTintToGlbObject(instance, getItemTint(it))",
            "applyWaypointColorToGlbObject(instance, it)",
            "applyAreaTriggerColorToGlbObject(instance, it)",
            "applyViewModeToGlbObject(instance, it)",
        ]
        positions = [body.index(call) for call in calls]
        self.assertEqual(positions, sorted(positions))

    def test_visual_signature_includes_graphic_type_and_tint(self):
        body = self.function_body("getGlbItemVisualSignature")
        for value in ("graphic", "it?.typeId", "getItemTint(it)"):
            self.assertIn(value, body)

    def test_map_loads_invalidate_instances_and_async_generation(self):
        self.assertIn("clearGlbItemInstances();", self.function_body("loadMisFile"))
        self.assertIn("clearGlbItemInstances();", self.function_body("clearEditorMap"))
        sync = self.function_body("syncGlbItems")
        self.assertIn("generation !== state.glb.documentGeneration", sync)

    def test_editor_folder_paths_have_case_insensitive_fallback(self):
        resolver = self.function_body("getEditorFolderEntry")
        self.assertIn("parent[directGetter](name)", resolver)
        self.assertIn('entryName.toLowerCase() === wanted', resolver)
        self.assertIn('handle.kind === kind', resolver)
        binary_reader = self.function_body("readArrayBufferFromEditorFolder")
        self.assertIn("getFileHandleFromEditorFolder(path)", binary_reader)

    def test_selecting_folder_waits_for_stale_glb_sync_and_reloads(self):
        command_start = SOURCE.index('"file.setEditorFolder": async () => {')
        command_end = SOURCE.index('"edit.briefing":', command_start)
        command = SOURCE[command_start:command_end]
        self.assertGreaterEqual(command.count("if (state.glb.syncPromise) await state.glb.syncPromise"), 2)
        self.assertIn("state.glb.templateByGraphic.clear()", command)
        self.assertIn("await syncGlbItems()", command)

    def test_glb_folder_index_supports_nested_assets_and_real_paths(self):
        indexer = self.function_body("listGlbFilesFromEditorFolder")
        self.assertIn("await visit(handle, [...relativeParts, entryName])", indexer)
        self.assertIn("state.glbPathByFilename = paths", indexer)
        loader = self.function_body("loadGraphicTemplate")
        self.assertIn('replace(/\\\\/g, "/").replace(/\\.glb$/i, "")', loader)
        self.assertIn("state.glbPathByFilename.get(requestedFilename.toLowerCase())", loader)


if __name__ == "__main__":
    unittest.main()
