# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Unreal Engine 5.7 project featuring a custom plugin **SmartCatAI** — an AI cat character with procedural IK. Primary development platform is macOS (Apple Silicon).

## Build Commands

Build from command line using Unreal Build Tool:

```bash
# Build game (Mac)
/Users/Shared/Epic\ Games/UE_5.7/Engine/Build/BatchFiles/Mac/Build.sh SmartCatAITest Mac Development "/Users/kentorimaru/Documents/Unreal Projects/SmartCatAITest/SmartCatAITest.uproject"

# Build editor module (Mac)
/Users/Shared/Epic\ Games/UE_5.7/Engine/Build/BatchFiles/Mac/Build.sh SmartCatAITestEditor Mac Development "/Users/kentorimaru/Documents/Unreal Projects/SmartCatAITest/SmartCatAITest.uproject"
```

Alternatively, build via the Xcode workspace: `SmartCatAITest (Mac).xcworkspace`

## Architecture

### Modules

- **SmartCatAITest** (`Source/SmartCatAITest/`) — Main game module. Default implementation, depends on Core, CoreUObject, Engine, InputCore, EnhancedInput.
- **SmartCatAI** (`Plugins/SmartCatAI/Source/SmartCatAI/`) — Runtime plugin module for the AI cat character. Depends on Core, CoreUObject, Engine, Slate, SlateCore.

### Build Rules

- Target files: `Source/SmartCatAITest.Target.cs` (Game) and `Source/SmartCatAITestEditor.Target.cs` (Editor)
- Module build rules: `Source/SmartCatAITest/SmartCatAITest.Build.cs` and `Plugins/SmartCatAI/Source/SmartCatAI/SmartCatAI.Build.cs`
- Build settings version V6, include order version Unreal5_7

### Configuration

- `Config/DefaultEngine.ini` — Renderer config (ray tracing, virtual shadow maps, substrate enabled), default map set to OpenWorld template
- `Config/DefaultInput.ini` — Input bindings (Enhanced Input System)
- Plugin descriptor: `Plugins/SmartCatAI/SmartCatAI.uplugin`
