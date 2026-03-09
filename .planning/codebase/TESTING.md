# Testing Patterns

**Analysis Date:** 2026-03-09

## Test Framework

**Runner:**
- Unreal Automation Framework
- Config: No dedicated repo-local `AutomationTest` config file is detected; tests are registered directly in module source files such as `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp`.

**Assertion Library:**
- Unreal automation assertions via `TestTrue`, `TestFalse`, `TestEqual`, `TestNotNull`, and `AddError` from `Misc/AutomationTest.h`.

**Run Commands:**
```bash
# No repo-local test script was detected.
# Typical Unreal Automation execution uses the editor or UnrealEditor-Cmd.exe.
UnrealEditor.exe ArchVis.uproject
UnrealEditor-Cmd.exe ArchVis.uproject -ExecCmds="Automation RunTests ArchVis" -unattended -nop4
UnrealEditor-Cmd.exe ArchVis.uproject -ExecCmds="Automation RunTests ArchVis.RTPlanCore" -unattended -nop4
```

## Test File Organization

**Location:**
- Tests are colocated with the owning module in that module’s `Private/` folder.
- Example locations:
  - `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp`
  - `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolsTests.cpp`
  - `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`
  - `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetTests.cpp`

**Naming:**
- Files typically end with `Tests.cpp`.
- Test names follow the namespace pattern `ArchVis.<Module>.<Scenario>`, for example `ArchVis.RTPlanCore.Serialization` and `ArchVis.RTPlanShell.Generation`.

**Structure:**
```text
Plugins/<Plugin>/Source/<Module>/Private/<Module>Tests.cpp
```

## Test Structure

**Suite Organization:**
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRTPlanCoreSerializationTest,
    "ArchVis.RTPlanCore.Serialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanCoreSerializationTest::RunTest(const FString& Parameters)
{
    URTPlanDocument* Doc = NewObject<URTPlanDocument>();
    // Arrange data
    // Act
    // Assert with TestTrue / TestEqual / AddError
    return true;
}
```

**Patterns:**
- Arrange in-memory Unreal objects directly with `NewObject<>()`.
- Mutate domain state through commands or direct setup depending on the test goal.
- Assert specific values instead of broad snapshot comparisons.
- Use `return true;` after writing failures through Unreal assertion helpers.

## Mocking

**Framework:**
- No separate mocking framework is detected.

**Patterns:**
```cpp
URTPlanDocument* Doc = NewObject<URTPlanDocument>();
FRTPlanData& Data = Doc->GetDataMutable();
Data.Vertices.Add(V1.Id, V1);
Data.Walls.Add(W1.Id, W1);
```

**What to Mock:**
- Prefer lightweight real Unreal objects and in-memory plan data over heavy mocks.
- Build minimal world state only when required, as in `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`.

**What NOT to Mock:**
- Do not replace core document/command behavior when the purpose is to verify command stack, serialization, snapping, or mesh generation.
- Use actual `URTPlanDocument`, actual command objects, and actual mesh/query helpers when validating engine-integrated behavior.

## Fixtures and Factories

**Test Data:**
```cpp
FRTVertex V1; V1.Id = FGuid::NewGuid(); V1.Position = FVector2D(0, 0);
FRTVertex V2; V2.Id = FGuid::NewGuid(); V2.Position = FVector2D(200, 0);
FRTWall W1; W1.Id = FGuid::NewGuid(); W1.VertexAId = V1.Id; W1.VertexBId = V2.Id;
W1.ThicknessCm = 20.0f;
W1.HeightCm = 300.0f;
```

**Location:**
- Fixtures are handwritten inline inside each test file.
- Shared test factory helpers are not detected.

## Coverage

**Requirements:**
- No enforced percentage threshold is detected.
- Coverage emphasis is module-level behavioral smoke tests across core RTPlan plugins.

**View Coverage:**
```bash
# Unreal automation results can be reviewed in the Editor Automation window.
UnrealEditor.exe ArchVis.uproject
```

## Test Types

**Unit Tests:**
- Core document/command and geometry behaviors are tested as focused automation tests.
- Examples:
  - `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp`
  - `Plugins/RTPlanMath/Source/RTPlanMath/Private/RTPlanMathTests.cpp`
  - `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialTests.cpp`

**Integration Tests:**
- Several tests cross engine/runtime boundaries by creating worlds, actors, meshes, or replication-facing objects.
- Examples:
  - `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`
  - `Plugins/RTPlanObjects/Source/RTPlanObjects/Private/RTPlanObjectsTests.cpp`
  - `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetTests.cpp`

**E2E Tests:**
- Not detected.

## Common Patterns

**Async Testing:**
```cpp
// Async-heavy patterns are limited in the sampled tests.
// Most tests stay synchronous and validate immediate state after setup or command execution.
```

**Error Testing:**
```cpp
bool bSuccess = Doc2->FromJson(Json);
TestTrue("Deserialization returned true", bSuccess);

if (!Doc2->GetData().Vertices.Contains(V1.Id))
{
    AddError("Vertex ID not found in deserialized data");
}
```

## Current Test Inventory

- `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp`
- `Plugins/RTPlanMath/Source/RTPlanMath/Private/RTPlanMathTests.cpp`
- `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialTests.cpp`
- `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolsTests.cpp`
- `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`
- `Plugins/RTPlanOpenings/Source/RTPlanOpenings/Private/RTPlanOpeningsTests.cpp`
- `Plugins/RTPlanObjects/Source/RTPlanObjects/Private/RTPlanObjectsTests.cpp`
- `Plugins/RTPlanRuns/Source/RTPlanRuns/Private/RTPlanRunsTests.cpp`
- `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetTests.cpp`

## Prescriptive Guidance

- Add new tests next to the owning plugin/module in `Private/`.
- Register tests with `IMPLEMENT_SIMPLE_AUTOMATION_TEST` and the `ArchVis.<Module>.<Scenario>` naming pattern.
- Prefer real `NewObject<>()` instances and real plan structs over mock-heavy patterns.
- Cover the happy path plus one rollback/error path for commands and serialization.
- When touching rendering or world-dependent code, copy the small-world setup pattern from `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`.

---

*Testing analysis: 2026-03-09*

