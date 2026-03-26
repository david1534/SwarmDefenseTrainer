# Swarm Defense Trainer (SDT)

Counter-UAS training system built with Unreal Engine 5.7.4+. The player defends a High Value Asset against incoming drone swarms using a first-person hitscan weapon. Supports both standard mouse/keyboard input and VN-100 IMU hardware aiming with mouse-click firing.

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
| `SDTPlayerController` | Input bridge: mouse (mock) or VN-100 (hardware) aiming, mouse-click firing, calibration |
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

### Plugin

| Plugin | Purpose |
|--------|---------|
| `VN100Input` | Custom UE5 plugin that reads orientation (yaw/pitch/roll) from a VectorNav VN-100 IMU over serial. Background thread, thread-safe, cross-platform (Win32 + POSIX). |

---

## Prerequisites

### Software You Need Installed

| Software | Version | Download | Why |
|----------|---------|----------|-----|
| Unreal Engine | **5.7.4+** | [Epic Games Launcher](https://www.unrealengine.com/download) | Game engine. Install via the Epic Games Launcher > Unreal Engine tab > Library > + button > 5.7. |
| Visual Studio | **2022 or 2026** (Windows) | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) | C++ compiler for UE5. During install, select the **"Game development with C++"** and **"Desktop development with C++"** workloads. If using VS 2026, also install the **MSVC v143** toolset via Individual Components to ensure UBT compatibility. |
| Xcode | **15+** (Mac only) | Mac App Store | C++ compiler for UE5 on macOS. |

### Hardware You Need (for VN-100 integration)

| Hardware | Purpose | Notes |
|----------|---------|-------|
| VectorNav VN-100 IMU | Orientation sensor (aim control) | Mounted to the hand-held device. Connects via USB serial. |
| USB cable (x1) | Connect VN-100 to PC | Standard USB cable for the VN-100. |
| Hand-held chassis | Physical housing | Custom-built to hold the VN-100. Mouse is used for firing. |

You do NOT need any hardware to develop and test. The game runs fully in mock mode with mouse and keyboard. In hardware mode, the VN-100 controls aiming and the mouse left-click fires.

---

## Step-by-Step Setup

### Phase 1: Get UE5 Ready

**Step 1 -- Install Unreal Engine 5.7.4+**

1. Download and install the [Epic Games Launcher](https://www.unrealengine.com/download)
2. Open the launcher, go to the **Unreal Engine** tab
3. Click **Library** in the left sidebar
4. Click the **+** button next to "Engine Versions"
5. Select version **5.7.x** (5.7.4 or newer) and click **Install**
6. Make sure you have at least **50 GB free disk space**
7. Wait for the install to complete (this can take 30-60 minutes)

**Step 2 -- Install a C++ Compiler**

*Windows (Visual Studio 2022):*
1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/) (free)
2. During installation, check the **"Game development with C++"** workload
3. Also check **"Desktop development with C++"** workload
4. Complete the install and restart your PC

