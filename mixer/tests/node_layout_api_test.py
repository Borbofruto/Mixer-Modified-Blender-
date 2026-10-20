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
tree = bpy.data.node_groups.new("MixerGridGroupingTest", 'ShaderNodeTree')
tree.use_fake_user = True
snode.tree_type = 'ShaderNodeTree'
snode.pin = True
snode.node_tree = tree
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

peer_area = next(candidate for candidate in screen.areas if candidate != area)
peer_area.type = 'NODE_EDITOR'
peer = peer_area.spaces.active
peer.tree_type = 'ShaderNodeTree'
peer.pin = True
peer.node_tree = tree
assert peer.use_socket_grid_layout
assert peer.node_grid_spacing == 7.5
assert not peer.use_node_alignment_guides
peer.node_grid_spacing = 12.5
assert snode.node_grid_spacing == 12.5, "Socket geometry must have one grid per tree"
peer.use_socket_grid_layout = False
assert not snode.use_socket_grid_layout
snode.node_grid_spacing = 10.0
peer.node_grid_spacing = 30.0
assert snode.node_grid_spacing == 10.0 and peer.node_grid_spacing == 30.0
snode.node_grid_spacing = 7.5
snode.use_socket_grid_layout = True
assert peer.node_grid_spacing == 7.5

first = tree.nodes.new('ShaderNodeMath')
second = tree.nodes.new('ShaderNodeMath')
first.location = (-16.0, 21.0)
second.location = (16.0, 44.0)
first.select = second.select = True
tree.nodes.active = first
region = next(region for region in area.regions if region.type == 'WINDOW')
with bpy.context.temp_override(area=area, region=region):
    for scene_snap in (False, True):
        bpy.context.scene.tool_settings.use_snap_node = scene_snap
        first.location = (-16.0, 21.0)
        second.location = (16.0, 44.0)
        assert bpy.ops.transform.translate(
            value=(0.0, 0.0, 0.0), snap=True, snap_elements={'INCREMENT'}) == {'FINISHED'}
        assert bpy.context.scene.tool_settings.use_snap_node == scene_snap
        for node in (first, second):
            for coordinate in node.location:
                assert abs(coordinate / 7.5 - round(coordinate / 7.5)) < 1e-4, node.name
    assert bpy.ops.node.group_make() == {'FINISHED'}
group = next(node for node in tree.nodes if node.bl_idname == 'ShaderNodeGroup')
for node in [group, *group.node_tree.nodes]:
    for coordinate in node.location:
        assert abs(coordinate / 7.5 - round(coordinate / 7.5)) < 1e-4, node.name
snode.node_tree = group.node_tree
assert snode.use_socket_grid_layout, "New groups must inherit their socket layout"
snode.node_grid_spacing = 15.0
assert peer.node_grid_spacing == 7.5, "Different trees retain separate socket geometry settings"
snode.node_grid_spacing = 7.5

library_path = output.parent / "linked_layout_source.blend"
bpy.data.libraries.write(str(library_path), {tree})
with bpy.data.libraries.load(str(library_path), link=True) as (source, destination):
    destination.node_groups = [tree.name]
linked_tree = destination.node_groups[0]
snode.node_tree = linked_tree
assert snode.use_socket_grid_layout
assert snode.is_property_readonly("use_socket_grid_layout")
assert snode.is_property_readonly("node_grid_spacing")
try:
    snode.node_grid_spacing = 12.5
except AttributeError:
    pass
else:
    raise AssertionError("Linked tree socket geometry must remain read-only")
snode.node_tree = group.node_tree

bpy.ops.wm.save_as_mainfile(filepath=str(output))
bpy.ops.wm.open_mainfile(filepath=str(output))
snode = bpy.data.screens[screen_name].areas[0].spaces.active
assert snode.node_grid_spacing == 7.5
assert snode.use_socket_grid_layout
assert snode.use_node_alignment_guides
assert snode.inspected_socket is None, "Reloaded editors start without an inspection target"
print("MIXER_NODE_LAYOUT_API_OK")
