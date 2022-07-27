format_version = "2.0"

-- local maxArraySize = 256
--
-- local mainLCDCustomDisplayValues = {
--   "/custom_properties/prop_cv_in_1_state",
--   "/custom_properties/prop_input_paused",
--   "/custom_properties/prop_input_page_offset",
--   "/custom_properties/prop_cv_in_1_color",
--   "/custom_properties/prop_screen_on",
--   "/custom_properties/prop_plus_05_axis",
--   "/custom_properties/prop_array_start",
-- }
--
-- for i = 1, maxArraySize do
--   mainLCDCustomDisplayValues[#mainLCDCustomDisplayValues + 1] = "/custom_properties/prop_array_" .. i
-- end
--
-- local commonWidgets = {
--   -- cv in 1 min/max reset
--   jbox.momentary_button{
--     graphics = {
--       node = "CVIn1MinMaxReset",
--     },
--     value = "/custom_properties/prop_cv_in_1_min_max_reset"
--   },
--
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1Value",
--     },
--     background = jbox.image{path = "Display_CV_Value_Background"},
--     display_width_pixels = math.floor(300 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1ValueDraw",
--     values = {
--       "/custom_properties/prop_cv_in_1_value",
--       "/custom_properties/prop_cv_in_1_value_int",
--       "/custom_properties/prop_cv_in_1_display_scale",
--       "/custom_properties/prop_cv_in_1_color",
--     }
--   },
--
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1MinValue",
--     },
--     background = jbox.image{path = "Display_CV_Value_Background"},
--     display_width_pixels = math.floor(300 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1MinValueDraw",
--     values = {
--       "/custom_properties/prop_cv_in_1_min_value",
--       "/custom_properties/prop_cv_in_1_min_value_int",
--       "/custom_properties/prop_cv_in_1_display_scale",
--       "/custom_properties/prop_cv_in_1_color",
--       "/custom_properties/prop_cv_in_1_min_max_reset",
--     }
--   },
--
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1MaxValue",
--     },
--     background = jbox.image{path = "Display_CV_Value_Background"},
--     display_width_pixels = math.floor(300 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1MaxValueDraw",
--     values = {
--       "/custom_properties/prop_cv_in_1_max_value",
--       "/custom_properties/prop_cv_in_1_max_value_int",
--       "/custom_properties/prop_cv_in_1_display_scale",
--       "/custom_properties/prop_cv_in_1_color",
--       "/custom_properties/prop_cv_in_1_min_max_reset",
--     }
--   },
--
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1ValueAsGate",
--     },
--     background = jbox.image{path = "Display_CV_Value_As_Background"},
--     display_width_pixels = math.floor(160 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1ValueAsGateDraw",
--     values = {
--       "/custom_properties/prop_midi_on",
--       "/custom_properties/prop_cv_in_1_value",
--       "/custom_properties/prop_cv_in_1_value_int",
--       "/custom_properties/prop_cv_in_1_color",
--     }
--   },
--
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1ValueAsNote",
--     },
--     background = jbox.image{path = "Display_CV_Value_As_Background"},
--     display_width_pixels = math.floor(160 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1ValueAsNoteDraw",
--     values = {
--       "/custom_properties/prop_midi_on",
--       "/custom_properties/prop_cv_in_1_state",
--       "/custom_properties/prop_cv_in_1_value_as_note",
--       "/custom_properties/prop_cv_in_1_color",
--     }
--   },
--
--   -- MidiOn
--   jbox.toggle_button {
--     graphics = {
--       node = "MidiOn",
--     },
--     value = "/custom_properties/prop_midi_on",
--   },
--
--
-- }

