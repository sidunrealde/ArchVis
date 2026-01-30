# Wall Properties Widget — Complete Step‑By‑Step Setup (UMG + Sub‑Widgets)

This doc is meant to be followed **top to bottom** without guessing. It explains **exact widget types**, **hierarchy**, **important property settings**, and **Blueprint wiring** to hook your UMG widgets up to the C++ class `URTPlanWallPropertiesWidget`.

> **NEW: RTPlan Subsystem**
> The `URTPlanSubsystem` (Game Instance Subsystem) is now available in `RTPlanCore`.
> It holds the Document and ToolManager references centrally, making Blueprint setup much simpler.
> See **Section 8 → Option C** for the recommended Blueprint-based setup using the subsystem.

> Scope
> - You will build 3 reusable **sub‑widgets**:
>   1) `WBP_DimensionField` (SpinBox + Unit dropdown)
>   2) `WBP_MaterialSelector` (Preview Image + material dropdown)
>   3) `WBP_LabeledCheckbox` (Checkbox + label)
> - Then you will build the main floating panel widget:
>   - `WBP_WallProperties` (parent: `URTPlanWallPropertiesWidget`)
> - Catalog/material authoring is included; this drives the **dropdown previews** and also drives the **runtime wall material application**.

---

## Checklist (what you’ll have when done)

- [ ] `M_WallMaster` master material + a set of `MI_*` instances
- [ ] `DA_WallFinishes` (type: `RTFinishCatalog`) with materials + preview textures
- [ ] `WBP_DimensionField` sub-widget
- [ ] `WBP_MaterialSelector` sub-widget (with thumbnail preview)
- [ ] `WBP_LabeledCheckbox` sub-widget
- [ ] `WBP_WallProperties` main widget (floating, draggable optional)
- [ ] Widget shows when a wall is selected, edits update in real time

---

## 0) Folder layout (recommended)

Create these folders in Content Browser:

- `Content/Data/`
- `Content/Materials/Master/`
- `Content/Materials/Walls/`
- `Content/Textures/Walls/`
- `Content/UI/MaterialPreviews/`
- `Content/UI/Widgets/`

---

## 1) Authoring: materials + preview textures + finish catalog

### 1.1 Create the master wall material (`M_WallMaster`)

**Widget work depends on this because wall surfaces use “finish IDs” that map to actual materials.**

1. Go to `Content/Materials/Master/`
2. Right‑click → **Material** → name it `M_WallMaster`
3. Open `M_WallMaster`
4. Minimum recommended setup:

- `TextureSampleParameter2D` named **BaseColor** → connect RGB to **Base Color**
- `ScalarParameter` named **Tiling** (default 1.0)
- `TextureCoordinate` → Multiply with `Tiling` → feed to BaseColor UV

