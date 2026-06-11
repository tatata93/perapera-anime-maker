# Coding Rules

This document records coding rules for ぺらぺらアニメ作り機.

## Basic Rule

Code must be understandable without AI assistance.

## Comments

- Add comments for beginners.
- Explain difficult math.
- Explain coordinate transforms.
- Explain why a library is used.
- Add a short description at the top of each file.

## Naming

Use clear names.

Bad:

```cpp
float f;
float s;
```

Good:

```cpp
float focalLengthMm;
float sensorWidthMm;
```

## File Structure

When explaining files, always use a tree.

```text
src/
├── app/
│   └── main.cpp
└── camera/
    ├── Camera.h
    └── Camera.cpp
```

## Library Use

When adding a library, explain:

```text
Library Explanation
├── Name
├── Purpose
├── License
├── Why it is needed
├── Alternatives
├── Free release considerations
└── How it is used
```

## License Safety

Avoid GPL-only dependencies.  
Avoid LGPL dependencies unless approved.  
Do not add Qt without approval.  
Do not statically link FFmpeg.