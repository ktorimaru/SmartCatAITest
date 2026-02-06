# IK Study Analysis - Bone Height Data

## Study Date: 2026-02-05

## Data Collection
- IK disabled, raw animation bone positions recorded
- Tracked: 4 paws (FL, FR, BL, BR), Bell (chest), Jaw (head)
- For each bone: World Z position, Ground Z at that location, Difference

---

## Flat Ground Observations (Speed = 0)

| Bone | Diff (height above ground) |
|------|---------------------------|
| Front Left Paw | ~0.00 |
| Front Right Paw | ~0.00 |
| Back Left Paw | -0.30 (slightly below) |
| Back Right Paw | -0.30 (slightly below) |
| Bell (chest) | ~18-19 |
| Jaw (head) | ~39-40 |

**Key insight**: Back paws sit slightly below ground in rest pose.

---

## Walking Animation Foot Lift (Speed = 75-125, flat ground)

| Bone | Min Diff | Max Diff | Lift Range |
|------|----------|----------|------------|
| Front Left | -0.38 | 6.67 | ~7 units |
| Front Right | -1.84 | 6.33 | ~8 units |
| Back Left | -0.91 | 8.71 | ~9 units |
| Back Right | -1.97 | 8.95 | ~11 units |

**Key insights**:
- Animation DOES lift feet (6-9 units during walk)
- Back legs lift higher than front legs
- Negative values = paw going below ground plane

---

## Climbing Data (Speed = 200, ground rising from 0 to 200)

### Example frame during climb (t=311.118):
| Bone | Ground Z | Diff |
|------|----------|------|
| FL | 40 | 8.56 |
| FR | 40 | 12.11 |
| BL | 20 | 39.91 |
| BR | 20 | 32.62 |
| Bell | 40 | 23.35 |
| Jaw | 60 | 21.45 |

**Observations**:
- Front paws encounter higher ground first (40) vs back paws (20)
- Large positive diffs on back legs = they're still on lower terrain
- Ground difference (Front - Back) = +20 units indicates upward slope
- Body needs to pitch UP (nose up) to align with slope

### Slope calculation during climb:
```
Front Avg Ground = (40 + 40) / 2 = 40
Back Avg Ground = (20 + 20) / 2 = 20
Slope Height Diff = 40 - 20 = +20 (climbing)
```

---

## Descending Data (Speed = 200, ground falling from 200 to 0)

### Example frame during descent (t=313.606):
| Bone | Ground Z | Diff |
|------|----------|------|
| FL | 160 | 18.19 |
| FR | 160 | 23.35 |
| BL | 200 | -10.88 |
| BR | 180 | 2.08 |
| Bell | 160 | 32.36 |
| Jaw | 140 | 70.58 |

**Observations**:
- Front paws encounter lower ground first (160) vs back paws (200/180)
- **Negative diff on BL (-10.88)** = back left paw going THROUGH the ground
- Large positive diffs = body hasn't adjusted to lower terrain yet
- Ground difference (Front - Back) = -40 units indicates downward slope
- Body needs to pitch DOWN (nose down) to align with slope

### Slope calculation during descent:
```
Front Avg Ground = (160 + 160) / 2 = 160
Back Avg Ground = (200 + 180) / 2 = 190
Slope Height Diff = 160 - 190 = -30 (descending)
```

---

## Key Findings for IK Implementation

### 1. Mesh Rotation (Primary Adjustment)
For gradual slopes, rotating the entire mesh handles most of the adaptation:

**Pitch calculation**:
```cpp
float FrontAvgGround = (FL_GroundZ + FR_GroundZ) / 2;
float BackAvgGround = (BL_GroundZ + BR_GroundZ) / 2;
float SlopeHeightDiff = FrontAvgGround - BackAvgGround;
float PitchAngle = atan2(SlopeHeightDiff, BodyLength);
// Positive = climbing (nose up), Negative = descending (nose down)
```

**Roll calculation**:
```cpp
float LeftAvgGround = (FL_GroundZ + BL_GroundZ) / 2;
float RightAvgGround = (FR_GroundZ + BR_GroundZ) / 2;
float RollHeightDiff = LeftAvgGround - RightAvgGround;
float RollAngle = atan2(RollHeightDiff, BodyWidth);
// Positive = left side higher, Negative = right side higher
```

### 2. Per-Foot IK (Secondary Adjustment)
After mesh rotation, apply IK only for residual differences:
- Uneven terrain (one foot on rock, others on flat)
- Stairs (discrete height changes)
- Large individual foot placement errors

### 3. Predictive Slope Tracking (For AI)
Track ground height changes over time with speed:
```cpp
float GroundHeightDelta = CurrentAvgGround - PreviousAvgGround;
float TimeElapsed = CurrentTime - PreviousTime;
float GroundChangeRate = GroundHeightDelta / TimeElapsed;
// Combined with speed, predicts continuing slope
```

### 4. Swing Phase Detection
From flat ground data:
- Foot lift threshold: ~3-5 units above expected ground
- When Diff > threshold AND was previously near zero = swing phase
- When Diff approaches zero from above = stance phase beginning

---

## Recommended IK Architecture

```
┌─────────────────────────────────────────────┐
│  1. GROUND SAMPLING                          │
│  - Trace at each paw location               │
│  - Record ground Z heights                  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  2. SLOPE CALCULATION                        │
│  - Calculate pitch from front/back diff     │
│  - Calculate roll from left/right diff      │
│  - Smooth over multiple frames              │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  3. MESH ROTATION                            │
│  - Apply pitch/roll to mesh or root bone    │
│  - Handles 80%+ of slope adaptation         │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  4. RESIDUAL FOOT IK                         │
│  - Only for remaining per-foot errors       │
│  - Skip during swing phase (let anim show)  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  5. AI TERRAIN TRACKING                      │
│  - Log slope changes over time              │
│  - Predict terrain ahead based on speed     │
│  - Use for path planning decisions          │
└─────────────────────────────────────────────┘
```

---

## Reference Values

| Parameter | Value | Notes |
|-----------|-------|-------|
| Bell height (flat) | ~18-19 units | Chest height |
| Jaw height (flat) | ~39-40 units | Head height |
| Max foot lift (walk) | ~9 units | Back legs lift more |
| Back paw rest offset | -0.30 units | Slightly below ground |
| Swing detection threshold | ~3-5 units | Above expected ground |

---

## File Reference
Debug data collected from: `Saved/RuntimeIKDebug.csv`
