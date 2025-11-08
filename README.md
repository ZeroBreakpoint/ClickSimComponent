# ClickSimComponent (Unreal Engine, C++)

This small class creates a component to attach to Player Controllers within Unreal Engine.  Its purpose is to simulate a **left mouse click** at the current mouse cursor when pressing **Gamepad A**.  
Prevents UMG from consuming the A-press by registering a **Slate input preprocessor**, then fires a real pointer click at the cursor.  Includes **widget-only** mode, by setting `bRequireHoveredWidget` to true.

SetEnabled and SetDisabled functions that can be called from within engine to turn this simulate on and off.

## Install (drop-in source)
1) Copy:
- `Source/YourProject/Public/ClickSimComponent.h`
- `Source/YourProject/Private/ClickSimComponent.cpp`

2) In `<YourProject>.Build.cs` add:
```csharp
PublicDependencyModuleNames.AddRange(new[]{
  "Core","CoreUObject","Engine","InputCore","Slate","SlateCore","UMG","ApplicationCore"
});