Optional but recommended:
- `TextureSampleParameter2D` named **Normal** (Sampler Type Normal) → connect to **Normal**
- `ScalarParameter` named **Roughness`** → connect to **Roughness**

5. Save.

### 1.2 Create material instances (the finishes you’ll choose in the dropdown)

1. Go to `Content/Materials/Walls/`
2. For each finish:
   - Right‑click `M_WallMaster` → **Create Material Instance**
   - Move it into `Content/Materials/Walls/`
   - Name examples: `MI_WhitePaint`, `MI_BrickRed`, `MI_WoodOak`
3. Open each `MI_*`:
   - Enable overrides (checkbox) for `BaseColor`, set texture
   - Set `Tiling` (brick maybe 4–8, paint 1, tile 8, wood 2)
   - (Optional) enable Normal and Roughness
4. Save.

### 1.3 Create preview textures for dropdown thumbnails

The widget uses `PreviewTexture` to show a thumbnail next to the dropdown.

You have two options:

**Option A (fast):** reuse the BaseColor texture as preview.

**Option B (clean UI):** create/import separate thumbnails.

If doing Option B:
1. Import 64×64 or 128×128 thumbnails into `Content/UI/MaterialPreviews/`
2. For each preview texture asset:
   - Compression Settings: **UserInterface2D (RGBA)**
   - Mip Gen Settings: **NoMipmaps**
   - Save

### 1.4 Create the finish catalog (`DA_WallFinishes`) (type `RTFinishCatalog`)

This is what:
- feeds the **UI dropdown list**, and
- feeds the **shell actor material assignment** in real time.

1. Go to `Content/Data/`
2. Right‑click → **Miscellaneous** → **Data Asset**
3. Pick class **RTFinishCatalog**
4. Name: `DA_WallFinishes`
5. Open it and add elements to **Finishes** array.

For each finish entry fill:
- **Id**: `WhitePaint` (must be unique, no spaces)
- **Display Name**: “White Paint”
- **Material**: pick the `MI_*` instance
- **Preview Texture**: pick preview (or BaseColor)
- **Category**: optional (Paint/Brick/Wood/Tile)

Set **DefaultFinishId** to something valid (e.g. `WhitePaint`).

Save.

---

## 2) Sub‑Widget 1: `WBP_DimensionField` (SpinBox + Unit dropdown)

### 2.1 Create the Widget Blueprint

1. Go to `Content/UI/Widgets/`
2. Right‑click → **User Interface** → **Widget Blueprint**
3. Parent class: **User Widget**
4. Name: `WBP_DimensionField`

### 2.2 Designer hierarchy (exact)

In the **Designer** tab:

1. Root: keep **Canvas Panel** (or replace with Horizontal Box; either works)
2. Add a **Horizontal Box** named `RootRow` and make it the only child.
3. Inside `RootRow`:

- **Spin Box** (type: `SpinBox`)
  - Name it `ValueSpinBox`
  - Slot:
    - Size: **Fill**
    - Padding: (0,0,8,0)

- **Combo Box (String)** (type: `ComboBoxString`)
  - Name it `UnitCombo`
  - Slot:
    - Size: **Auto**
    - Min Desired Width: 64–80

### 2.3 Set important properties (SpinBox)

Select `ValueSpinBox` and set:

- Min Value: **0** (or whatever you want)
- Max Value: **100000** (large enough)
- Min Slider Value: **0**
- Max Slider Value: **1000** (UI feel)
- Delta: **0.1** (cm precision; change if desired)

### 2.4 Set important properties (UnitCombo)

Select `UnitCombo`:
- Default Options: add exactly:
  - `cm`
  - `m`
  - `in`
  - `ft`
- Selected Option: `cm`

### 2.5 Variables + dispatcher (must create)

Create variables:

- `CurrentValue` (Float)
- `CurrentUnit` (ERTPlanUnit)

Create Event Dispatcher:
- `OnValueCommitted`
  - Parameter: `NewValue` (Float)

### 2.6 Graph wiring (exact)

**A) Event Pre Construct** (keeps widget correct in Designer too)

- Set `ValueSpinBox.Value` = `CurrentValue`
- Set `UnitCombo.SelectedOption` to match `CurrentUnit`:
  - Use `URTPlanWallPropertiesWidget::GetUnitDisplayName(CurrentUnit)`
  - Convert returned Text to String

**B) SpinBox event**

Use `ValueSpinBox → OnValueCommitted`:

- Set `CurrentValue` = incoming Value
- Call dispatcher `OnValueCommitted(Value)`

**C) Unit dropdown event**

Use `UnitCombo → OnSelectionChanged`:

- Switch on String:
  - “cm” → `CurrentUnit = Centimeters`
  - “m” → `CurrentUnit = Meters`
  - “in” → `CurrentUnit = Inches`
  - “ft” → `CurrentUnit = Feet`

> Note: don’t broadcast OnValueCommitted here; the parent widget controls what “unit” means globally (dimension vs skirting units). This sub-widget is only an input control.

### 2.7 Helper functions on this sub-widget (recommended)

Create these functions:

- `SetValue(float NewValue)`:
  - Set `CurrentValue` + set `ValueSpinBox.Value`

- `SetUnitFromEnum(ERTPlanUnit NewUnit)`:
  - Set `CurrentUnit`
  - Update UnitCombo selected option

Save + Compile.

---

## 3) Sub‑Widget 2: `WBP_MaterialSelector` (Preview + ComboBox)

### 3.1 Create the Widget Blueprint

1. `Content/UI/Widgets/` → Create Widget Blueprint
2. Parent: **User Widget**
3. Name: `WBP_MaterialSelector`

### 3.2 Designer hierarchy (exact)

Root (Canvas Panel ok) → add **Horizontal Box** `RootRow`.

Inside `RootRow`:

- **Image** named `PreviewImage`
  - Desired size: 32×32 or 48×48
  - Slot:
    - Size: Auto
    - Padding: (0,0,8,0)
    - Vertical Alignment: Center

- **Combo Box (String)** named `MaterialCombo`
  - Slot:
    - Size: Fill

### 3.3 Variables + dispatcher

Variables:
- `MaterialEntries` (Array of `FRTMaterialEntry`)
- `SelectedMaterialId` (Name)

Event Dispatcher:
- `OnMaterialChanged`
  - Parameter: `NewMaterialId` (Name)

### 3.4 Functions (required)

**Function: `PopulateMaterials(Entries)`**

- Set `MaterialEntries = Entries`
- Clear `MaterialCombo` options
- For each Entry:
  - Add option = Entry.DisplayName (Text → String)

(Recommended) If `Entries` not empty:
- Set selected index to 0
- Set `SelectedMaterialId = Entries[0].MaterialId`
- Call `UpdatePreview()`

**Function: `SetSelectedMaterial(MaterialId)`**

- Set `SelectedMaterialId = MaterialId`
- Find matching entry index (loop)
- Set `MaterialCombo` selected option to entry DisplayName
- Call `UpdatePreview()`

**Function: `UpdatePreview()`**

- Find entry by `SelectedMaterialId`
- If entry PreviewTexture is valid:
  - `PreviewImage → Set Brush From Texture(PreviewTexture, MatchSize = true)`
- Else:
  - Clear brush or set a placeholder

### 3.5 ComboBox selection event

`MaterialCombo → OnSelectionChanged`:

- Determine selected index (or find by selected string)
- Get corresponding entry
- Set `SelectedMaterialId = Entry.MaterialId`
- `UpdatePreview()`
- Broadcast `OnMaterialChanged(SelectedMaterialId)`

Save + Compile.

---

## 4) Sub‑Widget 3: `WBP_LabeledCheckbox`

### 4.1 Create the Widget Blueprint

1. Create Widget Blueprint
2. Parent: **User Widget**
3. Name: `WBP_LabeledCheckbox`

### 4.2 Designer hierarchy (exact)

Horizontal Box `RootRow`:

- **Check Box** named `ValueCheckbox`
- **Text Block** named `LabelText`
  - Slot Padding: (8,0,0,0)

### 4.3 Variables + dispatcher

Variables:
- `Label` (Text)
- `IsChecked` (Boolean)

Event Dispatcher:
- `OnCheckChanged`
  - Parameter: `bIsChecked` (Boolean)

### 4.4 Graph

Pre Construct:
- Set LabelText = Label
- Set ValueCheckbox.IsChecked = IsChecked

ValueCheckbox → OnCheckStateChanged:
- Set IsChecked
- Broadcast OnCheckChanged

Also add `SetChecked(bool)` helper.

Save + Compile.

---

## 5) Main Widget: `WBP_WallProperties` (parent: `URTPlanWallPropertiesWidget`)

### 5.1 Create the Widget Blueprint (IMPORTANT parent)

1. Create Widget Blueprint
2. In Parent Class picker search: **RTPlanWallPropertiesWidget**
3. Name: `WBP_WallProperties`

### 5.2 Designer: main hierarchy (exact)

Root: **Canvas Panel**

Inside Canvas Panel:

1) **Border** named `WindowBorder`
- Anchors: Top‑Left
- Position: X=100, Y=100
- Size: X=380, Y=650
- Padding: 0
- Brush Color: (0.06, 0.06, 0.06, 0.92)

Inside `WindowBorder`:

2) **Vertical Box** named `MainContainer`

Inside `MainContainer`:

3) **Border** named `TitleBar`
- VerticalBox Slot: Auto
- Padding: 8
- Brush Color: (0.10, 0.10, 0.10, 1.0)

Inside `TitleBar`:

4) **Horizontal Box** named `TitleRow`

Inside `TitleRow`:

- **Text Block** named `TitleText`
  - Slot: Fill
  - Text: “Wall Properties”
  - Font Size: 16

- **Button** named `CloseButton`
  - Slot: Auto
  - Inside button: Text “X”

Back in `MainContainer` (below TitleBar):

5) **Scroll Box** named `ContentScroll`
- VerticalBox Slot: Fill
- Padding: 8

Inside `ContentScroll`:

6) **Vertical Box** named `ContentContainer`

Now inside `ContentContainer` add 3 sections:

### 5.2.1 Section: Wall

Add **Text Block** header:
- Name: `Header_Wall`
- Text: `Wall`
- Font: Bold, Size 14
- Padding: (0,8,0,4)

Then add a **Vertical Box** `WallSection`.

Inside `WallSection`, add rows (each row is a Horizontal Box):

**Row template:**
- Horizontal Box
  - TextBlock label (fixed width)
  - sub-widget (fill)

For label TextBlocks:
- Set Min Desired Width: 150
- Vertical Alignment: Center

Rows:

1) Thickness
- Label: “Wall Thickness”
- Sub-widget: `WBP_DimensionField` → name it `Field_WallThickness`

2) Height
- Label: “Wall Height”
- Sub-widget: `WBP_DimensionField` → `Field_WallHeight`

3) Base Height
- Label: “Base Height”
- Sub-widget: `WBP_DimensionField` → `Field_BaseHeight`

4) Left material
- Label: “Left Wall Material”
- Sub-widget: `WBP_MaterialSelector` → `Mat_Left`

5) Right material
- Label: “Right Wall Material”
- Sub-widget: `WBP_MaterialSelector` → `Mat_Right`

6) Left cap material
- Label: “Left Cap Material”
- Sub-widget: `WBP_MaterialSelector` → `Mat_LeftCap`

7) Right cap material
- Label: “Right Cap Material”
- Sub-widget: `WBP_MaterialSelector` → `Mat_RightCap`

8) Top material
- Label: “Top Material”
- Sub-widget: `WBP_MaterialSelector` → `Mat_Top`

Add a **Spacer** height 12.

### 5.2.2 Section: Left Skirting

Header TextBlock `Header_LeftSkirting` (same style).

Vertical Box `LeftSkirtingSection` with rows:

1) Has left skirting
- Sub-widget: `WBP_LabeledCheckbox` → `Chk_HasLeftSkirting`

2) Height
- `WBP_DimensionField` → `Field_LeftSkirtingHeight`

3) Thickness
- `WBP_DimensionField` → `Field_LeftSkirtingThickness`

4) Material
- `WBP_MaterialSelector` → `Mat_LeftSkirting`

5) Top Material
- `WBP_MaterialSelector` → `Mat_LeftSkirtingTop`

6) Cap Material
- `WBP_MaterialSelector` → `Mat_LeftSkirtingCap`

Add Spacer height 12.

### 5.2.3 Section: Right Skirting

Header `Header_RightSkirting`.

Vertical Box `RightSkirtingSection` with rows:

1) Has right skirting
- `WBP_LabeledCheckbox` → `Chk_HasRightSkirting`

2) Height
- `WBP_DimensionField` → `Field_RightSkirtingHeight`

3) Thickness
- `WBP_DimensionField` → `Field_RightSkirtingThickness`

4) Material
- `WBP_MaterialSelector` → `Mat_RightSkirting`

5) Top Material
- `WBP_MaterialSelector` → `Mat_RightSkirtingTop`

6) Cap Material
- `WBP_MaterialSelector` → `Mat_RightSkirtingCap`

### 5.3 Mark variables (critical)

For EVERY sub-widget you need to access in Graph:

1. Click the sub-widget in the hierarchy
2. In Details: check **Is Variable**

Do this for:
- all `Field_*`
- all `Mat_*`
- `Chk_*`
- `TitleText` (optional)

Compile.

---

## 6) Wiring the main widget (Blueprint Graph)

All logic is driven by Blueprint events implemented in the parent C++ class.

### 6.1 Event: `OnMaterialsLoaded`

Goal: push the materials list into every `WBP_MaterialSelector`.

Graph:

1. Add event: **Event OnMaterialsLoaded**
2. Call `GetAvailableMaterials` → store as local variable `Materials`
3. For each material selector call its `PopulateMaterials(Materials)`:

- `Mat_Left`
- `Mat_Right`
- `Mat_LeftCap`
- `Mat_RightCap`
- `Mat_Top`
- `Mat_LeftSkirting`
- `Mat_LeftSkirtingTop`
- `Mat_LeftSkirtingCap`
- `Mat_RightSkirting`
- `Mat_RightSkirtingTop`
- `Mat_RightSkirtingCap`

Finally call `OnWallDataChanged` (custom event call) to refresh selection.

### 6.2 Event: `OnWallDataChanged`

Goal: pull current wall data via getter functions and push into sub-widgets.

1. Add event: **Event OnWallDataChanged**
2. Branch: `HasSelectedWall` (if false → return)
3. Set dimension fields:

- `Field_WallThickness.SetValue( GetWallThickness )`
- `Field_WallHeight.SetValue( GetWallHeight )`
- `Field_BaseHeight.SetValue( GetBaseHeight )`

Skirting values:
- `Field_LeftSkirtingHeight.SetValue( GetLeftSkirtingHeight )`
- `Field_LeftSkirtingThickness.SetValue( GetLeftSkirtingThickness )`
- `Field_RightSkirtingHeight.SetValue( GetRightSkirtingHeight )`
- `Field_RightSkirtingThickness.SetValue( GetRightSkirtingThickness )`

Checkboxes:
- `Chk_HasLeftSkirting.SetChecked( GetHasLeftSkirting )`
- `Chk_HasRightSkirting.SetChecked( GetHasRightSkirting )`

Material selectors:
- `Mat_Left.SetSelectedMaterial( GetLeftWallMaterial )`
- `Mat_Right.SetSelectedMaterial( GetRightWallMaterial )`
- `Mat_LeftCap.SetSelectedMaterial( GetLeftCapMaterial )`
- `Mat_RightCap.SetSelectedMaterial( GetRightCapMaterial )`
- `Mat_Top.SetSelectedMaterial( GetTopMaterial )`
- `Mat_LeftSkirting.SetSelectedMaterial( GetLeftSkirtingMaterial )`
- `Mat_LeftSkirtingTop.SetSelectedMaterial( GetLeftSkirtingTopMaterial )`
- `Mat_LeftSkirtingCap.SetSelectedMaterial( GetLeftSkirtingCapMaterial )`
- `Mat_RightSkirting.SetSelectedMaterial( GetRightSkirtingMaterial )`
- `Mat_RightSkirtingTop.SetSelectedMaterial( GetRightSkirtingTopMaterial )`
- `Mat_RightSkirtingCap.SetSelectedMaterial( GetRightSkirtingCapMaterial )`

### 6.3 Bind sub-widget events → call C++ setter functions

This ensures editing a field updates the wall immediately.

#### 6.3.1 Dimension fields

For each `WBP_DimensionField`:

- Bind `OnValueCommitted(NewValue)` → call correct setter:

| Field | Call |
|---|---|
| Field_WallThickness | `SetWallThickness(NewValue)` |
| Field_WallHeight | `SetWallHeight(NewValue)` |
| Field_BaseHeight | `SetBaseHeight(NewValue)` |
| Field_LeftSkirtingHeight | `SetLeftSkirtingHeight(NewValue)` |
| Field_LeftSkirtingThickness | `SetLeftSkirtingThickness(NewValue)` |
| Field_RightSkirtingHeight | `SetRightSkirtingHeight(NewValue)` |
| Field_RightSkirtingThickness | `SetRightSkirtingThickness(NewValue)` |

> Units note: the Set* functions expect the number in the currently active unit inside `URTPlanWallPropertiesWidget`. If you want per-field units, you can extend the widget later.

#### 6.3.2 Material selectors

Bind `OnMaterialChanged(NewMaterialId)`:

| Selector | Call |
|---|---|
| Mat_Left | `SetLeftWallMaterial(NewMaterialId)` |
| Mat_Right | `SetRightWallMaterial(NewMaterialId)` |
| Mat_LeftCap | `SetLeftCapMaterial(NewMaterialId)` |
| Mat_RightCap | `SetRightCapMaterial(NewMaterialId)` |
| Mat_Top | `SetTopMaterial(NewMaterialId)` |
| Mat_LeftSkirting | `SetLeftSkirtingMaterial(NewMaterialId)` |
| Mat_LeftSkirtingTop | `SetLeftSkirtingTopMaterial(NewMaterialId)` |
| Mat_LeftSkirtingCap | `SetLeftSkirtingCapMaterial(NewMaterialId)` |
| Mat_RightSkirting | `SetRightSkirtingMaterial(NewMaterialId)` |
| Mat_RightSkirtingTop | `SetRightSkirtingTopMaterial(NewMaterialId)` |
| Mat_RightSkirtingCap | `SetRightSkirtingCapMaterial(NewMaterialId)` |

#### 6.3.3 Checkboxes

Bind `OnCheckChanged(bIsChecked)`:

- `Chk_HasLeftSkirting` → `SetHasLeftSkirting(bIsChecked)`
- `Chk_HasRightSkirting` → `SetHasRightSkirting(bIsChecked)`

### 6.4 Selection‑driven show/hide

Implement event **OnSelectionChanged** in the main widget:

- If `HasSelectedWall` is true → `SetVisibility(Visible)`
- Else → `SetVisibility(Collapsed)`

Also update `TitleText` (optional):
- `GetSelectedWallCount` and show “Wall Properties (N selected)”

### 6.5 Close button

Bind `CloseButton.OnClicked` → call `HidePanel()`.

---

## 7) Floating window (dragging) — simplest approach

You can do a Blueprint-only drag.

### 7.1 Add variables on WBP_WallProperties

- `bDragging` (bool)
- `DragStartMouse` (Vector2D)
- `DragStartPos` (Vector2D)

### 7.2 Use these widget events on `TitleBar`

TitleBar must be something that can receive mouse events:
- Use `Border TitleBar`
- In Details:
  - Visibility: **Visible** (not Self Hit Test Invisible)

Graph pseudo:

- `TitleBar.OnMouseButtonDown`:
  - if Left Mouse:
    - bDragging = true
    - DragStartMouse = GetMousePositionOnViewport
    - DragStartPos = WindowBorder (CanvasSlot Position)
    - return Handled

- `TitleBar.OnMouseMove`:
  - if bDragging:
    - NewMouse = GetMousePositionOnViewport
    - Delta = NewMouse - DragStartMouse
    - Set WindowBorder CanvasSlot Position = DragStartPos + Delta
    - return Handled

- `TitleBar.OnMouseButtonUp`:
  - bDragging = false
  - return Handled

---

## 8) Runtime hookup (where you call things)

This section explains **exactly what code to add and where** so that:
1. The widget dropdowns are populated with your materials
2. Changing a material in the widget updates the 3D wall in real time

---

### 8.1 Overview: What needs to happen at runtime

When the game starts, you must:

1. **Create** the `WBP_WallProperties` widget
2. **Call `SetupContext()`** to connect it to the Document and SelectTool
3. **Call `LoadMaterialsFromCatalog()`** to populate the material dropdowns
4. **Call `SetFinishCatalog()`** on the Shell Actor so it knows how to look up materials

You do this **once** at BeginPlay (or equivalent).

---

### 8.2 Where to add this code

You have **3 options** depending on your project setup:

#### Option A: In your custom PlayerController (C++)

If you have a custom `APlayerController` subclass (e.g., `AArchVisPlayerController`):

**Header file** (e.g., `ArchVisPlayerController.h`):

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArchVisPlayerController.generated.h"

class URTPlanDocument;
class URTPlanSelectTool;
class URTPlanWallPropertiesWidget;
class URTFinishCatalog;
class ARTPlanShellActor;

UCLASS()
class AArchVisPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // Assign these in Blueprint defaults or in code
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<URTPlanWallPropertiesWidget> WallPropertiesWidgetClass;

    UPROPERTY(EditAnywhere, Category = "Catalog")
    TSoftObjectPtr<URTFinishCatalog> FinishCatalogAsset;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<URTPlanWallPropertiesWidget> WallPropertiesWidget;
};
```

