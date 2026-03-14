# TouchGFX Mapping Guide (Designer Work)

Last updated: 2026-03-13
Scope: current active project at Appli/TouchGFX

## 1. Which files are safe to edit

### Safe for custom mapping logic (kept after Generate Code)
- Appli/TouchGFX/gui/src/**
- Appli/TouchGFX/gui/include/**
- Appli/TouchGFX/gui/src/model/Model.cpp
- Appli/TouchGFX/gui/include/gui/model/Model.hpp

### Regenerated (changes are overwritten by Generate Code)
- Appli/TouchGFX/generated/**
- Appli/TouchGFX/target/generated/**
- Appli/TouchGFX/generated/texts/**

Rule: never place manual mapping logic in generated files.

## 2. Current mapping status (important)

Current custom mapping code is almost empty.
- Screen1View.cpp, Screen2View.cpp, Screen3View.cpp only call Base setup/teardown.
- Screen1Presenter.cpp, Screen2Presenter.cpp, Screen3Presenter.cpp are empty activate/deactivate.
- Model.cpp tick() is empty.

This means UI behavior currently comes mainly from Designer Interactions in generated base code.

## 3. Current screen transitions actually generated

Defined in:
- Appli/TouchGFX/generated/gui_generated/include/gui_generated/common/FrontendApplicationBase.hpp
- Appli/TouchGFX/generated/gui_generated/src/common/FrontendApplicationBase.cpp

Available transition APIs:
- gotoScreen1ScreenNoTransition()
- gotoScreen2ScreenNoTransition()

Note:
- Screen3 classes exist in heap includes, but no generated gotoScreen3... transition API exists in current build.

## 4. Designer interaction mappings currently active

### Screen1
Source file:
- Appli/TouchGFX/generated/gui_generated/src/screen1_screen/Screen1ViewBase.cpp

Active button callback:
- Prog_mode click -> application().gotoScreen2ScreenNoTransition()

Objects to keep stable if you rely on current names:
- Prog_mode
- buttonWithLabel1_1
- buttonWithLabel1_2
- buttonWithLabel1_3
- textProgress1
- textProgress1_1
- textProgress1_2
- toggleButton1
- buttonWithIcon1
- buttonWithIcon1_1
- slider1

### Screen2
Source file:
- Appli/TouchGFX/generated/gui_generated/src/screen2_screen/Screen2ViewBase.cpp

Active button callback:
- Main_button click -> application().gotoScreen1ScreenNoTransition()

Objects to keep stable if you rely on current names:
- Main_button
- Main_button_1
- Main_button_1_1
- Main_button_1_1_1
- SlowPosition
- SlowPosition_1
- SlowPosition_1_1
- SlowPosition_1_2
- SlowPosition_2
- SlowPosition_3
- SlowPosition_4
- TargetPosition
- DelyTime

### Screen3
Source file:
- Appli/TouchGFX/generated/gui_generated/src/screen3_screen/Screen3ViewBase.cpp

Current status:
- No generated button callback handler in Screen3 base.
- Main_button exists visually but has no generated action mapping now.

Objects present:
- Main_button
- buttonWithLabel1_2
- SlowPosition_1_1

## 5. Text and typography mapping references

Source file:
- Appli/TouchGFX/assets/texts/texts.xml

Important points:
- Most labels are auto-generated IDs like __SingleUse_xxxx.
- If text content changes in Designer, IDs may be regenerated.
- Do not hardcode __SingleUse IDs in custom user logic.

## 6. Designer checklist before flashing

1. Open exactly this file in Designer:
   - Appli/TouchGFX/MyApplication_4.touchgfx
2. Edit UI
3. Click Generate Code and wait until completion
4. Verify generated timestamp is newer than the touchgfx file
5. Run full build+flash:
   - powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_and_flash.ps1

## 7. If you want stable custom mapping in future

Recommended method:
1. Keep backend logic in gui/src and presenters/model.
2. In Designer, create interactions that call generated virtual hooks (not direct generated-only logic) when possible.
3. Avoid renaming screen/widget IDs that are referenced from custom user files.
4. After major Designer changes, compile immediately and fix name mismatches once.

## 8. Quick diagnosis when UI seems not updated

If touchgfx file time is newer than generated files:
- Generate Code did not actually apply.
- Re-run Generate Code in Designer and wait for completion.

If generated files are updated but LCD still old:
- run full build and full flash, not UI-only flash.
