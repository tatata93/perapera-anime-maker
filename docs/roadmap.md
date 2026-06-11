# Roadmap

This document records the development roadmap for ぺらぺらアニメ作り機.

## Step 0: Environment and Project Skeleton

```text
Step0
├── C++20 project
├── CMake build
├── Git LFS settings
├── Documentation files
├── License policy
└── Work log
```

## Phase 1: Minimal Application

```text
Phase1
├── SDL3 window
├── Dear ImGui menu
├── Build with CMake
├── Update WORK_LOG.md
└── Update DEPENDENCIES.md
```

## Phase 2: Work Canvas

```text
Phase2
├── WorkCanvas structure
├── RenderFormat structure
├── Arbitrary canvas size
├── Simple pen
├── Simple eraser
└── PNG save
```

## Phase 3: Camera Frame and Rendering

```text
Phase3
├── Output resolution
├── Camera frame view
├── Crop from work canvas
├── PNG image sequence
└── Renderer separation
```

## Phase 4: Camera and Lens

```text
Phase4
├── Focal length in mm
├── Sensor size
├── FOV calculation
├── Pan
├── Tilt
├── Dolly
├── Zoom
├── Dolly zoom
└── Camera keyframes
```

## Phase 5: Background Calibration

```text
Phase5
├── Load background image
├── Reference camera frame
├── 35mm equivalent setting
├── Pixel-to-angle conversion
├── Reprojection
├── Missing background warning
└── Aspect fit modes
```

## Phase 6: 3D Drawing Guides

```text
Phase6
├── Floor grid
├── Cube
├── Cylinder
├── Sphere
├── Pose guide
├── Motion path
├── Ghost display
└── Acceleration curve
```

## Phase 7: Storyboard and Timeline

```text
Phase7
├── StoryboardPanel
├── Shot list
├── Duration
├── Dialogue notes
├── Camera notes
├── Create Shot from storyboard
└── X-Sheet style timeline
```

## Phase 8: Simple Physics

```text
Phase8
├── Hair guide
├── Cloth strip guide
├── Wind
├── Gravity
├── Damping
├── Verlet or PBD
├── Physics bake
└── Manual correction
```

## Phase 9: Export

```text
Phase9
├── PNG sequence
├── WAV
├── OpenEXR sequence
├── External FFmpeg call
├── MP4 helper
├── OTIO export
└── License notice
```