**Source file** (e.g., `ArchVisPlayerController.cpp`):

```cpp
#include "ArchVisPlayerController.h"
#include "RTPlanWallPropertiesWidget.h"
#include "RTPlanFinishCatalog.h"
#include "RTPlanDocument.h"
#include "RTPlanToolManager.h"
#include "Tools/RTPlanSelectTool.h"
#include "RTPlanShellActor.h"
#include "Kismet/GameplayStatics.h"

void AArchVisPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // --- Get your existing references ---
    // (Replace these with however you access your Document, ToolManager, ShellActor)
    URTPlanDocument* Document = /* your document */;
    URTPlanToolManager* ToolManager = /* your tool manager */;
    URTPlanSelectTool* SelectTool = Cast<URTPlanSelectTool>(ToolManager->GetActiveTool()); // or however you get it
    ARTPlanShellActor* ShellActor = Cast<ARTPlanShellActor>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ARTPlanShellActor::StaticClass()));

    // --- Load the finish catalog ---
    URTFinishCatalog* FinishCatalog = FinishCatalogAsset.LoadSynchronous();

    // --- Create and setup the wall properties widget ---
    if (WallPropertiesWidgetClass && Document && SelectTool)
    {
        WallPropertiesWidget = CreateWidget<URTPlanWallPropertiesWidget>(this, WallPropertiesWidgetClass);
        
        // Connect to document and selection
        WallPropertiesWidget->SetupContext(Document, SelectTool);
        
        // Load materials into dropdowns
        if (FinishCatalog)
        {
            WallPropertiesWidget->LoadMaterialsFromCatalog(FinishCatalog);
        }
        
        // Add to screen (Z-order 10 so it's above other UI)
        WallPropertiesWidget->AddToViewport(10);
        
        // Start hidden (will show when a wall is selected)
        WallPropertiesWidget->HidePanel();
    }

    // --- Connect the shell actor to the catalog ---
    if (ShellActor && FinishCatalog)
    {
        ShellActor->SetFinishCatalog(FinishCatalog);
    }
}
```

