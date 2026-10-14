extends MainLoop

## Generates res://vfx/smoke_atlas.png - the flipbook that replaces the original's fx/smoke.dds
## when maszyna/smoke/generator_mode is "Modern". A drop-in swap for that one sprite:
## same role, same single quad, only animated over the particle's lifetime.
##
## What makes a puff read as smoke rather than as a ball is the structure inside it, not the
## outline. Three things do that work:
##
## * [b]domain warping[/b] - [FastNoiseLite] folds the density field through an offset taken from
##   a second noise field, which turns even blotches into billows and curls;
## * [b]the third noise axis[/b] - every frame is a slice at a different z, so the turbulence
##   evolves coherently from frame to frame instead of being zoomed or scrolled;
## * [b]a ragged envelope[/b] - the cloud is bounded by a radius that varies with the angle and
##   turns over the frames, so no frame is a disc.
##
## Alpha carries all of it; RGB stays white, because the particle tints it with the colour its
## data/smokesource_*.txt template declares.
##
## Deterministic - SEED is the only thing that changes the result. Run it with:
##
##     godot --headless -s res://vfx/make_smoke_atlas.gd

const SEED:int = 20260921
const COLUMNS:int = 4
const ROWS:int = 4
const CELL:int = 128
const OUTPUT:String = "res://vfx/smoke_atlas.png"

## How far the turbulence travels along the third axis over the whole flipbook. Too little and the
## frames barely differ, too much and the puff boils instead of billowing.
const EVOLUTION:float = 2.6
## Detail of the density field; the puff grows, so the frequency drops as the frames advance
const FREQUENCY_START:float = 0.105
const FREQUENCY_END:float = 0.062
## How hard the density field is folded, in noise units
const WARP_AMPLITUDE:float = 16.0

## Mean radius of the envelope as a share of the cell, and how far the angular variation pushes it
## in and out. radius * (1 + RAGGED) must stay under 0.5, or a frame touches its cell border and
## bleeds into the neighbouring one when Godot filters the atlas.
const RADIUS_START:float = 0.28
const RADIUS_END:float = 0.42
const RAGGED:float = 0.18
## Share of the radius the envelope fades out over
const FEATHER:float = 0.35
## The puff rises as it expands, as a share of the cell
const RISE:float = 0.07

## The cloud is thresholded after the envelope has already thinned its edges, so a rising
## threshold eats it from the outside in - the core stays solid while the rim tears into wisps,
## instead of the whole puff speckling evenly.
const THRESHOLD_START:float = 0.13
const THRESHOLD_END:float = 0.55
## Width of the threshold: narrow leaves hard flakes, wide leaves haze. It widens as the puff
## ages, because smoke thins out and diffuses - a late frame of hard specks reads as confetti.
const THRESHOLD_EDGE_START:float = 0.16
const THRESHOLD_EDGE_END:float = 0.46
## fBm crowds its values around the middle of the range; this spreads them back out, so the field
## has dense cores and empty gaps instead of even grey
const DENSITY_GAIN:float = 2.4
## The original's own sprite peaks at 222, never at full white
const PEAK_ALPHA:float = 0.88


func _initialize() -> void:
    var density:FastNoiseLite = _make_density()
    var outline:FastNoiseLite = _make_outline()
    var atlas:Image = Image.create(COLUMNS * CELL, ROWS * CELL, false, Image.FORMAT_RGBA8)
    var count:int = COLUMNS * ROWS

    for index:int in count:
        var t:float = float(index) / float(count - 1)
        density.frequency = lerpf(FREQUENCY_START, FREQUENCY_END, t)
        var frame:Image = _build_frame(density, outline, t, EVOLUTION * t)
        atlas.blit_rect(
            frame, Rect2i(Vector2i.ZERO, frame.get_size()),
            Vector2i((index % COLUMNS) * CELL, (index / COLUMNS) * CELL))
        _report(index, frame)

    var error:int = atlas.save_png(OUTPUT)
    if error:
        push_error("[make_smoke_atlas] Cannot write %s: %d" % [OUTPUT, error])
    print("written %s (%dx%d, %dx%d frames)" % [
        OUTPUT, atlas.get_width(), atlas.get_height(), COLUMNS, ROWS])


## Everything happens in _initialize(); returning true from the first frame ends the process
func _process(_delta:float) -> bool:
    return true


