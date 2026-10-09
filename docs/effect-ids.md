# Effect ids

Every ZD2 effect carries an id: a 32-bit number, written as 8 hex digits,
stamped into the container file. This doc is how to choose one for your own
effect so that it collides with nothing already on the pedal.

Ids are written as hex throughout, the way the tools print them (`07000f0a`).

## Where the id lives, and why it has to be unique

The id is a 32-bit little-endian integer at offset 96 (`0x60`) in the ZD2
container, just ahead of the effect name. Any effect file has one, and the
install tooling reads it straight from there.

Patches reference an effect by its id, not by its name. So if your effect
carries an id that some effect already on the pedal also carries (and a stock
Zoom effect is the collision that matters), every patch that meant that effect
now points at the wrong one. That is the single outcome to avoid.

What a patch actually resolves to when two installed effects share an id is
still untested. What is now confirmed, by accident, is the symptom one level
up: the pedal's effect browser shows icons only, no filenames or names, so two
DIY files sharing an id become visually indistinguishable there. The safe
installer, `tools-pedal/pedal_diy.py`, guards against this: before it uploads
anything it reads the pedal's effect list and refuses an effect whose id or
filename matches one that is already installed. It can only see the pedal it
is talking to, so an id still has to be unique among everything you plan to
install.

## How ids are structured

The top byte is the effect group. It also decides which category the effect
shows up under in the pedal's browser. The group numbers are per model: the
same number names a different category on another pedal, and each model uses
only some of the numbers. The table below shows the groups the stock effects
use on each MS Plus pedal. See also zoom-explorer, ZoomDevice.ts,
getCategoryNameFromID().

| Top byte | MS-70CDR+ | MS-50G+ | MS-60B+ | MS-80IR+ | MS-200D+ |
|---|---|---|---|---|---|
| 01 | DYNAMICS | DYNAMICS | DYNAMICS | - | BOOSTER |
| 02 | FILTER | FILTER | FILTER | - | OVERDRIVE |
| 03 | - | DRIVE | DRIVE | - | DISTORTION |
| 04 | - | PREAMP | PREAMP | AMP | FUZZ |
| 05 | - | - | BASS AMP | - | PREAMP |
| 06 | MODULATION | MODULATION | MODULATION | TOOL | TOOL |
| 07 | SFX | SFX | PITCH SHIFT | - | - |
| 08 | DELAY | DELAY | SYNTH | DELAY | - |
| 09 | REVERB | REVERB | SFX | - | - |
| 0a | - | - | DELAY | - | - |
| 0b | - | - | REVERB | - | - |
| 0f | - | - | - | AMBIENCE | - |
| 1d | - | - | - | - | DRIVE |

A dash means that no stock effect on that model uses the group. The names are
the ones the stock effect files carry in their own headers. The group list in
`zoomzt2.py` (mungewell/zoom-zt2) is one list for all models (`04` AMP, `05`
CABINET, `0b` PEDAL), so its labels are not always the ones in this table.

The rest of the id is the effect's identity within its group, and stock
effects sit at low values there. You can see the full stock catalogue for your
own pedal by reading it back, or in the zoom-zt2 catalog files.

## Picking an id

The rule is short: pick an id that collides with nothing, and keep it in a
range the safety tooling recognises as DIY. Where you pick depends on whether
the effect is just for your own pedal or something you will hand to other
people.

1. For an effect you will only run on your own pedal, use the playground:
   `low16 0x1000` to `0x13ff` (any group byte). These ids are throwaway: reuse
   them freely as an effect changes, and name the build with an `XX` prefix so
   it is obviously expendable. `low16 0x1400` to `0x1fff` is reserved, so do
   not pick ids from it. The older DIY page `07000f00` to `07000fff`
   works too, but Zoom occupies both ends of it (`07000f00` is the stock LINE
   SEL effect and `07000ff0` is the pedal's built-in BPM module, which has no
   file but still owns the id), so prefer the playground. The installer refuses
   those two stock ids either way.

2. For an effect you will distribute, get an id range of your own first, so
   it will not collide with anyone else's DIY effects on a shared pedal. Open
   an issue in this repo and I assign you one, recorded in
   [effect-authors.json](../ids/effect-authors.json). Do not pick an id at
   `low16 0x2000` or above yourself: every range there belongs to an author,
   and the first one, `0x2000` to `0x23ff`, is mine. How it works is in
   [id-scheme.md](id-scheme.md). The effects in this repo are examples, so they
   stay in the playground, with the SFX group byte `07` for the MS-70CDR+, at
   `07001000` (DIY Gain), `07001010` (WfSplit), `07001020` (WfMerge) and
   `07001030` (WfSwap). Copy the gain folder, and before you distribute, change
   the id to one in your own range and the `author` in the manifest to you.

3. The safe installer only ever touches DIY ids. `tools-pedal/pedal_diy.py`
   installs or uninstalls an effect only if its id is in a DIY range AND its id,
   name and filename match no stock effect. It is fail-closed: an id outside the
   DIY range is refused outright, so the wrapper cannot act on a stock effect by
   accident. Keep your effect in a DIY range and you keep that safety net; take
   it out and you lose it and have to drive the raw install by hand.
   See [../tools-pedal/README.md](../tools-pedal/README.md) and
   [../SAFETY.md](../SAFETY.md).

4. Do not collide with your own effects either. If you already have DIY effects
   installed, keep their ids distinct. Read the pedal back to see what is on it
   before you add another.

5. One id per effect, for good. Treat a released id like an API: a new version
   of an effect keeps its id, a genuinely new effect gets a new one.

6. The filename has to be unique too: 8 characters or fewer, from A-Z 0-9 and
   underscore. The id space and the pedal's filesystem are separate namespaces,
   and the tooling pairs an effect with its icon by filename. Filenames start
   with a 2-letter namespace prefix (`XX` for playground builds, an author's
   registered prefix for released effects; the convention and the reason it
   exists are in [id-scheme.md](id-scheme.md)). The gain effect is `XXGAIN`.
   Name, filename and icon are part of an effect's identity when you
   release it (see [ip-and-licensing.md](ip-and-licensing.md)).

## The bigger question: who gets which ids

So that DIY effects from different authors can coexist on one pedal, every
author needs ids that nobody else uses. I hand out ranges of ids above the
stock ids on request, and keep the list of who has which range in
[effect-authors.json](../ids/effect-authors.json). I use it for my own
effects, and I would like it to become a shared convention. How it works, and
how to ask for a range, is in [id-scheme.md](id-scheme.md). It is a proposal,
not a settled standard.

## Needs investigating

* Adoption of the per-author scheme. It is a proposal
  ([id-scheme.md](id-scheme.md)), so its uptake by other authors is still
  open. Until you have a range of your own, hand-check a new id against a
  readback.
* Whether more built-in entries like the BPM module exist or will appear. The
  effect list of every current MS Plus model was checked and the BPM module is
  the only entry without a file today, but a firmware update could add one.
* What the pedal and the patch format actually do when two installed effects
  share an id. Untested, deliberately.
* Whether ids have to be unique across all pedal models, or only per model.
  Unknown. Assume global uniqueness to be safe.