**After adding this code:**
1. Open your PlayerController Blueprint (or create one based on this class)
2. In Class Defaults:
   - Set **Wall Properties Widget Class** = `WBP_WallProperties`
   - Set **Finish Catalog Asset** = `DA_WallFinishes`

---

#### Option B: In your GameMode (C++)

Same pattern as above, but in your GameMode's BeginPlay.

---

#### Option C: In Blueprint using the RTPlan Subsystem (RECOMMENDED)

This is the **simplest approach** using the new `URTPlanSubsystem` (Game Instance Subsystem).

**Step 1: Open your PlayerController Blueprint**

1. Open the Blueprint (e.g., `BP_ArchVisController`)
2. Go to the **Event Graph**

**Step 2: Create variables in the Blueprint**

In the "My Blueprint" panel, create these variables:

| Variable Name | Variable Type | Description |
|---------------|---------------|-------------|
| `WallPropertiesWidget` | `RTPlan Wall Properties Widget` (Object Reference) | Stores the widget |
| `ToolManager` | `RTPlan Tool Manager` (Object Reference) | Stores the tool manager |
| `FinishCatalog` | `RT Finish Catalog` (Object Reference) | Stores loaded catalog |

**Step 3: Build the Blueprint graph**

Here's the exact node-by-node setup:

