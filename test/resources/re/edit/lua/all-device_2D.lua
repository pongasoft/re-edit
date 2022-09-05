format_version = "2.0"

--------------------------------------------------------------------------
-- front
--------------------------------------------------------------------------
front = {
 bg = { { path = "front_bg" } },
 {
  offset = { 200, 100 },
  {
    offset = { 0, -50 },
    Label = { { path = "Label_path" } },
    {
      offset = { 10, 20 },
      Knob1 = { { path = "Knob1_path", frames = 64 } }
    },
    { path = "Decal1_path" },
    {
      Knob2 = { offset = { 30, 40 }, { path = "Knob2_path", frames = 32  } }
    },
    { offset = { 100, 110 }, path = "Decal2_path" },
    {
      offset = { 50, 60 },
      Knob3 = { { size = { 5, 15 } } }
    }
  }
 }
}
re_edit = re_edit or {}
re_edit.front = { decals = {} }
re_edit.front.decals[1] = "decal1"
re_edit.front.decals[2] = "decal2"

--------------------------------------------------------------------------
-- folded_back (cable origin)
--------------------------------------------------------------------------
folded_back = {
    Panel_folded_back_bg = {
        { path = "Panel_folded_back" }
    },
}

folded_back['DeviceName'] = { offset = { 1330, 45 }, { path = 'Tape_Horizontal_1frames' } }
folded_back['CableOrigin'] = { offset = { 695, 75 } }
folded_back[1] = { offset = { 5, 5 }, { path = "Decal_path" } }