*Windows (Visual Studio 2026):*
1. Download [Visual Studio 2026 Community](https://visualstudio.microsoft.com/) (free)
2. During installation, check the **"Game development with C++"** workload
3. Also check **"Desktop development with C++"** workload
4. **Important — UE 5.7.4 requires the MSVC v143 toolset.** VS 2026 ships with MSVC v144 by default, which UE's build tool (UnrealBuildTool) may not yet recognize. To fix this:
   - Open the **Visual Studio Installer** (search for it in the Start menu)
   - Click **Modify** next to your VS 2026 install
   - Go to the **Individual Components** tab
   - Search for **MSVC v143**
   - Check the box for **MSVC v143 - VS 2022 C++ x64/x86 build tools**
   - Click **Modify** to install
5. Complete the install and restart your PC

*Mac:*
1. Install Xcode from the Mac App Store
2. Open Terminal and run: `xcode-select --install` to install command-line tools

**Step 3 -- Clone and Open the Project**

1. Clone this repository:
   ```
   git clone https://github.com/anthonyw298/SwarmDefenseTrainer.git
   ```
2. Double-click `SwarmDefenseTrainer.uproject`
   - If Windows asks which application to use, right-click the file instead > **Open with** > browse to your UE install (e.g., `C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe`) and check "Always use this app"
   - Alternatively, open the project through the **Epic Games Launcher** > Unreal Engine tab > Library > Browse
3. If prompted with **"Target Upgrade Required"**, click **Yes/Update** to convert the project to your engine version
4. UE5 will ask: *"Would you like to rebuild the missing modules?"* -- Click **Yes**
5. Wait for compilation (first time takes 2-5 minutes depending on your machine)
6. The UE5 editor opens with an empty/default viewport

---

### Phase 2: Create the Game Map

The game needs a map called `BattleArena`. It does not exist yet -- you create it in the editor.

> **UE5 Editor Orientation (read this first if you've never used Unreal):**
>
> When the editor opens you will see:
> - **Viewport** — the large 3D view in the center. This is where you see and place objects in your game world.
> - **Content Browser** — the panel at the bottom. This is your file manager for game assets (maps, meshes, Blueprints, etc.). If you don't see it, go to **Window > Content Browser > Content Browser 1**.
> - **Details panel** — the panel on the right. When you click on any object in the viewport, its properties appear here (location, rotation, scale, mesh, etc.). If you don't see it, go to **Window > Details**.
> - **Outliner** — the panel listing every object in the current level (usually top-right). If you don't see it, go to **Window > Outliner**.
> - **Toolbar** — the horizontal bar at the top with buttons like Play, Save, etc.
> - **Main menu bar** — the very top: File, Edit, Window, Tools, Build, etc.

**Step 4 -- Create the BattleArena Map**

1. In the UE5 editor menu bar at the very top: click **File > New Level**
2. A dialog appears with level templates. Select **Basic** (this gives you a floor, sky, and lighting out of the box)
3. Click **Create**
4. You should now see a 3D viewport with a floor and sky
5. Save it: click **File > Save Current Level As...**
6. In the save dialog:
   - You should see a `Content` folder. If you see a `Maps` folder inside it, click into it. If not, click the **+ New Folder** button (or right-click > **New Folder**), name it `Maps`, then click into it
   - In the **Name** field at the bottom, type: **BattleArena**
   - Click **Save**
   - The full path should now be: `/Game/Maps/BattleArena`

**Step 5 -- Place the High Value Asset (HVA)**

This is the object the player defends. Drones fly toward it. **Required — the game will not work without it.**

1. Open the **Place Actors** panel using one of these methods (the menu location changed in UE 5.7):
   - **Method A (recommended):** Click the **`+`** (plus) button in the **top-left of the toolbar** — this opens the quick-add menu
   - **Method B:** Press **Ctrl+Shift+1** on your keyboard to toggle the Place Actors panel
   - **Method C:** Go to **Window > Place Actors** from the top menu bar
2. In the search box at the top of the Place Actors panel, type: **SDTHighValueAsset**
   - You should see `SDTHighValueAsset` appear in the list. If you don't see it, make sure the project compiled successfully (check Step 3)
3. **Drag** `SDTHighValueAsset` from the list **into the viewport** (the 3D view). Drop it somewhere near the center of the floor
4. The HVA is now in your level but may look like a small white dot (it has no mesh yet). Click on it in the viewport to select it — you'll see an orange outline
5. In the **Details panel** (right side), find the **Transform** section at the top:
   - Set **Location** to: `X = 0`, `Y = 0`, `Z = 50`
   - You can type the numbers directly into the X, Y, Z fields
6. Still in the Details panel, scroll down to find **Mesh Component** (or click on `MeshComponent` in the Components list at the top of Details). Look for **Static Mesh**:
   - Click the dropdown next to **Static Mesh** (it may say "None")
   - **Important:** If the search results are empty, you need to enable engine content. Click the **settings/gear icon** (or **eye icon**) in the asset picker and check **"Show Engine Content"**. The basic shapes (Cube, Sphere, etc.) are engine assets and are hidden by default
   - In the search box that appears, type **Cube** (you may also see it listed as `Shape_Cube` or `SM_Cube`)
   - Select **Cube** from the results. The HVA should now appear as a white cube in the viewport
7. To make it bigger: in the **Transform** section, find **Scale** and set it to: `X = 3`, `Y = 3`, `Z = 3`
8. (Optional) To give it color: in the Details panel, find **Materials** under the mesh component, click the dropdown next to **Element 0**, and pick any material (e.g., search for `Basic` or `M_Basic`)
9. Press **Ctrl+S** to save

**Step 6 -- Place the Player Start**

This is where the player spawns and stands. The player is **stationary** (no walking/running), so this position is where they'll stay the entire game.

1. Open the Place Actors panel again (click the **`+`** button in the top-left toolbar, or press **Ctrl+Shift+1**)
2. In the search box, type: **Player Start**
3. **Drag** `Player Start` from the list **into the viewport**. Drop it on the floor, away from the HVA
4. Click on the Player Start to select it. You'll see a blue arrow pointing in the direction the player will face
5. In the **Details panel**, set its **Transform > Location** to:
   - `X = -2000`, `Y = 0`, `Z = 100`
   - This places the player 2000 units in front of the HVA (which is at 0,0,50)
6. Make sure the **blue arrow** on the Player Start points toward the HVA (toward the cube). If it doesn't:
   - Set **Transform > Rotation > Z (Yaw)** to `0` — this makes the player face in the +X direction (toward the HVA)
   - You can also rotate it by selecting the Player Start, pressing **E** to switch to rotation mode, and dragging the blue (Z-axis) ring
7. Press **Ctrl+S** to save

> **Tip:** To see what the player will see, you can click on the Player Start, then in the viewport toolbar click the small camera icon or right-click the Player Start and select **Pilot** to view from its perspective. Press **Eject** or **F** to return to free camera.

**Step 7 -- Verify Map Settings**

1. Go to **Edit > Project Settings** in the top menu bar (a large settings window opens)
2. In the left sidebar, scroll down and click **Maps & Modes** (under the "Project" section)
3. Verify these two fields:
   - **Editor Startup Map**: should say `/Game/Maps/BattleArena`. If it says "None" or something else, click the dropdown and select `BattleArena`
   - **Game Default Map**: should also say `/Game/Maps/BattleArena`. If not, set it the same way
   - These tell UE which map to load when you hit Play and when the game launches
4. These should already be set from `DefaultEngine.ini`, but it's worth double-checking
5. Close the Project Settings window
6. **Ctrl+S** to save

---

### Phase 3: Test in Mock Mode (Mouse + Keyboard)

**Step 8 -- Play Test**

1. Make sure `Config/DefaultGame.ini` has `InputMode=MockInput` (this is the default — you don't need to change anything if you haven't touched this file)
2. In the UE5 toolbar at the top, click the green **Play** button (triangle icon), or press **Alt+P**
   - The viewport will switch to game mode and your mouse cursor will disappear (it's captured by the game)
   - If nothing happens, make sure the viewport is focused (click on it first)
3. You should see the **PreGame title screen** with:
   - "SWARM DEFENSE TRAINER" title
   - Controls list
   - Input mode indicator ("Mock (Mouse)")
   - "Press SPACE to Start" prompt (pulsing green)
4. Press **SPACE** to start the countdown
5. You'll hear countdown beeps and see large numbers: **3... 2... 1... GO!**
6. Gameplay begins — drones spawn and fly toward the HVA
   - Drones may be **invisible** if you haven't created a drone Blueprint yet (Phase 4) — that's OK, they still have collision and you can still hit them with hitscan
7. **Aim** by moving the mouse, **left-click** to fire
8. You should hear: fire sound on each shot, a metallic ping when you hit a drone, an explosion when a drone dies, and an alarm warble if drones reach the HVA
9. The HUD displays: score (top-left), combo multiplier (below score), wave info (top-right), kill count (below wave), asset health bar (bottom-center), timer (top-center)
10. After all 6 waves complete (or if the HVA health reaches 0), the **GameOver screen** appears with:
    - Victory or Defeat title
    - Final score, kill count, time survived
    - Top-10 scoreboard
11. Press **R** to restart
12. To **stop playing** and return to the editor, press **Esc** or click the **Stop** button in the toolbar

**If something is wrong:**
- **Black screen / no HUD text?** Make sure you placed the SDTHighValueAsset in the map (Step 5). The game warns in the Output Log if it's missing. Open the Output Log: **Window > Output Log** from the top menu, and look for yellow warning messages
- **Drones not spawning?** Check the Output Log for error messages starting with "SDT:"
- **Can't aim or fire?** The viewport needs focus. Click inside the game viewport. If your mouse cursor is visible, the game doesn't have input focus
- **Viewport is too small?** You can click the three dots `...` next to the Play button and select **"New Editor Window (PIE)"** to play in a separate resizable window
- **Mouse feels stuck?** Press **Shift+F1** to release the mouse from the game viewport while keeping the game running

---

### Phase 4: Make Drones Visible (Optional but Recommended)

By default, drones have collision (you can hit them with hitscan) but no visible mesh, so they're invisible. To give them a 3D model you need to create a **Blueprint** — this is UE5's visual scripting system, but here we're just using it to assign a mesh to the drone C++ class.

**Step 9 -- Create a Drone Blueprint**

1. Look at the **Content Browser** panel at the bottom of the editor. You should see your project's content folders
   - If the Content Browser is not visible: go to **Window > Content Browser > Content Browser 1**
2. **Right-click** in an empty area of the Content Browser (the right-side file area, not the folder tree)
3. In the context menu, select **Blueprint Class**
4. A **"Pick Parent Class"** dialog appears. You need to find our custom drone class:
   - The common classes (Actor, Character, etc.) are shown at the top — ignore these
   - Click **"All Classes"** to expand the full class list
   - In the search box, type: **SDTDroneBase**
   - Click on `SDTDroneBase` to select it (it should highlight)
   - Click the **Select** button
5. A new Blueprint asset appears in the Content Browser with a name field ready to edit. Type: **BP_Drone** and press **Enter**
6. **Double-click** `BP_Drone` to open the Blueprint editor. A new window/tab opens with:
   - **Components panel** (left side) — shows the parts of this actor (DroneMesh, DefaultSceneRoot, etc.)
   - **Details panel** (right side) — shows properties of the selected component
   - **Viewport** (center) — 3D preview of this Blueprint
7. In the **Components panel** on the left, click on **DroneMesh** to select it
8. In the **Details panel** on the right, find the property called **Static Mesh**:
   - Click the dropdown next to **Static Mesh** (it says "None")
   - **Important:** If the search results are empty, click the **settings/gear icon** (or **eye icon**) in the asset picker and check **"Show Engine Content"** — the basic shapes are engine assets hidden by default
   - In the search box, type: **Sphere** (you may also see it as `Shape_Sphere` or `SM_Sphere`)
   - Select **Sphere** from the results
   - The drone should now appear as a white sphere in the Blueprint viewport
   - (You could also use `Cube`, `Cone`, or any mesh you want)
9. (Optional) Adjust the size: in the Details panel under **Transform > Scale**, set `X = 0.5`, `Y = 0.5`, `Z = 0.5` for a smaller drone
10. (Optional) Add color: under **Materials** in the Details panel, click the dropdown next to **Element 0** and pick a material
11. In the top-left of the Blueprint editor, click **Compile** (blue button with a checkmark). You should see "Compile Successful" in green
12. Click **Save** (floppy disk icon next to Compile)
13. Close the Blueprint editor tab (click the X on the tab) to return to the main editor

**Step 10 -- Create a Wave Manager Blueprint**

The Wave Manager controls how drones spawn. We need to tell it to use our BP_Drone.

1. In the **Content Browser**, **right-click** in an empty area > **Blueprint Class**
2. In the parent class picker, click **All Classes**, search for: **SDTWaveManager**
3. Select `SDTWaveManager` and click **Select**
4. Name it: **BP_WaveManager** and press Enter
5. **Double-click** `BP_WaveManager` to open the Blueprint editor
6. In the **Details panel** on the right (make sure the root `BP_WaveManager(self)` is selected in the Components panel), find the property called **Drone Class**:
   - Click the dropdown (it says "None" or "SDTDroneBase")
   - Select **BP_Drone** (the Blueprint you created in Step 9)
7. (Optional) You can scroll down in Details to find **Wave Configs** — this is a list of 6 waves. You can expand each entry to customize drone count, speed, spawn interval, and delay. The defaults work fine for testing
8. Click **Compile**, then **Save**
9. Close the Blueprint editor tab

**Step 11 -- Create a Game Mode Blueprint**

The Game Mode ties everything together. We need to tell it to use our Wave Manager.

1. In the **Content Browser**, **right-click** > **Blueprint Class**
2. Click **All Classes**, search for: **SDTGameMode**
3. Select `SDTGameMode` and click **Select**
4. Name it: **BP_GameMode** and press Enter
5. **Double-click** `BP_GameMode` to open the Blueprint editor
6. In the **Details panel** (with root selected), find **Wave Manager Class**:
   - Click the dropdown and select **BP_WaveManager** (from Step 10)
7. Click **Compile**, then **Save**
8. Close the Blueprint editor tab
9. Now tell the project to use this Game Mode:
   - Go to **Edit > Project Settings** from the top menu bar
   - In the left sidebar, click **Maps & Modes**
   - Find **Default GameMode** at the top
   - Click the dropdown and select **BP_GameMode**
   - Close the Project Settings window
10. **Ctrl+S** to save everything

**Step 12 -- Test Again**

1. Click the green **Play** button (or **Alt+P**)
2. You should now see visible drones (spheres) spawning in the distance and flying toward the HVA (cube)
3. Press **SPACE** to start, aim with the mouse, and **left-click** to shoot the drones
4. Verify: drones explode when hit, score increases, combo multiplier works, health bar decreases when drones reach the HVA
5. Press **Esc** to stop playing and return to the editor

---

### Phase 5: Custom Sound Assets (Optional)

The game generates procedural default sounds at runtime (fire, hit, explosion, buzz, alarm, beeps). These play automatically. To replace with custom `.wav` audio:

- On **SDTWeaponComponent** (on the character): assign `FireSound`, `HitSound`
- On **BP_Drone** (your drone Blueprint): assign `DeathSound`, `FlyingSound`, `DeathEffect`
- On **SDTHighValueAsset** (in the level): assign `DamageSound`
- On **SDTWeaponComponent**: assign `MuzzleFlashEffect`, `HitEffect` for particle effects

Custom assets override the procedural defaults. If a sound property is left empty, the built-in procedural sound plays automatically.

---

### Phase 6: VN-100 Hardware Integration (When Hardware Arrives)

#### Step 13 -- Set Up the VN-100 Orientation Sensor

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

#### Step 14 -- Mount the VN-100 to the Hand-Held Device

1. Mount the VN-100 securely to your hand-held chassis
   - Orientation matters: the VN-100's X-axis should point forward (the direction you aim)
   - Secure it so it doesn't shift during use
2. Route the USB cable so it reaches the PC
3. The mouse is used for firing -- position it within easy reach of the operator, or attach a mouse button to the chassis

#### Step 15 -- Configure the Game for Hardware Mode

1. Open `Config/DefaultGame.ini` in a text editor
2. Change the following values:
   ```ini
   [/Script/SwarmDefenseTrainer.SDTSettings]
   InputMode=HardwareInput
   OrientationSerialPort=COM3
   BaudRate=115200
   ```
   - Replace `COM3` with the VN-100's actual COM port (e.g., `COM5`, or `/dev/tty.usbserial-XXXX` on Mac)
3. Save the file

**Finding the COM port:**
- *Windows:* Open **Device Manager > Ports (COM & LPT)** -- the VN-100 shows its COM number. You can also plug/unplug the USB cable to see which port appears/disappears
- *Mac:* Open Terminal and run `ls /dev/tty.usb*` -- each device shows as `/dev/tty.usbserial-XXXX` or `/dev/tty.usbmodem-XXXX`

#### Step 16 -- Test Hardware Mode

1. Make sure the VN-100 is plugged in via USB
2. Make sure SensorExplorer is CLOSED (it locks the serial port)
3. Open the UE5 project and hit **Play**
4. Check the **Output Log** (Window > Output Log) for:
   ```
   SDT: VN-100 connected on COM3 @ 115200 baud
   ```
   If you see a "Failed to connect" warning, double-check the COM port number and that no other software has the port open.
5. During gameplay, the HUD (bottom-right) shows:
   - `VN-100: CONNECTED` with a green indicator
   - `FIRE: Mouse Click` label
6. Press **SPACE** to start
7. **Aim** by physically moving the hand-held device -- the weapon follows the VN-100 orientation
8. Press **C** to **calibrate** -- this sets the current device position as "center" (aiming straight ahead)
9. **Left-click the mouse** to fire
10. Press **T** at any time to toggle back to full mouse mode if needed

#### Step 17 -- Tune Sensitivity

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

| Action | Mock (Mouse) | Hardware (VN-100) |
|--------|-------------|----------|
| Aim | Mouse movement | VN-100 orientation |
| Fire | Left mouse button | Left mouse button |
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
  SwarmDefenseTrainer.uproject   # UE5 project file (engine 5.7.4+)
```

---

## Troubleshooting

### "Could not be compiled. Try rebuilding from source manually."

This error appears when UE cannot compile the project's C++ code. Common causes:

1. **No C++ compiler installed.** You need Visual Studio 2022 or 2026 (the full IDE), NOT Visual Studio Code. Install with the "Game development with C++" and "Desktop development with C++" workloads. See Step 2.

2. **Using VS 2026 without the MSVC v143 toolset.** UE 5.7.4's UnrealBuildTool may not recognize the MSVC v144 toolchain that ships with VS 2026. Install the MSVC v143 toolset as an Individual Component in the VS Installer. See Step 2 (VS 2026 instructions).

3. **Missing .NET SDK.** UnrealBuildTool is a C# application. If it fails silently (no log file generated in `Saved/Logs/`), you may need to install the [.NET 6.0 SDK](https://dotnet.microsoft.com/download/dotnet/6.0).

### ".uproject asks which application to open with"

Unreal Engine is not registered as the handler for `.uproject` files. Either:
- Right-click > Open with > browse to `UnrealEditor.exe` in your engine install
- Open through the Epic Games Launcher > Library > Browse to project

### "Target Upgrade Required"

Normal when first opening the project on a different UE patch version. Click **Yes/Update** to let the engine convert the project files.

### Build log location

If compilation fails, check `SwarmDefenseTrainer/Saved/Logs/SwarmDefenseTrainer.log` for detailed error messages. If this file does not exist or was not updated, the failure occurred before the engine loaded the project (likely a UBT or compiler toolchain issue — see items 1-3 above).

---

## Compatibility

| Component | Tested / Supported |
|-----------|-------------------|
| Unreal Engine | 5.7.4+ (uses `BuildSettingsVersion.Latest` and `EngineIncludeOrderVersion.Latest` for forward compatibility) |
| Visual Studio | 2022 (MSVC v143), 2026 (requires MSVC v143 toolset installed as Individual Component) |
| Xcode | 15+ (macOS) |
| Windows | 10/11 |
| macOS | Ventura+ |

The project uses `.Latest` enum values in its Target.cs build files rather than hardcoded version-specific values (e.g., `BuildSettingsVersion.V5`), ensuring forward compatibility with future UE 5.7.x patch releases and beyond.

---

## Requirements Compliance

| # | Requirement | Status |
|---|-------------|--------|
| 1 | Custom hardware & software | Met (VN-100 IMU + mouse) |
| 2 | FPS-style gameplay | Met |
| 3 | Single player | Met |
| 4 | Gameplay >= 30 seconds | Met (wave design ~74s min + explicit 30s safety net) |
| 5 | Defeat incoming autonomous vehicles | Met |
| 6 | Scoring system | Met |
| 7 | Top-10 scoreboard | Met |
| 8 | Hand-held hardware | Met (VN-100 for aiming, mouse for firing; see Phase 6) |
| 9 | VN-100 mounted to hardware | Met (VN-100 controls aim via custom UE5 plugin; see Phase 6) |
| 10 | VN-100 controls aim via custom UE plugin | Met |
| 11 | Sound effects | Met (procedural defaults + custom asset support) |
