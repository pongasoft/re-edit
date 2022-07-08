format_version = "1.0"

-- front
front = {
  Bg = {
    { path = "Panel_Front" },
  },

  TapeFront = {
    offset = { 52, 147 },
    { path = "Tape_Vertical_1frames" },
  },

  -- Main LCD
  MainLCD = {
    offset = { 1710, 50 },
    { path = "Main_LCD" },
  },

  -- Main LCD
  MainLCDScrollbar = {
    offset = { 1710, 575 },
    { path = "Main_LCD_Scrollbar" },
  },

  -- Input Paused (on/off)
  InputPaused = {
    offset = { 3070, 85 },
    { path = "InputPaused_4frames", frames = 4 }
  },

  -- Zoom Factor (X)
  ZoomFactorX = {
    offset = { 3140, 400},
    { path = "Knob_01_mini_63frames", frames = 63 }
  },

  -- Zoom Factor (Y)
  ZoomFactorY = {
    offset = { 3414, 440},
    { path = "Knob_17_matte_63frames", frames = 63 }
  },

  -- Screen On
  ScreenOn = {
    offset = { 3425, 83},
    { path = "Button_mini_translucent_4frames", frames = 4 }
  },

  -- Midi On
  MidiOn = {
    offset = { 1315, 83},
    { path = "Button_mini_translucent_4frames", frames = 4 }
  },

  -- Color
  CVIn1Color = {
    offset = { 3425, 273},
    { path = "Button_mini_translucent_2frames", frames = 2 }
  },

  CVIn1MinMaxAutoReset = {
    offset = { 1010, 325},
    { path = "Knob_01_mini_63frames", frames = 63 }
  },

  -- display scale
  CVIn1DisplayScale = {
    offset = { 695, 300 },
    { path = "Toggle_Fader_3frames", frames = 3 }
  },

  -- momentary button to reset min/max values
  CVIn1MinMaxReset = {
    offset = { 687, 530 },
    { path = "Button_mini_translucent_2frames", frames = 2 }
  },

  CVIn1Value = {
    offset = { 315, 310 },
    { path = "Display_CV_Value" }
  },

  CVIn1MinValue = {
    offset = { 315, 535 },
    { path = "Display_CV_Value" }
  },

  CVIn1MaxValue = {
    offset = { 315, 460 },
    { path = "Display_CV_Value" }
  },

  CVIn1ValueAsKeyboard = {
    offset = { 1490, 60 },
    { path = "Display_CV_Note_Portrait" }
  },

  CVIn1ValueAsNote = {
    offset = { 1490, 478 },
    { path = "Display_CV_Value_As" }
  },

  CVIn1ValueAsGate = {
    offset = { 1490, 560 },
    { path = "Display_CV_Value_As" }
  },
}

-- back
back = {
  Bg = {
    { path = "Panel_Back" },
  },

  TapeBack = {
    offset = { 270, 275 },
    { path = "Tape_Horizontal_1frames" },
  },

  Placeholder = {
    offset = { 270, 575 },
    { path = "Placeholder" }
  },

  CVIn1 = {
    offset = { 1085, 270},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVIn1TrimKnob = {
    offset = { 1070, 475},
    { path = "TrimKnob" }
  },

  CVOut1 = {
    offset = { 1425, 265},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOut1Type = {
    offset = { 1430, 357 },
    { path = "Toggle_Fader_4frames", frames = 4 }
  },

  CVOut2 = {
    offset = { 1535, 265},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOut2Type = {
    offset = { 1540, 357 },
    { path = "Toggle_Fader_4frames", frames = 4 }
  },

  CVOut3 = {
    offset = { 1645, 265},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOut3Type = {
    offset = { 1650, 357 },
    { path = "Toggle_Fader_4frames", frames = 4 }
  },

  CVOut4 = {
    offset = { 1755, 265},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOut4Type = {
    offset = { 1760, 357 },
    { path = "Toggle_Fader_4frames", frames = 4 }
  },


  CVOutValue1 = {
    offset = { 2460, 265},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOutMaxValue1 = {
    offset = { 2460, 380},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

  CVOutMinValue1 = {
    offset = { 2460, 495},
    { path = "Cable_Attachment_CV_01_1frames" }
  },

}

-- folded front
folded_front = {
  Bg = {
    { path = "Panel_Folded_Front" },
  },

  TapeFoldedFront = {
    offset = { 440, 40 },
    { path = "Tape_Horizontal_1frames" },
  },

  -- momentary button to reset min/max values
  CVIn1MinMaxReset = {
    offset = { 2287, 40 },
    { path = "Button_mini_translucent_2frames", frames = 2 }
  },

  CVIn1Value = {
    offset = { 1000, 40 },
    { path = "Display_CV_Value" }
  },

  CVIn1MinValue = {
    offset = { 1605, 40 },
    { path = "Display_CV_Value" }
  },

  CVIn1MaxValue = {
    offset = { 1955, 40 },
    { path = "Display_CV_Value" }
  },

  CVIn1ValueAsKeyboard = {
    offset = { 2850, 40 },
    { path = "Display_CV_Note_Landscape" }
  },

  CVIn1ValueAsNote = {
    offset = { 3120, 40 },
    { path = "Display_CV_Value_As" }
  },

  CVIn1ValueAsGate = {
    offset = { 3290, 40 },
    { path = "Display_CV_Value_As" }
  },

  -- Midi On
  MidiOn = {
    offset = { 3482, 40},
    { path = "Button_mini_translucent_4frames", frames = 4 }
  },
}

-- folded back
folded_back = {
  Bg = {
    { path = "Panel_Folded_Back" },
  },
  TapeFoldedBack = {
    offset = { 440, 40 },
    { path = "Tape_Horizontal_1frames" },
  },

  CableOrigin = {
    offset = { 1875, 75 },
  },
}