func _make_density() -> FastNoiseLite:
    var noise:FastNoiseLite = FastNoiseLite.new()
    noise.seed = SEED
    noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
    noise.fractal_type = FastNoiseLite.FRACTAL_FBM
    noise.fractal_octaves = 5
    noise.fractal_lacunarity = 2.1
    noise.fractal_gain = 0.52
    noise.domain_warp_enabled = true
    noise.domain_warp_type = FastNoiseLite.DOMAIN_WARP_SIMPLEX
    noise.domain_warp_amplitude = WARP_AMPLITUDE
    noise.domain_warp_frequency = 0.035
    noise.domain_warp_fractal_type = FastNoiseLite.DOMAIN_WARP_FRACTAL_PROGRESSIVE
    noise.domain_warp_fractal_octaves = 3
    return noise


## Low frequency noise read around a circle, so the outline wanders instead of closing into a disc
func _make_outline() -> FastNoiseLite:
    var noise:FastNoiseLite = FastNoiseLite.new()
    noise.seed = SEED + 37
    noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
    noise.fractal_type = FastNoiseLite.FRACTAL_FBM
    noise.fractal_octaves = 3
    noise.frequency = 0.9
    return noise


func _build_frame(density:FastNoiseLite, outline:FastNoiseLite, t:float, z:float) -> Image:
    # Built as raw bytes rather than through set_pixel(): sixteen thousand Color allocations per
    # frame is what made this take minutes
    var bytes:PackedByteArray = PackedByteArray()
    bytes.resize(CELL * CELL * 4)
    var centre:float = (CELL - 1) * 0.5
    var radius:float = lerpf(RADIUS_START, RADIUS_END, t) * CELL
    var threshold:float = lerpf(THRESHOLD_START, THRESHOLD_END, t)
    var edge:float = lerpf(THRESHOLD_EDGE_START, THRESHOLD_EDGE_END, t)
    var rise:float = RISE * t * CELL
    # wells up over the first frames, then thins out; the particle's own fade finishes it
    var amplitude:float = minf(1.0, (t + 0.1) * 6.0) * (1.0 - t * 0.5) * PEAK_ALPHA

    var index:int = 0
    for y:int in CELL:
        var dy:float = y - centre + rise
        for x:int in CELL:
            bytes[index] = 255
            bytes[index + 1] = 255
            bytes[index + 2] = 255
            index += 4
            var dx:float = x - centre
            var angle:float = atan2(dy, dx)
            var wobble:float = outline.get_noise_3d(cos(angle), sin(angle), z)
            var limit:float = radius * (1.0 + RAGGED * wobble)
            var envelope:float = smoothstep(limit, limit * (1.0 - FEATHER), sqrt(dx * dx + dy * dy))
            if not envelope:
                continue

            # get_noise_3d returns -1..1 and already carries the domain warp
            var value:float = clampf(
                density.get_noise_3d(x, y - rise, z * 12.0) * DENSITY_GAIN * 0.5 + 0.5, 0.0, 1.0)
            # thinned by the envelope first, so the threshold below bites the rim before the core
            value = smoothstep(threshold, threshold + edge, value * envelope)
            if value <= 0.0:
                continue
            bytes[index - 1] = int(value * amplitude * 255.0)
    return Image.create_from_data(CELL, CELL, false, Image.FORMAT_RGBA8, bytes)


## Coarse rows plus the alpha profile, so the silhouette can be judged without opening the file
func _report(index:int, frame:Image) -> void:
    var ramp:String = " .:-=+*#@"
    var bytes:PackedByteArray = frame.get_data()
    var total:int = 0
    var peak:int = 0
    for offset:int in range(3, bytes.size(), 4):
        total += bytes[offset]
        peak = maxi(peak, bytes[offset])
    var rows:PackedStringArray = []
    var step:int = CELL / 12
    for row:int in 12:
        var line:String = ""
        for column:int in 24:
            var alpha:int = bytes[((row * step) * CELL + column * (CELL / 24)) * 4 + 3]
            line += ramp[clampi(alpha * ramp.length() / 256, 0, ramp.length() - 1)]
        rows.append(line)
    print("frame %2d  mean %.3f  peak %.2f" % [
        index, float(total) / float(CELL * CELL * 255), float(peak) / 255.0])
    print("\n".join(rows))
