# UI System & Selection Tool Implementation Plan

## Overview
Create a basic toolbar UI with Select, Line, and Polyline tools. Implement a full-featured Selection Tool with single/multi-select, modifier keys (Shift to add, Alt to remove), and marquee selection.

---

## Input Mapping Context System

The project uses a layered Input Mapping Context (IMC) system to handle different key meanings in different modes/tools.

### Context Hierarchy (higher priority = processed first)
| Layer | Priority | Description |
|-------|----------|-------------|
| Tool | 2 | Tool-specific bindings (Select, Draw) |
| Mode | 1 | Mode-specific bindings (2D Drafting, 3D Navigation) |
| Base | 0 | Always active, common actions |

### Input Mapping Contexts to Create

#### IMC_Base (Priority 0 - Always Active)
| Action | Key | Description |
|--------|-----|-------------|
| IA_MouseMove | Mouse XY | Virtual cursor movement |
| IA_MouseWheel | Mouse Wheel | Zoom |
| IA_ToggleView | Tab | Toggle 2D/3D mode |
| IA_Cancel | Escape | Context-sensitive cancel |
| IA_ModifierShift | Left Shift | Shift modifier state |
| IA_ModifierAlt | Left Alt | Alt modifier state |
| IA_ToolSelect | V | Switch to Select tool |
| IA_ToolLine | L | Switch to Line tool |
| IA_ToolPolyline | P | Switch to Polyline tool |

#### IMC_2DDrafting (Priority 1 - 2D Mode)
| Action | Key | Description |
|--------|-----|-------------|
| IA_PrimaryAction | Left Click | Place point / Select |
| IA_SecondaryAction | Right Click | Pan / Cancel |
| IA_MiddleMouseDrag | Middle Mouse | Pan |
| IA_NumericDigit | 0-9 | Numeric entry (with Scalar modifiers) |
| IA_NumericDecimal | Period | Decimal point |
| IA_NumericBackspace | Backspace | Delete digit |
| IA_NumericCommit | Enter | Confirm numeric input |
| IA_NumericSwitchField | Tab | Switch Length/Angle |
| IA_CycleUnits | U | Cycle units (cm/m/in/ft) |

#### IMC_3DNavigation (Priority 1 - 3D Mode)
| Action | Key | Description |
|--------|-----|-------------|
| IA_PrimaryAction | Left Click | Select |
| IA_SecondaryAction | Right Click | Orbit / Look |
| IA_Orbit | Alt + Left Drag | Orbit camera |
| IA_FlyForward | W/S | Fly forward/back |
| IA_FlyStrafe | A/D | Fly left/right |
| IA_FlyVertical | Q/E | Fly up/down |

#### IMC_SelectTool (Priority 2 - Select Tool Active)
No additional bindings needed - modifiers handled at base level.
The Select tool interprets:
- Shift = Add to selection
- Alt = Remove from selection

#### IMC_DrawTool (Priority 2 - Line/Polyline Tool Active)
No additional bindings needed - modifiers handled at base level.
The Draw tool interprets:
- Shift = Snap toggle (via SnapModifierMode setting)

### Input Actions to Create

| Action | Value Type | Description |
|--------|------------|-------------|
| IA_MouseMove | Axis2D (Vector2D) | Mouse delta |
| IA_MouseWheel | Axis1D (Float) | Scroll delta |
| IA_ToggleView | Digital (Bool) | View mode toggle |
| IA_Cancel | Digital (Bool) | Cancel/Escape |
| IA_PrimaryAction | Digital (Bool) | Left click |
| IA_SecondaryAction | Digital (Bool) | Right click |
| IA_MiddleMouseDrag | Digital (Bool) | Middle mouse |
| IA_ModifierShift | Digital (Bool) | Shift key |
| IA_ModifierAlt | Digital (Bool) | Alt key |
| IA_ModifierCtrl | Digital (Bool) | Ctrl key |
| IA_NumericDigit | Axis1D (Float) | Digits 0-9 (use Scalar modifiers) |
| IA_NumericDecimal | Digital (Bool) | Period key |
| IA_NumericBackspace | Digital (Bool) | Backspace key |
| IA_NumericCommit | Digital (Bool) | Enter key |
| IA_NumericSwitchField | Digital (Bool) | Tab key |
| IA_CycleUnits | Digital (Bool) | U key |
| IA_ToolSelect | Digital (Bool) | V key |
| IA_ToolLine | Digital (Bool) | L key |
| IA_ToolPolyline | Digital (Bool) | P key |
| IA_Orbit | Digital (Bool) | Alt + Left Mouse (3D) |
| IA_FlyForward | Axis1D (Float) | W/S keys |
| IA_FlyStrafe | Axis1D (Float) | A/D keys |
| IA_FlyVertical | Axis1D (Float) | Q/E keys |