local frontWidgets = {
  -- device name / tape
--   jbox.device_name {
--     graphics = {
--       node = "TapeFront",
--     },
--   },

--   -- Main LCD
--   jbox.custom_display{
--     graphics = {
--       node = "MainLCD",
--     },
--     background = jbox.image{path = "Main_LCD_Background"},
--     display_width_pixels = math.floor(1280 / 5),
--     display_height_pixels = math.floor(510 / 5),
--     draw_function = "MainLCDDraw",
--     gesture_function = "MainLCDGesture",
--     values = mainLCDCustomDisplayValues
--   },
--
--   -- Main LCD Scrollbar
--   jbox.custom_display{
--     graphics = {
--       node = "MainLCDScrollbar",
--     },
--     background = jbox.image{path = "Main_LCD_Scrollbar_Background"},
--     display_width_pixels = math.floor(1280 / 5),
--     display_height_pixels = math.floor(80 / 5),
--     draw_function = "MainLCDScrollbarDraw",
--     gesture_function = "MainLCDScrollbarGesture",
--     values = {
--       "/custom_properties/prop_cv_in_1_state",
--       "/custom_properties/prop_input_paused",
--       "/custom_properties/prop_input_history_offset",
--       "/custom_properties/prop_input_history_offset_rt",
--       "/custom_properties/prop_zoom_factor_x",
--       "/custom_properties/prop_cv_in_1_color",
--       "/custom_properties/prop_screen_on",
--     }
--   },
--
--   -- input paused
--   jbox.toggle_button {
--     graphics = {
--       node = "InputPaused",
--     },
--     value = "/custom_properties/prop_input_paused",
--   },

  -- Zoom Factor (X axis)
  jbox.analog_knob {
    graphics = {
      node = "ZoomFactorX",
    },
    value = "/custom_properties/prop_zoom_factor_x",
  },

  -- Zoom Factor (Y axis)
  jbox.analog_knob {
    graphics = {
      node = "ZoomFactorY",
    },
    value = "/custom_properties/prop_zoom_factor_y",
  },

--   -- Color
--   jbox.step_button {
--     graphics = {
--       node = "CVIn1Color",
--     },
--     value = "/custom_properties/prop_cv_in_1_color",
--   },
--
--   -- ScreenOn
--   jbox.toggle_button {
--     graphics = {
--       node = "ScreenOn",
--     },
--     value = "/custom_properties/prop_screen_on",
--   },

  -- min/max auto reset
  jbox.analog_knob {
    graphics = {
      node = "CVIn1MinMaxAutoReset",
    },
    value = "/custom_properties/prop_cv_in_1_min_max_auto_reset",
  },

--   -- cv in 1 display scale
--   jbox.sequence_fader {
--     graphics = {
--       node = "CVIn1DisplayScale",
--     },
--     value = "/custom_properties/prop_cv_in_1_display_scale",
--     orientation = "vertical",
--     inverted = true,
--     handle_size = 43
--   },
--
--   -- keyboard
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1ValueAsKeyboard",
--     },
--     background = jbox.image{path = "Display_CV_Note_Portrait_Background"},
--     display_width_pixels = math.floor(160 / 5),
--     display_height_pixels = math.floor(410 / 5),
--     draw_function = "CVIn1ValueAsKeyboardDraw",
--     values = {
--       "/custom_properties/prop_midi_on",
--       "/custom_properties/prop_cv_in_1_state",
--       "/custom_properties/prop_cv_in_1_value_as_note",
--       "/custom_properties/prop_cv_in_1_color",
--     }
--   },

}

