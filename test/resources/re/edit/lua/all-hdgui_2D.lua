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

front_widgets[#front_widgets + 1] = jbox.sequence_fader {
  graphics = { node = "sf1_node" },
  value = "/sf1"
}

front_widgets[#front_widgets + 1] = jbox.sequence_fader {
  graphics = { node = "sf2_node" },
  value_switch = "/sf2_switch",
  values = { "/sf2_v1" },
  orientation = "horizontal",
  inverted = true,
  inset1 = 10,
  inset2 = 20,
  handle_size = 30,
  visibility_switch = "/sf2_switch",
  visibility_values = { 3, 1 },
  tooltip_position = "top",
  tooltip_template = jbox.ui_text("sf2_tooltip_template"),
  show_remote_box = false,
  show_automation_rect = false,
}

front_widgets[#front_widgets + 1] = jbox.momentary_button {
  graphics = { node = "mb1_node", },
  value = "/mb1",
}

front_widgets[#front_widgets + 1] = jbox.momentary_button {
  graphics = { node = "mb2_node", },
  value = "/mb2",
  visibility_switch = "/mb2_switch",
  visibility_values = { 7, 2 },
  tooltip_position = "center",
  tooltip_template = jbox.ui_text("mb2_tooltip_template"),
  show_remote_box = false,
  show_automation_rect = false,
}

front_widgets[#front_widgets + 1] = jbox.toggle_button {
  graphics = { node = "tb1_node", },
  value = "/tb1",
}

front_widgets[#front_widgets + 1] = jbox.toggle_button {
  graphics = { node = "tb2_node", },
  value = "/tb2",
  visibility_switch = "/tb2_switch",
  visibility_values = { 5, 0 },
  tooltip_position = "top_right",
  tooltip_template = jbox.ui_text("tb2_tooltip_template"),
  show_remote_box = false,
  show_automation_rect = false,
}

front_widgets[#front_widgets + 1] = jbox.step_button {
  graphics = { node = "sb1_node", },
  value = "/sb1",
}

front_widgets[#front_widgets + 1] = jbox.step_button {
  graphics = { node = "sb2_node", },
  value = "/sb2",
  visibility_switch = "/sb2_switch",
  visibility_values = { 6, 5 },
  tooltip_position = "no_tooltip",
  tooltip_template = jbox.ui_text("sb2_tooltip_template"),
  increasing = false,
  show_remote_box = false,
  show_automation_rect = false,
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
back_widgets[#back_widgets + 1] = jbox.cv_trim_knob {
  graphics = { node = "cv_trim_knob_node", }, socket = "/cv_inputs/cv_trim_1",
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