---

## Phase 1: Selection Tool Implementation

### 1.1 Create RTPlanSelectTool
**File**: `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanSelectTool.h`
**File**: `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanSelectTool.cpp`

- State machine:
  - `Idle` - Waiting for input
  - `MarqueeDragging` - User is dragging a selection rectangle
  - `Selected` - Items are selected, waiting for next action

- Selection data:
  - `TSet<FGuid> SelectedWallIds`
  - `TSet<FGuid> SelectedOpeningIds`
  - Delegate `OnSelectionChanged` to notify UI

- Input handling:
  - **Click (no drag)**: Hit test at cursor, select/deselect based on modifiers
  - **Click + Drag**: Start marquee selection
  - **Shift + Click**: Add to selection
  - **Alt + Click**: Remove from selection

### 1.2 Extend FRTPointerEvent for Modifier Keys
**File**: `Plugins/RTPlanInput/Source/RTPlanInput/Public/RTPlanInputData.h`

Add modifier key states:
```cpp
bool bShiftDown = false;   // Add to selection
bool bAltDown = false;     // Remove from selection
bool bCtrlDown = false;    // Reserved for future
```

### 1.3 Update Player Controller for Modifier Keys
**File**: `Source/ArchVis/Public/ArchVisInputConfig.h`
**File**: `Source/ArchVis/Private/ArchVisPlayerController.cpp`

- Add Input Actions for Shift and Alt modifiers
- Track modifier key states
- Pass modifier states in FRTPointerEvent

### 1.4 Implement Hit Testing
**File**: `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Public/RTPlanSpatialIndex.h`

Add method:
```cpp
FGuid HitTestWall(const FVector2D& Point, float Tolerance = 10.0f) const;
TArray<FGuid> HitTestWallsInRect(const FVector2D& Min, const FVector2D& Max) const;
```

### 1.5 Selection Visualization
- Highlight selected walls with different color/outline
- Draw marquee rectangle during drag

---

## Phase 2: Input Config Updates

### 2.1 Add Selection Modifier Input Actions
**File**: `Source/ArchVis/Public/ArchVisInputConfig.h`

```cpp
// Selection modifiers
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
TObjectPtr<UInputAction> IA_SelectionAddModifier;  // Left Shift

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
TObjectPtr<UInputAction> IA_SelectionRemoveModifier;  // Left Alt
```

### 2.2 Editor Setup Required
Create in Unreal Editor:
- `IA_SelectionAddModifier` - Digital (Bool), map to Left Shift
- `IA_SelectionRemoveModifier` - Digital (Bool), map to Left Alt

---

## Phase 3: UI Toolbar Implementation

### 3.1 Extend RTPlanToolbar Widget
**File**: `Plugins/RTPlanUI/Source/RTPlanUI/Public/RTPlanToolbar.h`

Add tool enum and selection tracking:
```cpp
UENUM(BlueprintType)
enum class ERTPlanToolType : uint8
{
    None,
    Select,
    Line,
    Polyline
};

UFUNCTION(BlueprintCallable)
void SelectToolByType(ERTPlanToolType ToolType);

UFUNCTION(BlueprintCallable)
ERTPlanToolType GetActiveToolType() const;
```

### 3.2 Create Blueprint Widget
- Create `WBP_Toolbar` widget blueprint extending `URTPlanToolbar`
- Add buttons for:
  - Select Tool (cursor icon)
  - Line Tool (line icon)
  - Polyline Tool (polyline icon)
- Highlight active tool button
- Wire buttons to `SelectToolByType()`

### 3.3 Integrate Toolbar into HUD
**File**: `Source/ArchVis/Private/ArchVisHUD.cpp` or Blueprint

- Spawn toolbar widget on BeginPlay
- Position at top or left of screen
- Pass ToolManager reference