```
┌─────────────────────────────────────────────────────────────────────┐
│                        EVENT BEGINPLAY                              │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 1. GET RTPLAN SUBSYSTEM                                             │
│    Node: "Get RTPlan Subsystem"                                     │
│    Return Value → Promote to local variable: "Subsystem"            │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 2. GET DOCUMENT FROM SUBSYSTEM                                      │
│    Target: Subsystem                                                │
│    Node: "Get Document"                                             │
│    Return Value → Store as local: "Doc"                             │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 3. CREATE THE TOOL MANAGER                                          │
│    Node: "Construct Object from Class"                              │
│    Class: RTPlanToolManager                                         │
│    Outer: Self                                                      │
│    Return Value → SET ToolManager variable                          │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 4. INITIALIZE TOOL MANAGER                                          │
│    Target: ToolManager                                              │
│    Node: "Initialize"                                               │
│    In Doc: Doc                                                      │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 5. STORE TOOL MANAGER IN SUBSYSTEM                                  │
│    Target: Subsystem                                                │
│    Node: "Set Tool Manager"                                         │
│    In Tool Manager: ToolManager                                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 6. GET SELECT TOOL                                                  │
│    Target: ToolManager                                              │
│    Node: "Get Select Tool"                                          │
│    Return Value → Store as local: "SelectTool"                      │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 7. CREATE THE WALL PROPERTIES WIDGET                                │
│    Node: "Create Widget"                                            │
│    Class: WBP_WallProperties                                        │
│    Owning Player: Self                                              │
│    Return Value → SET WallPropertiesWidget variable                 │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 8. ADD WIDGET TO VIEWPORT                                           │
│    Target: WallPropertiesWidget                                     │
│    Node: "Add to Viewport"                                          │
│    Z-Order: 10                                                      │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 9. LOAD FINISH CATALOG                                              │
│    Node: "Load Asset (Blocking)" or "Async Load Asset"              │
│    Asset: DA_WallFinishes (select from dropdown)                    │
│    → Cast to RT Finish Catalog                                      │
│    → SET FinishCatalog variable                                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 10. SETUP WIDGET CONTEXT                                            │
│     Target: WallPropertiesWidget                                    │
│     Node: "Setup Context"                                           │
│     In Document: Doc                                                │
│     In Select Tool: SelectTool                                      │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 11. LOAD MATERIALS INTO WIDGET                                      │
│     Target: WallPropertiesWidget                                    │
│     Node: "Load Materials From Catalog"                             │
│     Catalog: FinishCatalog                                          │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 12. HIDE WIDGET INITIALLY                                           │
│     Target: WallPropertiesWidget                                    │
│     Node: "Hide Panel"                                              │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 13. GET SHELL ACTOR                                                 │
│     Node: "Get Actor of Class"                                      │
│     Actor Class: RTPlanShellActor                                   │
│     Return Value → Store as local: "ShellActor"                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 14. SET DOCUMENT ON SHELL ACTOR                                     │
│     Target: ShellActor                                              │
│     Node: "Set Document"                                            │
│     In Doc: Doc                                                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 15. SET FINISH CATALOG ON SHELL ACTOR                               │
│     Target: ShellActor                                              │
│     Node: "Set Finish Catalog"                                      │
│     In Catalog: FinishCatalog                                       │
└─────────────────────────────────────────────────────────────────────┘
```

