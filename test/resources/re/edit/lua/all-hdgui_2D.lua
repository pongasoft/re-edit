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
    hit_boundaries = { top = 1, bottom = 2, left = 3, right = 4 }
  },
  value_switch = "/ak2_switch",
  values = { "/ak2_v1", "/ak2_v2" },
  tooltip_position = "top",
  tooltip_template = jbox.ui_text("ak2_tooltip_template"),
  show_remote_box = false,
  show_automation_rect = false,
}

front_widgets[#front_widgets + 1] = jbox.analog_knob {
  graphics = {
    node = "ak3_node",
  },
  value = "/ak3",
  visibility_switch = "/ak3_switch",
  visibility_values = { 1, 0, 3 }
}

front_widgets[#front_widgets + 1] = jbox.static_decoration {
  graphics = {
    node = "sd1_node",
  },
}

front_widgets[#front_widgets + 1] = jbox.static_decoration {
  graphics = {
    node = "sd2_node",
  },
  blend_mode = "luminance",
  visibility_switch = "/sd2_switch",
  visibility_values = { 4, 9, 1 }
}

front_widgets[#front_widgets + 1] = jbox.custom_display {
  graphics = { node = "cd1_node", },
  values = { "/cd1" },
	display_width_pixels = 30,
	display_height_pixels = 10,
	draw_function = "draw_cd1"
}

front_widgets[#front_widgets + 1] = jbox.custom_display {
  graphics = { node = "cd2_node", },
	display_width_pixels = 35,
	display_height_pixels = 40,
  values = { "/cd2_1", "/cd2_2" },
	invalidate_function = "invalidate_cd2",
	draw_function = "draw_cd2",
	gesture_function = "gesture_cd2",
  show_remote_box = false,
  show_automation_rect = false,
  visibility_switch = "/cd2_switch",
  visibility_values = { 8, 1 },
  background = jbox.image{ path = "cd2_bg" }
}

front = jbox.panel{
  graphics = {
    node = "Panel_front_bg",
  },
  widgets = front_widgets
}

--------------------------------------------------------------------------
-- back_widgets
--------------------------------------------------------------------------
back_widgets = {}

back_widgets[#back_widgets + 1] = jbox.audio_input_socket {
  graphics = { node = "au_in_1_node", }, socket = "/audio_inputs/au_in_1",
}
back_widgets[#back_widgets + 1] = jbox.audio_output_socket {
  graphics = { node = "au_out_1_node", }, socket = "/audio_outputs/au_out_1",
}
back_widgets[#back_widgets + 1] = jbox.cv_input_socket {
  graphics = { node = "cv_in_1_node", }, socket = "/cv_inputs/cv_in_1",
}
back_widgets[#back_widgets + 1] = jbox.cv_output_socket {
  graphics = { node = "cv_out_1_node", }, socket = "/cv_outputs/cv_out_1",
}
back_widgets[#back_widgets + 1] = jbox.placeholder {
  graphics = { node = "ph1_node", }
}

back = jbox.panel{
  graphics = {
    node = "Panel_back_bg",
  },
  widgets = back_widgets
}
