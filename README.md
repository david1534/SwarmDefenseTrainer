# Swarm Defense Trainer (SDT)

Counter-UAS training system built with Unreal Engine 5.7. The player defends a High Value Asset against incoming drone swarms using a first-person hitscan weapon. Supports both standard mouse/keyboard input and custom hardware (VN-100 IMU orientation sensor + Arduino trigger button).

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

---

## Prerequisites

### Software You Need Installed

| Software | Version | Download | Why |
|----------|---------|----------|-----|
| Unreal Engine | **5.7** | [Epic Games Launcher](https://www.unrealengine.com/download) | Game engine. Install via the Epic Games Launcher > Unreal Engine tab > Library > + button > 5.7. |
| Visual Studio | **2022** (Windows) | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) | C++ compiler for UE5. During install, select the **"Game development with C++"** workload. |
| Xcode | **15+** (Mac only) | Mac App Store | C++ compiler for UE5 on macOS. |
| Arduino IDE | **2.0+** | [arduino.cc/en/software](https://www.arduino.cc/en/software) | For uploading the trigger button sketch to the Arduino. Only needed when integrating hardware. |

### Hardware You Need (for full integration)

| Hardware | Purpose | Notes |
|----------|---------|-------|
| VectorNav VN-100 IMU | Orientation sensor (aim control) | Mounted to the hand-held device. Connects via USB serial. |
| Arduino board | Trigger button controller | Arduino Nano, Uno, or any compatible board. |
| Momentary push button | Fire trigger | Wired between Arduino pin D2 and GND. |
| USB cables (x2) | Connect VN-100 and Arduino to PC | One for each device. |
| Hand-held chassis | Physical housing | Custom-built to hold the VN-100 and trigger button. |

You do NOT need any hardware to develop and test. The game runs fully in mock mode with mouse and keyboard.

---

## Step-by-Step Setup

### Phase 1: Get UE5 Ready

**Step 1 -- Install Unreal Engine 5.7**

1. Download and install the [Epic Games Launcher](https://www.unrealengine.com/download)
2. Open the launcher, go to the **Unreal Engine** tab
3. Click **Library** in the left sidebar
4. Click the **+** button next to "Engine Versions"
5. Select version **5.7.x** and click **Install**
6. Make sure you have at least **50 GB free disk space**
7. Wait for the install to complete (this can take 30-60 minutes)

**Step 2 -- Install a C++ Compiler**

*Windows:*
1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/) (free)
2. During installation, check the **"Game development with C++"** workload
3. Also check **"Desktop development with C++"** under Individual Components if not auto-selected
4. Complete the install

*Mac:*
1. Install Xcode from the Mac App Store
2. Open Terminal and run: `xcode-select --install` to install command-line tools

**Step 3 -- Clone and Open the Project**

1. Clone this repository:
   ```
   git clone https://github.com/anthonyw298/SwarmDefenseTrainer.git
   ```
2. Double-click `SwarmDefenseTrainer.uproject`
3. If prompted to select an engine version, choose **5.7**
4. UE5 will ask: *"Would you like to rebuild the missing modules?"* -- Click **Yes**
5. Wait for compilation (first time takes 2-5 minutes depending on your machine)
6. The UE5 editor opens with an empty/default viewport

---

### Phase 2: Create the Game Map

The game needs a map called `BattleArena`. It does not exist yet -- you create it in the editor.

**Step 4 -- Create the BattleArena Map**

1. In the UE5 editor menu bar: **File > New Level**
2. Select **Basic** (this gives you a floor, sky, and lighting out of the box)
3. Click **Create**
4. **File > Save Current Level As...**
5. In the save dialog, navigate to (or create) the folder: `Content/Maps/`
   - If the `Maps` folder doesn't exist: right-click in the Content Browser > **New Folder** > name it `Maps`
6. Save the level as **BattleArena**
   - The full path should be: `/Game/Maps/BattleArena`

**Step 5 -- Place the High Value Asset (HVA)**

This is the object the player defends. Drones fly toward it. Required.

1. In the top menu: **Edit > Place Actors** (or use the Place Actors panel on the left)
2. In the search box, type **SDTHighValueAsset**
3. Drag `SDTHighValueAsset` into the viewport
4. In the **Details** panel (right side), set its location to: `X=0, Y=0, Z=50`
5. Under the **MeshComponent**, click the dropdown next to **Static Mesh**
6. Pick any placeholder mesh -- for example type `Cube` or `Cylinder` and select one
7. Adjust the **Scale** to make it visible (e.g., `X=3, Y=3, Z=3` for a cube)
8. Optionally assign a material to give it color

**Step 6 -- Place the Player Start**

This is where the player spawns. Required.

1. In the Place Actors search, type **Player Start**
2. Drag `Player Start` into the viewport
3. Position it so the player faces the HVA. For example:
   - Location: `X=-2000, Y=0, Z=100`
   - Rotation: `Yaw=0` (facing toward X+ where the HVA is at 0,0,50)
4. The player is **stationary** (no WASD movement), so choose the position carefully
5. Make sure the Player Start arrow (blue arrow gizmo) points toward the HVA

**Step 7 -- Save the Map**

1. **Ctrl+S** to save
2. Verify the map is set as default: **Edit > Project Settings > Maps & Modes**
   - **Editor Startup Map** should be `/Game/Maps/BattleArena`
   - **Game Default Map** should be `/Game/Maps/BattleArena`
   - These should already be set from `DefaultEngine.ini`, but double-check

---

### Phase 3: Test in Mock Mode (Mouse + Keyboard)

**Step 8 -- Play Test**

1. Make sure `Config/DefaultGame.ini` has `InputMode=MockInput` (this is the default)
2. Click the **Play** button (green triangle) in the UE5 toolbar, or press **Alt+P**
3. You should see the **PreGame screen** with the title "SWARM DEFENSE TRAINER" and controls
4. Press **SPACE** to start the countdown
5. You'll hear countdown beeps: **3... 2... 1... GO!**
6. Drones spawn and fly toward the HVA (they may be invisible if you haven't created a drone Blueprint yet -- that's OK, they still have collision)
7. **Aim** with the mouse, **click** to fire
8. You should hear: fire sound, hit confirmation ping, drone explosion, damage alarm if drones reach the HVA
9. The HUD shows: score, combo multiplier, wave number, kills, asset health, timer
10. After 6 waves (or if the HVA is destroyed), the **GameOver** screen appears with final stats and the Top-10 scoreboard
11. Press **R** to restart

**If something is wrong:**
- No HUD text? Make sure you placed the HVA in the map (the game warns in the Output Log if it's missing)
- Drones not spawning? Check the Output Log (Window > Output Log) for error messages
- Can't aim/fire? Make sure the viewport has focus (click in it)

---

### Phase 4: Make Drones Visible (Optional but Recommended)

By default, drones have collision (you can hit them with hitscan) but no visible mesh. To give them a 3D model:

**Step 9 -- Create a Drone Blueprint**

1. In the **Content Browser** (bottom panel), right-click in an empty area
2. Select **Blueprint Class**
3. In the parent class picker, expand **All Classes**, search for **SDTDroneBase**
4. Select `SDTDroneBase` and click **Select**
5. Name it **BP_Drone**
6. Double-click `BP_Drone` to open the Blueprint editor
7. In the **Components** panel (left side), click on **DroneMesh**
8. In the **Details** panel, find **Static Mesh** and pick a mesh:
   - Type `Sphere` for a simple ball, or `Cube`, or import your own drone mesh
9. Adjust the scale (e.g., `X=0.5, Y=0.5, Z=0.5` for a sphere)
10. Optionally add a material for color
11. Click **Compile** (top left) then **Save**

**Step 10 -- Create a Wave Manager Blueprint**

1. In Content Browser, right-click > **Blueprint Class**
2. Search for **SDTWaveManager** as parent > Select > Name it **BP_WaveManager**
3. Double-click to open
4. In the **Details** panel (with the root selected), find **Drone Class**
5. Set it to **BP_Drone** (the Blueprint you created in Step 9)
6. You can also customize wave configs here (drone count, speed, spawn interval, delay per wave)
7. **Compile** and **Save**

**Step 11 -- Create a Game Mode Blueprint**

1. In Content Browser, right-click > **Blueprint Class**
2. Search for **SDTGameMode** as parent > Select > Name it **BP_GameMode**
3. Double-click to open
4. In the **Details** panel, find **Wave Manager Class**
5. Set it to **BP_WaveManager** (from Step 10)
6. **Compile** and **Save**
7. Go to **Edit > Project Settings > Maps & Modes**
8. Set **Default GameMode** to **BP_GameMode**

**Step 12 -- Test Again**

1. Hit **Play** -- you should now see visible drones spawning and flying toward the HVA
2. Shoot them with the mouse and verify they explode

---

### Phase 5: Custom Sound Assets (Optional)

The game generates procedural default sounds at runtime (fire, hit, explosion, buzz, alarm, beeps). These play automatically. To replace with custom `.wav` audio:

- On **SDTWeaponComponent** (on the character): assign `FireSound`, `HitSound`
- On **BP_Drone** (your drone Blueprint): assign `DeathSound`, `FlyingSound`, `DeathEffect`
- On **SDTHighValueAsset** (in the level): assign `DamageSound`
- On **SDTWeaponComponent**: assign `MuzzleFlashEffect`, `HitEffect` for particle effects

Custom assets override the procedural defaults. If a sound property is left empty, the built-in procedural sound plays automatically.

---

### Phase 6: Hardware Integration (When Hardware Arrives)

#### Step 13 -- Set Up the Arduino Trigger Button

**What you need:** Arduino board, momentary push button, USB cable.

1. Install the [Arduino IDE](https://www.arduino.cc/en/software)
2. Connect the Arduino to your PC via USB
3. Open `Arduino/TriggerButton/TriggerButton.ino` in the Arduino IDE
4. Select your board type: **Tools > Board** (e.g., Arduino Nano, Arduino Uno)
5. Select the COM port: **Tools > Port** (e.g., COM4, /dev/cu.usbserial-XXX)
6. Click **Upload** (right arrow button)
7. Wait for "Done uploading"
8. Wire the push button:
   - One leg of the button to **Arduino pin D2**
   - Other leg to **GND**
   - No resistor needed (the code uses the Arduino's internal pull-up)
9. Test: Open **Tools > Serial Monitor** in the Arduino IDE, set baud to **9600**
10. Press the button -- you should see `F` characters appear in the serial monitor
11. Close the Serial Monitor (important -- the game needs exclusive access to the port)
12. Note the COM port number (you'll need it later)

#### Step 14 -- Set Up the VN-100 Orientation Sensor

**What you need:** VectorNav VN-100 IMU, USB cable, VectorNav SensorExplorer software.

1. Download [VectorNav SensorExplorer](https://www.vectornav.com/resources/software) (free, requires registration)
2. Connect the VN-100 to your PC via USB
3. Open SensorExplorer, connect to the VN-100
4. Configure the VN-100 output:
   - Set **Async Data Output Type** to **YPR** (Yaw, Pitch, Roll)
   - Set **Async Data Output Frequency** to **40 Hz** or higher (for responsive aiming; 80 Hz is ideal)
   - Set **Serial Baud Rate** to **115200**
5. Save the configuration to the VN-100
6. Verify: In SensorExplorer's terminal/raw data view, you should see lines like:
   ```
   $VNYPR,12.345,-3.210,0.150*5A
   ```
   This is the exact format the plugin parses: `$VNYPR,yaw,pitch,roll*checksum`
7. Close SensorExplorer (important -- the game needs exclusive access to the port)
8. Note the COM port number

**IMPORTANT**: The plugin ONLY parses `$VNYPR` sentences. Other VN-100 output formats (`$VNYMR`, `$VNIMU`, binary packets) will NOT work. Make sure the async output type is set to YPR.

#### Step 15 -- Mount Hardware to Hand-Held Device

1. Mount the VN-100 securely to your hand-held chassis
   - Orientation matters: the VN-100's X-axis should point forward (the direction you aim)
   - Secure it so it doesn't shift during use
2. Mount the Arduino and trigger button to the chassis
   - Position the button where the trigger finger rests
3. Route both USB cables so they reach the PC

#### Step 16 -- Configure the Game for Hardware

1. Open `Config/DefaultGame.ini` in a text editor
2. Change the following values:
   ```ini
   [/Script/SwarmDefenseTrainer.SDTSettings]
   InputMode=HardwareInput
   OrientationSerialPort=COM3
   TriggerSerialPort=COM4
   BaudRate=115200
   ```
   - Replace `COM3` with the VN-100's actual COM port (e.g., `COM5`, or `/dev/tty.usbserial-XXXX` on Mac)
   - Replace `COM4` with the Arduino's actual COM port
3. Save the file

**Finding COM ports:**
- *Windows:* Open **Device Manager > Ports (COM & LPT)** -- each device shows its COM number
- *Mac:* Open Terminal and run `ls /dev/tty.usb*` -- each device shows as `/dev/tty.usbserial-XXXX` or `/dev/tty.usbmodem-XXXX`

#### Step 17 -- Test Hardware Mode

1. Make sure both the VN-100 and Arduino are plugged in via USB
2. Make sure SensorExplorer and Arduino Serial Monitor are CLOSED (they lock the port)
3. Open the UE5 project and hit **Play**
4. Check the **Output Log** (Window > Output Log) for:
   ```
   SDT: VN-100 connected on COM3 @ 115200 baud
   SDT: Trigger connected on COM4 @ 9600 baud
   ```
   If you see "Failed to connect" warnings, double-check the COM port numbers and that no other software has the port open.
5. During gameplay, the HUD (bottom-right) shows:
   - `VN-100: CONNECTED` with a green indicator
   - `TRIGGER: CONNECTED` with a green indicator
6. Press **SPACE** to start
7. **Aim** by physically moving the hand-held device -- the weapon follows the VN-100 orientation
8. Press **C** to **calibrate** -- this sets the current device position as "center" (aiming straight ahead)
9. **Pull the trigger** to fire
10. Press **T** at any time to toggle back to mouse mode if needed

#### Step 18 -- Tune Sensitivity

If aiming feels too fast, too slow, or jittery, adjust these in `Config/DefaultGame.ini`:

| Parameter | What to Change | Effect |
|-----------|---------------|--------|
| `YawSensitivity` | Increase (>1.0) or decrease (<1.0) | How fast horizontal aim responds |
| `PitchSensitivity` | Increase (>1.0) or decrease (<1.0) | How fast vertical aim responds |
| `SmoothingAlpha` | Lower (0.1) for smooth, higher (0.8) for responsive | Reduces jitter but adds slight lag |
| `DeadZoneDegrees` | Higher (2.0-3.0) to ignore small movements | Prevents jitter when holding still |
| `FireCooldown` | Lower (0.05) for rapid fire, higher (0.5) for slower | Time between shots |

These are also editable in the UE5 editor: **Project Settings > Game > SDT Settings**.

---

## Controls Reference

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

---

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
  SwarmDefenseTrainer.uproject   # UE5 project file (engine 5.7)
```

---

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
| 8 | Hand-held hardware | Met (software ready, see Phase 6) |
| 9 | VN-100 mounted to hardware | Met (software ready, see Phase 6) |
| 10 | VN-100 controls aim via custom UE plugin | Met |
| 11 | Sound effects | Met (procedural defaults + custom asset support) |