---

## Phase 4: Tool Manager Updates

### 4.1 Add Tool Type Tracking
**File**: `Plugins/RTPlanTools/Source/RTPlanTools/Public/RTPlanToolManager.h`

```cpp
UFUNCTION(BlueprintCallable)
void SelectToolByType(ERTPlanToolType ToolType);

UFUNCTION(BlueprintCallable)
ERTPlanToolType GetActiveToolType() const;

// Get current selection (from SelectTool)
UFUNCTION(BlueprintCallable)
TArray<FGuid> GetSelectedWallIds() const;
```

### 4.2 Selection State Access
Expose selection state for property panel and other systems.

---

## Phase 5: Selection Visualization

### 5.1 Draw Selected Walls
**File**: `Source/ArchVis/Private/ArchVisHUD.cpp`

- Query selected wall IDs from SelectTool
- Draw selected walls with highlight color
- Draw selection handles at vertices

### 5.2 Draw Marquee Rectangle
- During marquee drag, draw dashed rectangle from start to current cursor

---

## Implementation Order

1. **Phase 1.2**: Add modifier keys to FRTPointerEvent
2. **Phase 2**: Update InputConfig and PlayerController for modifiers
3. **Phase 1.4**: Add hit testing to SpatialIndex
4. **Phase 1.1**: Create RTPlanSelectTool
5. **Phase 4**: Update ToolManager
6. **Phase 3**: Create/extend toolbar UI
7. **Phase 5**: Selection visualization
8. **Phase 1.5**: Polish and testing

---

## Files to Create/Modify

### New Files:
- `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanSelectTool.h`
- `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanSelectTool.cpp`

### Modified Files:
- `Plugins/RTPlanInput/Source/RTPlanInput/Public/RTPlanInputData.h`
- `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Public/RTPlanSpatialIndex.h`
- `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`
- `Plugins/RTPlanTools/Source/RTPlanTools/Public/RTPlanToolManager.h`
- `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`
- `Plugins/RTPlanUI/Source/RTPlanUI/Public/RTPlanToolbar.h`
- `Plugins/RTPlanUI/Source/RTPlanUI/Private/RTPlanToolbar.cpp`
- `Source/ArchVis/Public/ArchVisInputConfig.h`
- `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- `Source/ArchVis/Private/ArchVisHUD.cpp`

### Editor Assets to Create:
- `IA_SelectionAddModifier` - Input Action (Digital/Bool)
- `IA_SelectionRemoveModifier` - Input Action (Digital/Bool)
- `WBP_Toolbar` - Widget Blueprint (optional, can also do in C++)

---

## Checklist

### Phase 1: Selection Tool
- [ ] Add modifier key fields to FRTPointerEvent
- [ ] Add IA_SelectionAddModifier and IA_SelectionRemoveModifier to InputConfig
- [ ] Track modifier key states in PlayerController
- [ ] Pass modifier states in GetPointerEvent()
- [ ] Add HitTestWall() to SpatialIndex
- [ ] Add HitTestWallsInRect() to SpatialIndex
- [ ] Create RTPlanSelectTool header
- [ ] Create RTPlanSelectTool implementation
- [ ] Implement single-click selection
- [ ] Implement Shift+click add to selection
- [ ] Implement Alt+click remove from selection
- [ ] Implement marquee drag selection

### Phase 2: Tool Manager
- [ ] Add ERTPlanToolType enum
- [ ] Add SelectToolByType() method
- [ ] Add GetActiveToolType() method
- [ ] Add GetSelectedWallIds() method

### Phase 3: Toolbar UI
- [ ] Extend RTPlanToolbar with tool type methods
- [ ] Create WBP_Toolbar widget (or C++ widget)
- [ ] Add Select button
- [ ] Add Line button
- [ ] Add Polyline button
- [ ] Wire buttons to SelectToolByType()
- [ ] Highlight active tool button

### Phase 4: Visualization
- [ ] Draw selected walls with highlight in HUD
- [ ] Draw marquee rectangle during drag
- [ ] Draw selection handles at vertices

### Phase 5: Line/Polyline Tools
- [ ] Ensure Line tool works (already exists)
- [ ] Ensure Polyline mode toggle works
- [ ] Add tool switching from toolbar

