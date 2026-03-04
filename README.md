# Swarm Defense Trainer (SDT)

Counter-UAS training system built with Unreal Engine 5.4. The player defends a High Value Asset against incoming drone swarms using a first-person hitscan weapon. Supports both standard mouse/keyboard input and custom hardware (VN-100 IMU orientation sensor + Arduino trigger button).

## Overview

The SDT is a single-player first-person shooter training game. Autonomous drones spawn in escalating waves and fly toward a High Value Asset (HVA) that the player must defend. The player aims and fires to destroy drones before they reach the target. A scoring system tracks kills with a combo multiplier, and a persistent Top-10 scoreboard saves between sessions.

### Game Flow

```
PreGame (title screen) -> Countdown (3-2-1) -> Playing (6 waves) -> GameOver (victory/defeat)
```

- **PreGame**: Title screen with controls, input mode indicator, and Top-10 scoreboard. Press SPACE to start.
- **Countdown**: 3-2-1-GO with beep sounds and scale animation.
- **Playing**: 6 escalating waves of drones. HUD shows score, combo, wave info, kills, asset health, timer.
- **GameOver**: Final stats, scoreboard, victory/defeat. Press R to restart.

### Architecture

| Class | Purpose |
|-------|---------|
| `SDTGameMode` | Game flow state machine, wave/score manager ownership, hardware init/shutdown |
| `SDTCharacter` | Stationary first-person character with camera and weapon mesh |
| `SDTPlayerController` | Input bridge: mouse (mock) or VN-100 (hardware), fire handling, calibration |
| `SDTWeaponComponent` | Hitscan weapon: line trace, cooldown, sound/visual effects |
| `SDTDroneBase` | Autonomous drone enemy: flies toward HVA, weaving motion, damageable |
| `SDTWaveManager` | Spawns drones across 6 escalating waves with configurable timing |
| `SDTHighValueAsset` | Defendable target: has health bar, takes damage from drones |
| `SDTScoreManager` | Scoring: 100pts/kill, combo multiplier (x1-x10), persistent Top-10 |
| `SDTSaveGame` | Persistent Top-10 scoreboard using UE5 SaveGame system |
| `SDTSettings` | Project settings: input mode, serial ports, sensitivity, dead zones |
| `SDTGameInstance` | Persists player name across level reloads |
| `SDTHUD` | Canvas-based HUD: all screens drawn directly (no UMG widgets) |
| `SDTDefaultSounds` | Procedural sound effects generated at runtime (no WAV files needed) |

### Plugins

| Plugin | Purpose |
|--------|---------|
| `VN100Input` | Custom UE5 plugin that reads orientation (yaw/pitch/roll) from a VectorNav VN-100 IMU over serial. Background thread, thread-safe, cross-platform (Win32 + POSIX). |
| `HardwareTrigger` | Custom UE5 plugin that reads fire button signals from an Arduino over serial. Detects 'F' byte protocol. |

## Controls

| Action | Mock (Mouse) | Hardware |
|--------|-------------|----------|
| Aim | Mouse movement | VN-100 orientation |
| Fire | Left mouse button | Arduino trigger button |
| Calibrate | C | C (sets VN-100 center) |
| Toggle input mode | T | T |
| Start game | SPACE | SPACE |
| Restart (game over) | R | R |

### Debug Console Commands

Open the UE5 console with `~` and type:

- `SDTForceStart` -- Skip pre-game, start countdown
- `SDTEndGame` -- Force game over
- `SDTToggleInput` -- Switch mock/hardware mode
- `SDTAddScore 500` -- Add score points
- `SDTSetName PlayerOne` -- Set scoreboard name

## What Needs to Be Integrated (Hardware)

The software is fully functional in mock (mouse) mode. When hardware arrives, the following physical integration is needed:

### 1. VN-100 IMU Orientation Sensor

**What it does**: Mounted to the hand-held hardware, the VN-100 provides absolute orientation (yaw, pitch, roll) that controls the weapon aim in-game.

**Protocol**: VN-100 outputs ASCII sentences over serial in the format `$VNYPR,yaw,pitch,roll*checksum` at the configured baud rate (default 115200).

