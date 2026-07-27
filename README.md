# Standoff 2 Mod Menu

## Features

### ESP
- Box ESP
- Snaplines
- Health Bar
- Name ESP
- Distance ESP
- Weapon ESP
- Head Dot

### Aimbot
- Aimbot (FOV, Smooth, Bone)
- Silent Aim
- Predict Movement

### Triggerbot
- Auto Trigger with Delay

### Player
- No Recoil
- No Spread
- Infinite Ammo
- Rapid Fire
- Magic Bullet
- One Hit Kill

### World
- Wallhack
- Chams (Normal, Flat, Glow, Wireframe)
- Glow ESP
- Crosshair (Dot, Cross, Circle, Triangle)
- No Fog
- Night Mode
- Colorful Sky

### Movement
- Speed Hack
- Bunny Hop
- No Clip
- Fly Hack

### Misc
- Anti Spectate
- Streamer Mode
- Fake Lag
- Fake Name

## Installation

1. Uninstall original Standoff 2
2. Install `Standoff2_ModMenu.apk`
3. Launch game
4. Tap "S2" floating button to open menu

## Technical Details

- **Architecture:** ARM64 inline hooking
- **Hooked:** `eglSwapBuffers`, `InputConsumer`
- **UI:** ImGui with GitHub Dark theme
- **Injection:** Smali patch in `UnityPlayer.smali`

## Building

```bash
cd standoff_overlay
gradlew assembleDebug
```

## Disclaimer

This is for educational purposes only. Use at your own risk.