-- for _, widget in pairs(commonWidgets) do
--   frontWidgets[#frontWidgets + 1] = widget
-- end

front = jbox.panel {
  graphics = {
    node = "Bg",
  },
  widgets = frontWidgets
}

-- back = jbox.panel {
--   graphics = {
--     node = "Bg"
--   },
--   widgets = {
--
--     -- device name / tape
--     jbox.device_name {
--       graphics = {
--         node = "TapeBack",
--       },
--     },
--
--     -- placeholder
--     jbox.placeholder {
--       graphics = {
--         node = "Placeholder",
--       },
--     },
--
--     -- cv in 1
--     jbox.cv_input_socket{
--       graphics = {
--         node = "CVIn1",
--       },
--       socket = "/cv_inputs/cv_in_1",
--     },
--
--     -- cv in 1 trim knob
--     jbox.cv_trim_knob{
--       graphics = {
--         node = "CVIn1TrimKnob",
--       },
--       socket = "/cv_inputs/cv_in_1",
--     },
--
--     -- cv out 1
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOut1",
--       },
--       socket = "/cv_outputs/cv_out_1",
--     },
--
--     -- cv out 1 type
--     jbox.sequence_fader {
--       graphics = {
--         node = "CVOut1Type",
--       },
--       value = "/custom_properties/prop_cv_out_1_type",
--       orientation = "vertical",
--       inverted = true,
--       handle_size = 43
--     },
--
--     -- cv out 2
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOut2",
--       },
--       socket = "/cv_outputs/cv_out_2",
--     },
--
--     -- cv out 2 type
--     jbox.sequence_fader {
--       graphics = {
--         node = "CVOut2Type",
--       },
--       value = "/custom_properties/prop_cv_out_2_type",
--       orientation = "vertical",
--       inverted = true,
--       handle_size = 43
--     },
--
--     -- cv out 3
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOut3",
--       },
--       socket = "/cv_outputs/cv_out_3",
--     },
--
--     -- cv out 3 type
--     jbox.sequence_fader {
--       graphics = {
--         node = "CVOut3Type",
--       },
--       value = "/custom_properties/prop_cv_out_3_type",
--       orientation = "vertical",
--       inverted = true,
--       handle_size = 43
--     },
--
--     -- cv out 4
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOut4",
--       },
--       socket = "/cv_outputs/cv_out_4",
--     },
--
--     -- cv out 4 type
--     jbox.sequence_fader {
--       graphics = {
--         node = "CVOut4Type",
--       },
--       value = "/custom_properties/prop_cv_out_4_type",
--       orientation = "vertical",
--       inverted = true,
--       handle_size = 43
--     },
--
--     -- cv_out_value_1
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOutValue1",
--       },
--       socket = "/cv_outputs/cv_out_value_1",
--     },
--
--     -- cv_out_min_value_1
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOutMinValue1",
--       },
--       socket = "/cv_outputs/cv_out_min_value_1",
--     },
--
--     -- cv_out_max_value_1
--     jbox.cv_output_socket{
--       graphics = {
--         node = "CVOutMaxValue1",
--       },
--       socket = "/cv_outputs/cv_out_max_value_1",
--     },
--   },
-- }
--
-- local foldedFrontWidgets = {
--   -- device name / tape
--   jbox.device_name {
--     graphics = {
--       node = "TapeFoldedFront",
--     },
--   },
--
--   -- keyboard
--   jbox.custom_display{
--     graphics = {
--       node = "CVIn1ValueAsKeyboard",
--     },
--     background = jbox.image{path = "Display_CV_Note_Landscape_Background"},
--     display_width_pixels = math.floor(270 / 5),
--     display_height_pixels = math.floor(75 / 5),
--     draw_function = "CVIn1ValueAsKeyboardDraw",
--     values = {
--       "/custom_properties/prop_midi_on",
--       "/custom_properties/prop_cv_in_1_state",
--       "/custom_properties/prop_cv_in_1_value_as_note",
--       "/custom_properties/prop_cv_in_1_color",
--     }
--   },
-- }
--
-- for _, widget in pairs(commonWidgets) do
--   foldedFrontWidgets[#foldedFrontWidgets + 1] = widget
-- end
--
-- folded_front = jbox.panel {
--   graphics = {
--     node = "Bg",
--   },
--   widgets = foldedFrontWidgets
-- }
--
-- folded_back = jbox.panel {
--   graphics = {
--     node = "Bg",
--   },
--   cable_origin = {
--     node = "CableOrigin",
--   },
--   widgets = {
--     -- device name / tape
--     jbox.device_name {
--       graphics = {
--         node = "TapeFoldedBack",
--       },
--     },
--   },
-- }