**Software status**: Fully implemented.
- `Plugins/VN100Input/` -- Custom UE5 plugin with background serial reader thread
- `VN100OrientationReader` -- Parses `$VNYPR` sentences, provides thread-safe `FRotator`
- `VN100BlueprintLibrary` -- Blueprint-callable `StartVN100()`, `StopVN100()`, `GetVN100Orientation()`, `IsVN100Connected()`
- `SDTPlayerController::HandleHardwareInput()` -- Reads orientation, applies calibration offset, dead zone filtering, exponential smoothing, sets control rotation

**Integration steps**:
1. Mount VN-100 to the hand-held device
2. Connect VN-100 to PC via USB (note the COM port it appears as)
3. In `Config/DefaultGame.ini`, set `OrientationSerialPort=COMX` (replace X with your port number, or use `/dev/tty.usbserial-XXXX` on Mac)

### 2. Arduino Trigger Button

**What it does**: A momentary push button connected to an Arduino sends fire signals to the game over serial.

**Protocol**: Arduino sends `'F'` byte at 9600 baud when button is pressed (100ms debounce).

**Software status**: Fully implemented.
- `Arduino/TriggerButton/TriggerButton.ino` -- Upload this sketch to the Arduino
- `Plugins/HardwareTrigger/` -- Custom UE5 plugin with background serial reader thread
- `TriggerSerialReader` -- Reads serial bytes, sets thread-safe trigger flag on `'F'`
- `TriggerBlueprintLibrary` -- Blueprint-callable `StartTrigger()`, `StopTrigger()`, `ConsumeTriggerPress()`, `IsTriggerConnected()`

**Integration steps**:
1. Upload `Arduino/TriggerButton/TriggerButton.ino` to the Arduino (Nano or any board)
2. Wire a momentary push button between **pin D2** and **GND** (internal pull-up is used, no external resistor needed)
3. Connect Arduino to PC via USB (note the COM port)
4. In `Config/DefaultGame.ini`, set `TriggerSerialPort=COMY` (replace Y with your port number)

### 3. Switching to Hardware Mode

Once both devices are connected:

1. Open `Config/DefaultGame.ini`
2. Change `InputMode=MockInput` to `InputMode=HardwareInput`
3. Launch the game -- hardware connections are auto-initialized on startup
4. The HUD (bottom-right during gameplay) shows connection status:
   - `VN-100: CONNECTED` / `VN-100: DISCONNECTED`
   - `TRIGGER: CONNECTED` / `TRIGGER: DISCONNECTED`
5. Press **C** to calibrate the VN-100 (sets current orientation as center)
6. Press **T** at runtime to toggle between mock and hardware input

### 4. Tuning Parameters

All tuning is in `Config/DefaultGame.ini` under `[/Script/SwarmDefenseTrainer.SDTSettings]`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `InputMode` | `MockInput` | `MockInput` or `HardwareInput` |
| `OrientationSerialPort` | `COM3` | VN-100 serial port |
| `TriggerSerialPort` | `COM4` | Arduino serial port |
| `BaudRate` | `115200` | VN-100 baud rate |
| `YawSensitivity` | `1.0` | Horizontal aim multiplier (0.1-10.0) |
| `PitchSensitivity` | `1.0` | Vertical aim multiplier (0.1-10.0) |
| `SmoothingAlpha` | `0.3` | Hardware input smoothing (lower = smoother, 0.05-1.0) |
| `DeadZoneDegrees` | `1.0` | Ignore orientation changes smaller than this (0.0-5.0) |
| `FireCooldown` | `0.15` | Minimum seconds between shots (0.05-2.0) |

These are also editable in the UE5 editor under **Project Settings > Game > SDT Settings**.

## UE5 Editor Setup

After cloning, open `SwarmDefenseTrainer.uproject` in Unreal Engine 5.4.

### Required Level Setup

The game expects a map at `/Game/Maps/BattleArena` with:

