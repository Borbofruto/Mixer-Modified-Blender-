# SPDX-License-Identifier: GPL-2.0-or-later
"""Run with the compiled Mixer, using --background --factory-startup --python."""
from pathlib import Path
import bpy

root = Path(__file__).resolve().parents[2]
output = root / "build" / "layout-tests" / "settings_roundtrip.blend"
output.parent.mkdir(parents=True, exist_ok=True)
screen = bpy.context.screen or bpy.data.screens[0]
screen_name = screen.name
area = screen.areas[0]
area.type = 'NODE_EDITOR'
snode = area.spaces.active
assert snode.node_grid_spacing == 20.0
assert not snode.use_socket_grid_layout
assert not snode.use_node_alignment_guides
assert snode.inspected_socket is None
assert snode.bl_rna.properties['inspected_socket_location'].is_readonly
snode.node_grid_spacing = 1.0
assert snode.node_grid_spacing == 4.0
snode.node_grid_spacing = 1000.0
assert snode.node_grid_spacing == 200.0
snode.node_grid_spacing = 7.5
snode.use_socket_grid_layout = True
snode.use_node_alignment_guides = True
bpy.ops.wm.save_as_mainfile(filepath=str(output))
bpy.ops.wm.open_mainfile(filepath=str(output))
snode = next(area.spaces.active for area in bpy.data.screens[screen_name].areas
             if area.type == 'NODE_EDITOR')
assert snode.node_grid_spacing == 7.5
assert snode.use_socket_grid_layout
assert snode.use_node_alignment_guides
assert snode.inspected_socket is None, "Reloaded editors start without an inspection target"
print("MIXER_NODE_LAYOUT_API_OK")