**How to find each node in Blueprint:**

| Step | Node Name | How to Find It |
|------|-----------|----------------|
| 1 | Get RTPlan Subsystem | Right-click → search "Get RTPlan Subsystem" |
| 2 | Get Document | Drag from Subsystem → "Get Document" |
| 3 | Construct Object from Class | Right-click → "Construct Object from Class" |
| 4 | Initialize | Drag from ToolManager → "Initialize" |
| 5 | Set Tool Manager | Drag from Subsystem → "Set Tool Manager" |
| 6 | Get Select Tool | Drag from ToolManager → "Get Select Tool" |
| 7 | Create Widget | Right-click → User Interface → "Create Widget" |
| 8 | Add to Viewport | Drag from Widget → "Add to Viewport" |
| 9 | Load Asset | Right-click → Asset Manager → "Load Asset" |
| 10 | Setup Context | Drag from Widget → "Setup Context" |
| 11 | Load Materials From Catalog | Drag from Widget → "Load Materials From Catalog" |
| 12 | Hide Panel | Drag from Widget → "Hide Panel" |
| 13 | Get Actor of Class | Right-click → Game → "Get Actor of Class" |
| 14 | Set Document | Drag from ShellActor → "Set Document" |
| 15 | Set Finish Catalog | Drag from ShellActor → "Set Finish Catalog" |