1. **ASDTHighValueAsset** -- Place one in the level. This is what drones fly toward and the player defends. Assign a static mesh in the details panel.
2. **Player Start** -- Place a Player Start actor where the player should stand.
3. **Drone Blueprint** (optional) -- Create a Blueprint child of `ASDTDroneBase`, assign a drone mesh and optionally custom sounds/effects. Set it as the `DroneClass` on the WaveManager (or leave blank to use the base class).

### Custom Sound Assets (Optional)

The game generates procedural default sounds at runtime. To replace with custom audio:

- On `SDTWeaponComponent`: assign `FireSound`, `HitSound`, `MuzzleFlashEffect`, `HitEffect`
- On `SDTDroneBase` (Blueprint child): assign `DeathSound`, `FlyingSound`, `DeathEffect`
- On `SDTHighValueAsset`: assign `DamageSound`

Custom assets override the procedural defaults. If a sound property is left empty, the built-in procedural sound plays automatically.

## Project Structure

```
SwarmDefenseTrainer/
  Arduino/
    TriggerButton/
      TriggerButton.ino          # Arduino sketch for fire button
  Config/
    DefaultEngine.ini            # Game mode, map, game instance config
    DefaultGame.ini              # SDT settings (input mode, serial ports, tuning)
    DefaultInput.ini             # Key/mouse bindings
  Content/                       # UE5 content (maps, meshes, materials — created in editor)
  Plugins/
    VN100Input/                  # Custom UE5 plugin: VN-100 orientation sensor
      Source/VN100Input/
        Public/
          VN100InputModule.h
          VN100OrientationReader.h   # Background serial reader thread
          VN100BlueprintLibrary.h    # Blueprint API: Start/Stop/Get/IsConnected
        Private/
          VN100InputModule.cpp
          VN100OrientationReader.cpp # Cross-platform serial (Win32 + POSIX)
          VN100BlueprintLibrary.cpp  # Singleton reader management
    HardwareTrigger/             # Custom UE5 plugin: Arduino trigger button
      Source/HardwareTrigger/
        Public/
          HardwareTriggerModule.h
          TriggerSerialReader.h      # Background serial reader thread
          TriggerBlueprintLibrary.h  # Blueprint API: Start/Stop/Consume/IsConnected
        Private/
          HardwareTriggerModule.cpp
          TriggerSerialReader.cpp    # Cross-platform serial (Win32 + POSIX)
          TriggerBlueprintLibrary.cpp
  Source/
    SwarmDefenseTrainer/
      Public/
        SDTGameMode.h            # Game flow state machine
        SDTCharacter.h           # First-person stationary player
        SDTPlayerController.h    # Mock/hardware input bridge
        SDTWeaponComponent.h     # Hitscan weapon logic
        SDTDroneBase.h           # Autonomous drone enemy
        SDTWaveManager.h         # Wave spawning system
        SDTHighValueAsset.h      # Defendable target
        SDTScoreManager.h        # Scoring + combo + Top-10
        SDTSaveGame.h            # Persistent scoreboard
        SDTSettings.h            # Project settings (serial ports, tuning)
        SDTGameInstance.h        # Player name persistence
        SDTHUD.h                 # Canvas-based HUD
        SDTDefaultSounds.h       # Procedural sound generator
      Private/
        (corresponding .cpp files)
  SwarmDefenseTrainer.uproject   # UE5 project file (engine 5.4)
```

## Requirements Compliance

| # | Requirement | Status |
|---|-------------|--------|
| 1 | Custom hardware & software | Met |
| 2 | FPS-style gameplay | Met |
| 3 | Single player | Met |
| 4 | Gameplay >= 30 seconds | Met (wave design ~74s min + explicit 30s safety net) |
| 5 | Defeat incoming autonomous vehicles | Met |
| 6 | Scoring system | Met |
| 7 | Top-10 scoreboard | Met |
| 8 | Hand-held hardware | Met (software ready, see integration section) |
| 9 | VN-100 mounted to hardware | Met (software ready, see integration section) |
| 10 | VN-100 controls aim via custom UE plugin | Met |
| 11 | Sound effects | Met (procedural defaults + custom asset support) |
