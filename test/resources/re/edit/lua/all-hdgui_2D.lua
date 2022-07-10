format_version = "2.0"

--------------------------------------------------------------------------
-- front
--------------------------------------------------------------------------
front_widgets = {}

-- device name / tape
-- front_widgets[#front_widgets + 1] = jbox.device_name {
--   graphics = {
--     node = "DeviceName",
--   },
-- }

front_widgets[#front_widgets + 1] = jbox.analog_knob {
  graphics = {
    node = "ak1_node",
  },
  value = "/ak1",
}

front_widgets[#front_widgets + 1] = jbox.analog_knob {
  graphics = {
    node = "ak2_node",
  },
  value_switch = "/ak2_switch",
  values = { "/ak2_v1", "/ak2_v2" }
}

front_widgets[#front_widgets + 1] = jbox.analog_knob {
  graphics = {
    node = "ak3_node",
  },
  value = "/ak3",
  visibility_switch = "/ak3_switch",
  visibility_values = { 1, 0, 3 }
}

front = jbox.panel{
  graphics = {
    node = "Panel_front_bg",
  },
  widgets = front_widgets
}