**Using the Subsystem from anywhere else:**

Once the subsystem is set up, you can access the Document and ToolManager from **any** other Blueprint:

```
Get RTPlan Subsystem → Get Document → (use the document)
Get RTPlan Subsystem → Get Tool Manager Object → Cast to RTPlan Tool Manager → (use the tool manager)
```

---

**Key nodes to search for in Blueprint:**
- `Get RTPlan Subsystem` (the main entry point)
- `Create Widget` (under User Interface)
- `Add to Viewport`
- `Setup Context` (on the WallPropertiesWidget)
- `Load Materials From Catalog` (on the WallPropertiesWidget)
- `Hide Panel` (on the WallPropertiesWidget)
- `Set Finish Catalog` (on the ShellActor)
- `Get Actor of Class`

---

### 8.3 Setting default materials on the Shell Actor

The Shell Actor has two fallback material properties used when a wall doesn't have a finish ID set:

1. Find your **Shell Actor** in the level (or in the Blueprint if you have one)
2. In the Details panel, under **RTPlan | Shell | Default Materials**:
   - **Default Wall Material**: Pick a material (e.g., `MI_WhitePaint`)
   - **Default Skirting Material**: Pick a material (e.g., `MI_WhitePaint`)

These are used as fallbacks only. Once you assign a finish in the widget, that takes priority.

