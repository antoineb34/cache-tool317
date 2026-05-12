# Algorithms Reference

This document describes the key algorithms used in the project. The goal is to make them portable — you should be able to reimplement these in any language.

## HSL Color → RGB Palette (RS 317)

RS 317 stores face colors as 16-bit HSL values. At runtime, it builds a 65,536-entry palette mapping HSL to RGB.

### HSL Packing

```
Bits 15-10: Hue        (0..63)
Bits 9-7:   Saturation (0..7)
Bits 6-0:   Lightness  (0..127)
```

Palette index = `(hue * 8 + sat) * 128 + light`

### Palette Generation

```
For hueGroup = 0..511:
    hue        = (hueGroup / 8) / 64.0 + 0.0078125
    saturation = (hueGroup & 7) / 8.0 + 0.0625

    For light = 0..127:
        intensity = light / 128.0

        If saturation == 0:
            r = g = b = intensity
        Else:
            If intensity < 0.5:
                a = intensity * (1.0 + saturation)
            Else:
                a = intensity + saturation - intensity * saturation
            b = 2.0 * intensity - a

            r = paletteValue(hue + 1/3, a, b)
            g = paletteValue(hue,       a, b)
            b = paletteValue(hue - 1/3, a, b)

        rgb = (int(r * 256) << 16) | (int(g * 256) << 8) | int(b * 256)
        rgb = adjustBrightness(rgb, brightness)
        If rgb == 0: rgb = 1
        palette[hueGroup * 128 + light] = rgb
```

### paletteValue(h, a, b)

```
If 6.0 * h < 1.0: return b + (a - b) * 6.0 * h
If 2.0 * h < 1.0: return a
If 3.0 * h < 2.0: return b + (a - b) * ((2.0/3.0) - h) * 6.0
Return b
```

### adjustBrightness(rgb, brightness)

```
r = ((rgb >> 16) & 0xFF) / 256.0
g = ((rgb >> 8)  & 0xFF) / 256.0
b = ( rgb        & 0xFF) / 256.0
r = pow(r, brightness)
g = pow(g, brightness)
b = pow(b, brightness)
return (int(r * 256) << 16) | (int(g * 256) << 8) | int(b * 256)
```

## mixLightness (Face Lighting)

After computing a face normal, apply lighting to the base HSL color.

```
lightness = lightMod + dot(lightVector, faceNormal) / lightMagnitude

If drawType & 2:
    // Textured or special — just return lightness as grayscale
    lightness = clamp(lightness, 0, 127)
    return 127 - lightness
Else:
    // Normal colored face
    lightness = (lightness * (colour & 0x7F)) >> 7
    lightness = clamp(lightness, 2, 126)
    return (colour & 0xFF80) + lightness
```

Where:
- `lightVector = (-30, -50, -30)`
- `lightMod = 64`
- `lightMagnitude = 65` (approximate length of lightVector)
- `colour & 0x7F` extracts the base lightness (0..127)
- `colour & 0xFF80` preserves hue and saturation

## Vertex Normal Computation (Smooth Shading)

For smooth-shaded faces (renderType & 1 == 0), accumulate face normals at shared vertices.

```
For each face:
    Compute face normal via cross product
    Scale normal to length 256

    If face is smooth-shaded:
        Add normal to vertexNormal[a], [b], [c]
        Increment vertexNormal[a].count, etc.

For each smooth-shaded face:
    For each vertex:
        If vertexNormal.count > 0:
            avgNormal = vertexNormal.sum / vertexNormal.count
        Else:
            avgNormal = faceNormal
        lightness = lightMod + dot(lightVector, avgNormal) / lightMagnitude
        litColor = mixLightness(faceColor, lightness, renderType)
```

For flat-shaded faces (renderType & 1 == 1), use the face normal directly.

## Face Normal (Cross Product)

Given triangle vertices A, B, C:

```
dx1 = B.x - A.x, dy1 = B.y - A.y, dz1 = B.z - A.z
dx2 = C.x - A.x, dy2 = C.y - A.y, dz2 = C.z - A.z

nx = dy1 * dz2 - dy2 * dz1
ny = dz1 * dx2 - dz2 * dx1
nz = dx1 * dy2 - dx2 * dy1

len = sqrt(nx*nx + ny*ny + nz*nz)
If len == 0: len = 1
nx = (nx * 256) / len
ny = (ny * 256) / len
nz = (nz * 256) / len
```

## Coordinate System

RS 317 uses a right-handed coordinate system:
- **+X**: East
- **+Y**: Up (but stored as negative in some places)
- **+Z**: South

For OpenGL rendering, Y is flipped:
```cpp
glVertex3f(x, -y, z);  // RS Y-up → OpenGL Y-up
```

Models typically need a 180° Y rotation so +Z faces the camera.

## Perspective Projection Matrix

```
f = 1.0 / tan(fov * PI / 360.0)
nf = 1.0 / (near - far)

[ f/aspect   0          0                    0 ]
[ 0          f          0                    0 ]
[ 0          0   (far+near)*nf        -1      ]
[ 0          0   2*far*near*nf         0      ]
```

## Name Hash

```
hash = 0
for each character c in name:
    hash = hash * 61 + uppercase(c) - 32
```
