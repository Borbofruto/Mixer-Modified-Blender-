/* SPDX-FileCopyrightText: 2001-2002 NaN Holding BV. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup edtransform
 */

#include "DNA_space_types.h"
#include "DNA_userdef_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_listbase.h"
#include "BLI_math_matrix.h"
#include "BLI_math_vector.h"
#include "BLI_math_vector.hh"
#include "BLI_rect.h"

#include "BKE_context.hh"
#include "BKE_main_invariants.hh"
#include "BKE_node.hh"
#include "BKE_node_runtime.hh"

#include "ED_node.hh"
#include "ED_node_layout.hh"
#include "ED_numinput.hh"

#include "UI_view2d.hh"

#include "transform.hh"
#include "transform_convert.hh"
#include "transform_snap.hh"

#include "WM_api.hh"

namespace blender::ed::transform {

struct NodeAlignmentSource {
  int data_index;
  float2 initial_position;
};

struct TransCustomDataNode {
  ui::View2DEdgePanData edgepan_data{};

  /* Compare if the view has changed so we can update with `transformViewUpdate`. */
  rctf viewrect_prev{};

  bool is_new_node = false;
  Vector<NodeAlignmentSource> alignment_sources;
  Vector<float> alignment_targets_x;
  Vector<float> alignment_targets_y;