---

### 8.4 How the data flows (for understanding)

```
┌─────────────────────────────────────────────────────────────────────┐
│  YOU change "Left Wall Material" dropdown in WBP_WallProperties    │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  WBP_MaterialSelector broadcasts OnMaterialChanged(NewMaterialId)  │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  WBP_WallProperties calls SetLeftWallMaterial(NewMaterialId)       │
│  (this is a C++ function on URTPlanWallPropertiesWidget)           │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  C++ creates a URTCmdAddWall command with the new FinishLeftId     │
│  and submits it to the Document                                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Document broadcasts OnPlanChanged                                  │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  ARTPlanShellActor::OnPlanChanged() is called                      │
│  → calls RebuildAll()                                               │
│  → calls ApplyWallMaterials() for each wall                        │
│  → looks up FinishLeftId in FinishCatalog                          │
│  → calls MeshComp->SetMaterial(0, FoundMaterial)                   │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Wall mesh updates visually with new material                       │
└─────────────────────────────────────────────────────────────────────┘
```

---

### 8.5 Checklist for Section 8

- [ ] Created/opened your PlayerController or GameMode
- [ ] Added code to create `WBP_WallProperties` widget
- [ ] Called `SetupContext(Document, SelectTool)` on the widget
- [ ] Called `LoadMaterialsFromCatalog(DA_WallFinishes)` on the widget
- [ ] Called `AddToViewport(10)` on the widget
- [ ] Called `HidePanel()` on the widget (starts hidden)
- [ ] Got reference to the Shell Actor
- [ ] Called `SetFinishCatalog(DA_WallFinishes)` on the Shell Actor
- [ ] Set **Default Wall Material** on the Shell Actor (in Details panel)
- [ ] Set **Default Skirting Material** on the Shell Actor (in Details panel)

---

## 9) Material slot mapping (how widget properties map to mesh)

The mesh builder now creates geometry with **11 material slots** - one for each wall surface you can configure in the widget.

### Current mapping

| Slot | Geometry | Widget Property |
|------|----------|-----------------|
| 0 | Left wall face | `Left Wall Material` |
| 1 | Right wall face | `Right Wall Material` |
| 2 | Top face | `Top Material` |
| 3 | Left cap (start end) | `Left Cap Material` |
| 4 | Right cap (end) | `Right Cap Material` |
| 5 | Left skirting face | `Left Skirting Material` |
| 6 | Left skirting top | `Left Skirting Top Material` |
| 7 | Left skirting cap | `Left Skirting Cap Material` |
| 8 | Right skirting face | `Right Skirting Material` |
| 9 | Right skirting top | `Right Skirting Top Material` |
| 10 | Right skirting cap | `Right Skirting Cap Material` |

### All 11 properties are now fully supported

Every material dropdown in the widget now directly controls its own dedicated geometry:

1. **Left/Right wall faces** ✓
2. **Top face** ✓
3. **Left/Right caps** ✓
4. **Left/Right skirting faces** ✓
5. **Left/Right skirting tops** ✓
6. **Left/Right skirting caps** ✓

---

## Troubleshooting

### Dropdown shows text but no thumbnails

- Ensure each catalog entry has **PreviewTexture** assigned.
- Ensure preview textures are set to:
  - Compression: UserInterface2D
  - NoMipmaps
- Ensure `WBP_MaterialSelector.UpdatePreview()` uses `Set Brush From Texture`.

### Dropdown is empty

- Ensure you call `LoadMaterialsFromCatalog(DA_WallFinishes)` and have implemented `OnMaterialsLoaded` in `WBP_WallProperties`.

### Wall doesn’t change material

- Ensure ShellActor has `SetFinishCatalog(DA_WallFinishes)`.
- Ensure your wall edit is going through the document command (the widget setters already do).

---

## API (quick)

In `WBP_WallProperties`, you mainly call:

- `SetupContext(Document, SelectTool)`
- `LoadMaterialsFromCatalog(FinishCatalog)`

and implement these events:

- `OnMaterialsLoaded`
- `OnWallDataChanged`
- `OnSelectionChanged`
- `OnUnitChanged`
