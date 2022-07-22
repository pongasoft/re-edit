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
    { path = "Anonymous1_path" },
    {
      Knob2 = { offset = { 30, 40 }, { path = "Knob2_path", frames = 32  } }
    },
    { offset = { 100, 110 }, path = "Anonymous2_path" },
    {
      offset = { 50, 60 },
      Knob3 = { { size = { 5, 15 } } }
    }
  }
 }
}

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