  Map<bNode *, bNode *> old_parent_by_detached_node;
};

/* -------------------------------------------------------------------- */
/** \name Node Transform Creation
 * \{ */

static void create_transform_data_for_node(TransData &td,
                                           TransData2D &td2d,
                                           bNode &node,
                                           const float dpi_fac)
{
  /* Account for parents (nested nodes). */
  float2 loc = float2(node.location) * dpi_fac;

  /* Use top-left corner as the transform origin for nodes. */
  /* Weirdo - but the node system is a mix of free 2d elements and DPI sensitive UI. */
  td2d.loc[0] = loc.x;
  td2d.loc[1] = loc.y;
  td2d.loc[2] = 0.0f;
  td2d.loc2d = td2d.loc; /* Current location. */

  td.loc = td2d.loc;
  copy_v3_v3(td.iloc, td.loc);
  /* Use node center instead of origin (top-left corner). */
  td.center[0] = td2d.loc[0];
  td.center[1] = td2d.loc[1];
  td.center[2] = 0.0f;

  memset(td.axismtx, 0, sizeof(td.axismtx));
  td.axismtx[2][2] = 1.0f;

  td.val = nullptr;

  td.flag = TD_SELECTED;
  td.dist = 0.0f;

  unit_m3(td.mtx);
  unit_m3(td.smtx);

  td.extra = &node;
}

static bool is_node_parent_select(const bNode *node)
{
  while ((node = node->parent)) {
    if (node->flag & NODE_SELECT) {
      return true;
    }
  }
  return false;
}

/**
 * Some nodes are transformed together with other nodes:
 * - Parent frames with shrinking turned on are automatically resized based on their children.
 * - Child nodes of frames that are manually resizable are transformed together with their parent
 *   frame.
 */
static bool transform_tied_to_other_node(bNode *node, VectorSet<bNode *> transformed_nodes)
{
  /* Check for frame nodes that adjust their size based on the contained child nodes. */
  if (node->is_frame()) {
    const NodeFrame *data = static_cast<const NodeFrame *>(node->storage);
    const bool shrinking = data->flag & NODE_FRAME_SHRINK;
    const bool is_parent = !node->direct_children_in_frame().is_empty();

    if (is_parent && shrinking) {
      return true;
    }
  }

  /* Now check for child nodes of manually resized frames. */
  while ((node = node->parent)) {
    const NodeFrame *parent_data = static_cast<const NodeFrame *>(node->storage);
    const bool parent_shrinking = parent_data->flag & NODE_FRAME_SHRINK;
    const bool parent_transformed = transformed_nodes.contains(node);

    if (parent_transformed && !parent_shrinking) {
      return true;
    }
  }

  return false;
}

static VectorSet<bNode *> get_transformed_nodes(bNodeTree &node_tree)
{
  VectorSet<bNode *> nodes = node_tree.all_nodes();

  /* Keep only nodes that are selected or inside a frame that is selected. */
  nodes.remove_if([&](bNode *node) {
    const bool node_selected = node->flag & NODE_SELECT;
    const bool parent_selected = is_node_parent_select(node);
    return (!node_selected && !parent_selected);
  });

  /* Remove nodes that are transformed together with their parent or child nodes. */
  nodes.remove_if([&](bNode *node) { return transform_tied_to_other_node(node, nodes); });

  return nodes;
}

/** Bounds and sockets share view coordinates, including UI scaling. */
static Vector<float2> node_alignment_anchors(const bNode &node)
{
  Vector<float2> anchors;
  if (node.is_reroute()) {
    anchors.append(float2(node.location) * UI_SCALE_FAC);
    return anchors;
  }
  const rctf &bounds = node.runtime->draw_bounds;
  anchors.append(float2(bounds.xmin, bounds.ymin));
  anchors.append(float2(bounds.xmax, bounds.ymax));
  anchors.append(float2(BLI_rctf_cent_x(&bounds), BLI_rctf_cent_y(&bounds)));
  for (const bNodeSocket *socket : node.input_sockets()) {
    if (socket->is_icon_visible()) {
      anchors.append(socket->runtime->location);
    }
  }
  for (const bNodeSocket *socket : node.output_sockets()) {
    if (socket->is_icon_visible()) {
      anchors.append(socket->runtime->location);
    }
  }
  return anchors;
}

static void node_alignment_cache_init(const SpaceNode &snode,
                                      const ARegion &region,
                                      const VectorSet<bNode *> &moving_nodes,
                                      TransCustomDataNode &data)
{
  for (const int i : moving_nodes.index_range()) {
    for (const float2 position : node_alignment_anchors(*moving_nodes[i])) {
      data.alignment_sources.append({i, position});
    }
  }
  for (const bNode *node : snode.edittree->all_nodes()) {
    /* Frames may resize when a selected descendant moves; they are unstable targets. */
    if (moving_nodes.contains(const_cast<bNode *>(node)) || node->is_frame() ||
        (node->flag & NODE_SELECT) || is_node_parent_select(node))
    {
      continue;
    }
    const rctf &bounds = node->runtime->draw_bounds;
    if (bounds.xmax < region.v2d.cur.xmin || bounds.xmin > region.v2d.cur.xmax ||
        bounds.ymax < region.v2d.cur.ymin || bounds.ymin > region.v2d.cur.ymax)
    {
      continue;
    }
    for (const float2 position : node_alignment_anchors(*node)) {
      data.alignment_targets_x.append(position.x);
      data.alignment_targets_y.append(position.y);
    }
  }
  std::sort(data.alignment_targets_x.begin(), data.alignment_targets_x.end());
  std::sort(data.alignment_targets_y.begin(), data.alignment_targets_y.end());
}

static void createTransNodeData(bContext * /*C*/, TransInfo *t)
{
  SpaceNode *snode = static_cast<SpaceNode *>(t->area->spacedata.first);
  bNodeTree *node_tree = snode->edittree;
  if (!node_tree) {
    return;
  }

  /* Custom data to enable edge panning during the node transform. */
  TransCustomDataNode *customdata = MEM_new<TransCustomDataNode>(__func__);
  view2d_edge_pan_init(t->context,
                       &customdata->edgepan_data,
                       NODE_EDGE_PAN_INSIDE_PAD,
                       NODE_EDGE_PAN_OUTSIDE_PAD,
                       NODE_EDGE_PAN_SPEED_RAMP,
                       NODE_EDGE_PAN_MAX_SPEED,
                       NODE_EDGE_PAN_DELAY,
                       NODE_EDGE_PAN_ZOOM_INFLUENCE);
  customdata->viewrect_prev = customdata->edgepan_data.initial_rect;
  customdata->is_new_node = t->remove_on_cancel;

  if (t->region) {
    space_node::node_insert_on_link_flags_set(
        *snode, *t->region, t->modifiers & MOD_NODE_ATTACH, customdata->is_new_node);
    space_node::node_insert_on_frame_flag_set(*snode, *t->region, int2(t->mval));
  }

  t->custom.type.data = customdata;
  t->custom.type.free_cb = [](TransInfo *t, TransDataContainer *, TransCustomData *custom_data) {
    if (t->area && t->area->spacetype == SPACE_NODE) {
      SpaceNode &snode = *static_cast<SpaceNode *>(t->area->spacedata.first);
      space_node::node_alignment_guides_set(snode, std::nullopt, std::nullopt);
    }
    TransCustomDataNode *data = static_cast<TransCustomDataNode *>(custom_data->data);
    MEM_delete(data);
    custom_data->data = nullptr;
  };

  TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_SINGLE(t);

  /* Nodes don't support proportional editing and probably never will. */
  t->flag = t->flag & ~T_PROP_EDIT_ALL;

  VectorSet<bNode *> nodes = get_transformed_nodes(*node_tree);
  if (nodes.is_empty()) {
    return;
  }

  tc->data_len = nodes.size();
  tc->data = MEM_new_array_zeroed<TransData>(tc->data_len, __func__);
  tc->data_2d = MEM_new_array_zeroed<TransData2D>(tc->data_len, __func__);

  for (const int i : nodes.index_range()) {
    create_transform_data_for_node(tc->data[i], tc->data_2d[i], *nodes[i], UI_SCALE_FAC);
  }
  if ((t->flag & T_MODAL) && t->region && (snode->node_layout_flag & SNODE_LAYOUT_SMART_GUIDES)) {
    node_alignment_cache_init(*snode, *t->region, nodes, *customdata);
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Flush Transform Nodes
 * \{ */

static void node_snap_grid_apply(TransInfo *t)
{
  if (t->state == TRANS_CANCEL || !(transform_snap_is_active(t) &&
                                    (t->tsnap.mode & (SCE_SNAP_TO_INCREMENT | SCE_SNAP_TO_GRID))))
  {
    return;
  }

  float2 grid_size = t->snap_spatial;
  if (t->modifiers & MOD_PRECISION) {
    grid_size *= t->snap_spatial_precision;
  }

  /* Early exit on unusable grid size. */
  if (math::is_zero(grid_size)) {
    return;
  }

  const SpaceNode &snode = *static_cast<const SpaceNode *>(t->area->spacedata.first);
  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    if ((t->flag & T_MODAL) && (snode.node_layout_flag & SNODE_LAYOUT_SMART_GUIDES) &&
        tc->data_len > 1)
    {
      /* A shared grid correction preserves the layout of a dragged multi-selection. */
      const TransData *reference = nullptr;
      for (const TransData &td : Span(tc->data, tc->data_len)) {
        if (!(td.flag & TD_SKIP)) {
          reference = &td;
          break;
        }
      }
      if (reference) {
        const float2 initial = reference->iloc;
        const float2 target = reference->loc;
        const float2 snapped = math::round(initial / grid_size) * grid_size +
                               math::round((target - initial) / grid_size) * grid_size;
        const float2 correction = snapped - target;
        for (TransData &td : MutableSpan(tc->data, tc->data_len)) {
          if (!(td.flag & TD_SKIP)) {
            td.loc[0] += correction.x;
            td.loc[1] += correction.y;
          }
        }
      }
      continue;
    }
    for (const int i : IndexRange(tc->data_len)) {
      TransData &td = tc->data[i];
      if (td.flag & TD_SKIP) {
        continue;
      }

      if ((t->flag & T_PROP_EDIT) && (td.factor == 0.0f)) {
        continue;
      }

      /* Nodes are snapped to the grid by first aligning their initial position to the grid and
       * then offsetting them in grid increments.
       *
       * This ensures that multiple unsnapped nodes snap to the grid in sync while moving.
       */

      const float2 inital_location = td.iloc;
      const float2 target_location = td.loc;
      const float2 offset = target_location - inital_location;

      const float2 snapped_inital_location = math::round(inital_location / grid_size) * grid_size;
      const float2 snapped_offset = math::round(offset / grid_size) * grid_size;
      const float2 snapped_target_location = snapped_inital_location + snapped_offset;

      copy_v2_v2(td.loc, snapped_target_location);
    }
  }
}

static void node_alignment_apply(TransInfo *t, const float2 &view_offset)
{
  SpaceNode &snode = *static_cast<SpaceNode *>(t->area->spacedata.first);
  space_node::node_alignment_guides_set(snode, std::nullopt, std::nullopt);
  if (!(snode.node_layout_flag & SNODE_LAYOUT_SMART_GUIDES) || !t->region ||
      !(t->flag & T_MODAL) || t->state == TRANS_CANCEL || t->mode != TFM_TRANSLATION ||
      hasNumInput(&t->num))
  {
    return;
  }
  const auto &data = *static_cast<const TransCustomDataNode *>(t->custom.type.data);
  TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_SINGLE(t);
  const bool snap = transform_snap_is_active(t);
  const float2 scale(ui::view2d_scale_get_x(&t->region->v2d),
                     ui::view2d_scale_get_y(&t->region->v2d));
  float2 correction(0.0f);
  std::optional<float> guides[2];
  for (int axis = 0; axis < 2; axis++) {
    if ((t->con.mode & CON_APPLY) && !(t->con.mode & (CON_AXIS0 << axis))) {
      continue;
    }
    const Vector<float> &targets = axis == 0 ? data.alignment_targets_x : data.alignment_targets_y;
    /* Screen-space tolerance stays stable under zoom; without snap only show exact alignment. */
    float best_distance = (snap ? 8.0f : 0.75f) / std::max(std::abs(scale[axis]), 1e-5f);
    for (const NodeAlignmentSource &source : data.alignment_sources) {
      const TransData &td = tc->data[source.data_index];
      if (td.flag & TD_SKIP) {
        continue;
      }
      const float coordinate = source.initial_position[axis] + td.loc[axis] - td.iloc[axis] +
                               view_offset[axis];
      const std::optional<float> target = space_node::layout::nearest_axis_coordinate(
          targets.begin(), targets.end(), coordinate);
      if (target && std::abs(*target - coordinate) < best_distance) {
        best_distance = std::abs(*target - coordinate);
        correction[axis] = *target - coordinate;
        guides[axis] = *target;
      }
    }
  }
  if (snap) {
    /* One correction for the whole selection; never pull selected nodes apart. */
    for (TransData &td : MutableSpan(tc->data, tc->data_len)) {
      if (!(td.flag & TD_SKIP)) {
        td.loc[0] += correction.x;
        td.loc[1] += correction.y;
      }
    }
  }
  space_node::node_alignment_guides_set(snode, guides[0], guides[1]);
}

static void move_child_nodes(bNode &node, const float2 &delta)
{
  for (bNode *child : node.direct_children_in_frame()) {
    child->location[0] += delta.x;
    child->location[1] += delta.y;
    if (child->is_frame()) {
      move_child_nodes(*child, delta);
    }
  }
}

static bool has_selected_parent(const bNode &node)
{
  for (bNode *parent = node.parent; parent; parent = parent->parent) {
    if (parent->flag & NODE_SELECT) {
      return true;
    }
  }
  return false;
}

static void flushTransNodes(TransInfo *t)
{
  const float dpi_fac = UI_SCALE_FAC;
  SpaceNode *snode = static_cast<SpaceNode *>(t->area->spacedata.first);

  TransCustomDataNode *customdata = static_cast<TransCustomDataNode *>(t->custom.type.data);

  if (t->options & CTX_VIEW2D_EDGE_PAN) {
    if (t->state == TRANS_CANCEL) {
      view2d_edge_pan_cancel(t->context, &customdata->edgepan_data);
    }
    else {
      /* Edge panning functions expect window coordinates, mval is relative to region. */
      const int xy[2] = {
          t->region->winrct.xmin + int(t->mval[0]),
          t->region->winrct.ymin + int(t->mval[1]),
      };
      ui::view2d_edge_pan_apply(t->context, &customdata->edgepan_data, xy);
    }
  }

  float offset[2] = {0.0f, 0.0f};
  if (t->state != TRANS_CANCEL) {
    if (t->region &&
        !BLI_rctf_compare(&customdata->viewrect_prev, &t->region->v2d.cur, FLT_EPSILON))
    {
      /* Additional offset due to change in view2D rect. */
      BLI_rctf_transform_pt_v(&t->region->v2d.cur, &customdata->viewrect_prev, offset, offset);
      transformViewUpdate(t);
      customdata->viewrect_prev = t->region->v2d.cur;
    }
  }

  if (t->modifiers & MOD_NODE_FRAME) {
    t->modifiers &= ~MOD_NODE_FRAME;
    Vector<bNode *> nodes_to_detach;
    for (bNode *node : snode->edittree->all_nodes()) {
      if (!(node->flag & NODE_SELECT)) {
        continue;
      }
      if (has_selected_parent(*node)) {
        continue;
      }
      if (!node->parent) {
        continue;
      }
      customdata->old_parent_by_detached_node.add(node, node->parent);
      nodes_to_detach.append(node);
    }
    if (nodes_to_detach.is_empty()) {
      WM_operator_name_call(
          t->context, "NODE_OT_attach", wm::OpCallContext::InvokeDefault, nullptr, nullptr);
    }
    else {
      for (bNode *node : nodes_to_detach) {
        bke::node_detach_node(*snode->edittree, *node);
      }
    }
  }

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    node_snap_grid_apply(t);
    node_alignment_apply(t, float2(offset));

    /* Flush to 2d vector from internally used 3d vector. */
    for (int i = 0; i < tc->data_len; i++) {
      TransData *td = &tc->data[i];
      TransData2D *td2d = &tc->data_2d[i];
      bNode *node = static_cast<bNode *>(td->extra);

      float2 loc = float2(td2d->loc) + offset;

      /* Weirdo - but the node system is a mix of free 2d elements and DPI sensitive UI. */
      loc /= dpi_fac;

      if (node->is_frame()) {
        const float2 delta = loc - float2(node->location);
        move_child_nodes(*node, delta);
      }

      node->location[0] = loc.x;
      node->location[1] = loc.y;
    }

    /* Handle intersection with noodles. */
    if (tc->data_len == 1) {
      space_node::node_insert_on_link_flags_set(
          *snode, *t->region, t->modifiers & MOD_NODE_ATTACH, customdata->is_new_node);
    }
    if (t->region) {
      space_node::node_insert_on_frame_flag_set(*snode, *t->region, int2(t->mval));
    }
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Special After Transform Node
 * \{ */

static void special_aftertrans_update__node(bContext *C, TransInfo *t)
{
  Main *bmain = CTX_data_main(C);
  SpaceNode *snode = static_cast<SpaceNode *>(t->area->spacedata.first);
  bNodeTree *ntree = snode->edittree;
  const TransCustomDataNode &customdata = *static_cast<TransCustomDataNode *>(t->custom.type.data);

  const bool canceled = (t->state == TRANS_CANCEL);
  space_node::node_alignment_guides_set(*snode, std::nullopt, std::nullopt);

  if (canceled) {
    for (auto &&[node, parent] : customdata.old_parent_by_detached_node.items()) {
      bke::node_attach_node(*ntree, *node, *parent);
    }
  }
  if (canceled && t->remove_on_cancel) {
    /* Remove selected nodes on cancel. */
    if (ntree) {
      for (bNode &node : ntree->nodes.items_mutable()) {
        if (node.flag & NODE_SELECT) {
          bke::node_remove_node(bmain, *ntree, node, true);
        }
      }
      BKE_main_ensure_invariants(*bmain, ntree->id);
    }
  }

  if (!canceled) {
    ED_node_post_apply_transform(C, snode->edittree);
    if (t->modifiers & MOD_NODE_ATTACH) {
      space_node::node_insert_on_link_flags(*bmain, *snode, customdata.is_new_node);
    }
  }

  space_node::node_insert_on_link_flags_clear(*ntree);
  space_node::node_insert_on_frame_flag_clear(*snode);

  wmOperatorType *ot = WM_operatortype_find("NODE_OT_insert_offset", true);
  BLI_assert(ot);
  PointerRNA ptr = WM_operator_properties_create_ptr(ot);
  WM_operator_name_call_ptr(C, ot, wm::OpCallContext::InvokeDefault, &ptr, nullptr);
  WM_operator_properties_free(&ptr);
}

/** \} */

TransConvertTypeInfo TransConvertType_Node = {
    /*flags*/ (T_POINTS | T_2D_EDIT),
    /*create_trans_data*/ createTransNodeData,
    /*recalc_data*/ flushTransNodes,
    /*special_aftertrans_update*/ special_aftertrans_update__node,
};

}  // namespace blender::ed::transform
