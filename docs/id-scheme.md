# A proposed per-author id scheme for DIY Zoom effects

Every ZD2 effect carries a 32-bit id, and the pedal keys its whole effect
catalog by that number. Two DIY effects that share an id cannot safely coexist
on one pedal: a patch refers to an effect by its id alone, so it can no longer
say which of the two it means. What the pedal actually does in that case is
untested. Today every DIY author picks from the same small
range by hand and hopes nobody else picked the same number. A community that
shares effects needs a namespace.

This is the convention I propose, and use for my own effects. It is a
proposal, not a settled standard. Read [effect-ids.md](effect-ids.md) first
for what an id is and where it lives; this doc is only about handing out
non-overlapping ranges.

## What the fields mean

```
id = (group << 24) | (byte98 << 16) | (index << 4) | variant
      top byte        third byte        bits 4..15     bits 0..3
```

`group` picks the browser category. `index` is the position field, and it is
category-orthogonal: Zoom reuses each `low16` value across every group, so one
range chosen in `index` is owned in every category at once. `variant` is a
small slot for versions of one effect.

Two facts open a free band on the MS Plus pedals. First, every stock effect
keeps its `index` inside one byte and leaves `byte98 = 0`, so nothing stock
lands at `low16 >= 0x2000`. I checked this across the current Zoom catalogs,
on both of Zoom's id-numbering generations: no stock id collides there. (Other
Zoom pedals use the id space more fully, but their effects are a different file
format and will not load on the MS Plus units.) Second, keeping `byte98 = 0`
is part of the convention: it is the field the clean generation never touches,
so a `byte98 = 0` range is safe everywhere.

## Author ranges, above 0x2000

Distributed effects live at `low16 0x2000` and above, and each author gets a
range of their own there. Do not pick one yourself. If you want a range, open
an issue in this repo: I assign one that fits what you plan to release and
record it in [`ids/effect-authors.json`](../ids/effect-authors.json), which is
the ledger of who has which range. The first range, `0x2000`-`0x23ff`, is
mine.

```
effect id = (group << 24) | low16
             per target      a value in your range
```

A range is yours in every category: Zoom reuses each `low16` value across all
groups, so the same range holds your effects whatever the top byte is. The
browser sorts each column by file order, not by id, so the value you choose
creates no visible ordering; it is purely for avoiding collisions.

The **group byte is set per target pedal.** Categories are numbered
differently on different models (the table is in
[effect-ids.md](effect-ids.md)), so one fixed top byte cannot be
category-correct everywhere. An effect that targets several models therefore
gets one finished file per model. Each file carries that model's group byte in
its id and that model's category name in its header and INFO chunk. The files
come from one compile, so the code inside is the same, and the
author-and-effect part of the id stays the same. A single binary does not run on every pedal
anyway (the format and ABI differ across families), so "universal" here means
the id coordinate is free everywhere, not that one file loads everywhere.

## Below 0x2000: the playground, and a reserved range

Nothing below `low16 0x2000` is ever distributed. The playground is
`low16 0x1000`-`0x13ff`: use it freely on your own pedal, reuse the same
numbers across experiments, but **do not distribute anything from it.** Before
you hand an effect to someone else, give it an id in your author range. (The
older `0x0f00` page works too, but Zoom occupies both ends of it, `0x0f00` =
LINE SEL and `0x0ff0` = the built-in BPM module, so `0x1000`-`0x13ff` is
cleaner.)

`low16 0x1400`-`0x1fff` is reserved. Do not pick ids from it.

So the whole export rule collapses to one line: refuse to distribute any
effect, or any patch that references one, whose `low16` is below `0x2000`.

Name throwaway builds with the filename prefix `XX` ("experimental and
expendable"), per the filename convention in the next section. Install
playground effects with `tools-pedal/pedal_diy.py` (which accepts the range)
and audition in the temp buffer per [SAFETY.md](../SAFETY.md).

## Effect filenames: a 2-letter author prefix

The id is not the only namespace. The pedal stores every effect by its flash
filename (8 characters or fewer, from A-Z 0-9 _), and that namespace collides
on its own terms: **installing an effect whose filename already exists on the
pedal silently keeps the resident file.** The new effect appears to install,
but the pedal keeps running the old one. So two DIY effects that share a
filename cannot coexist, whatever their ids say.

Filenames therefore follow the same convention as ids: **every author gets a
unique 2-letter prefix together with their id range**, in the same registry
row (the `prefix` field). A filename is the prefix plus up to 6 more
characters: `WFSPLIT` is `WF` + `SPLIT`. My prefix is `WF`.

One prefix is reserved and is never a personal prefix: `XX` marks playground
builds, anyone's own-pedal scratch effects. No stock or released DIY filename
starts with `XX`, so an `XX...` file is unmistakably a throwaway at a glance,
which matters because the pedal shows only the icon, never the filename.

The rule, per id: an effect with an id in an author range (`low16` at or
above `0x2000`) takes its author's registered prefix; an effect with a
playground id takes `XX`, or its author's own prefix. The build tool
(`tools/zd2_make_effect.py`) refuses a filename that does not match the
manifest's `author` and id, and the safe installer
(`tools-pedal/pedal_diy.py`) refuses an install whose filename or id collides
with a different effect already resident on the pedal. A collision fails
loudly at the desk or at install time, never silently on the pedal.

## Effect names: never a stock name

The display name is a third namespace. It is the short name stored in the
effect file, and it is what an editor or a listing tool shows for the effect.
(The pedal's own effect browser shows only icons.) Two effects sharing a name
cannot be told apart in such a listing.

Stock Zoom names are therefore off limits. The safe installer
(`tools-pedal/pedal_diy.py`) refuses to install a DIY effect whose name
matches a stock one, and the build tool (`tools/zd2_make_effect.py`) refuses
the same name at build time, which is a far cheaper place to find out:
renaming a manifest costs nothing, renaming a finished effect costs a rebuild,
a redrawn icon and a documentation pass.

The comparison covers the whole MS Plus series, not one model: a name that is
stock on the MS-60B+ is refused for an MS-70CDR+ effect too, because the
installer refuses it on any of them. Case and surrounding spaces are ignored,
so `TREMOLO` and `" Tremolo "` are refused along with `Tremolo`. Spacing
inside the name counts, so `Tremolo AW` builds fine: stock itself ships
`Tremolo`, `Tremolo Rev`, `TremDelay`, `DuoTrem` and `ORG Trem` side by side,
and a distinguishing suffix is all it takes.

The list of stock names travels with the build tool as a constant, generated
by a catalog scan of the stock effects: names only, the same identity metadata
any pedal reports over MIDI, and the same list `tools-pedal/stock_catalog.py`
carries for the installer. This repository ships no stock Zoom effect files,
so a tool that scanned for them locally would find nothing and quietly stop
checking.

If a collision is deliberate, set `"allow_stock_name_collision": true` in the
manifest and the build warns instead of refusing. That buys a build, not an
install: an exact stock-name match is still refused by
`tools-pedal/pedal_diy.py`.

## The registry

Author ranges are recorded in a small machine-readable file,
[`ids/effect-authors.json`](../ids/effect-authors.json): one row per author
with a `low16` range, a nickname and full name, a filename `prefix`, and an
optional URL. A row covers both namespaces at once: the id range and the
2-letter filename prefix. To get a row, open an issue. The build tool reads
the file for the filename prefixes. The safe installer does not read it: it
treats any id at `low16 0x2000` and above as DIY, whether or not that range is
assigned yet, so the list is what keeps authors apart, not the installer.

## The effects in this repo

These effects are **examples**. They sit in the playground range on purpose:
build them, install them, study them, then give them an id of your own before
you share anything. They carry the SFX group byte `0x07` for the MS-70CDR+:

| Effect | filename | id | `low16` |
|---|---|---|---|
| DIY Gain | `XXGAIN` | `07001000` | `0x1000` |
| WfSplit | `WFSPLIT` | `07001010` | `0x1010` |
| WfMerge | `WFMERGE` | `07001020` | `0x1020` |
| WfSwap | `WFSWAP` | `07001030` | `0x1030` |

The filenames follow the prefix convention above: the gain example is a
playground scratch effect, so it wears the `XX` marker; the routing three
keep my `WF` prefix, which is equally valid for a playground id.

I keep the first author range (`low16 0x2000`-`0x23ff`) for effects I
distribute, so nothing you build from this repo can collide with one of mine.
Copy the gain folder, change the id to a free value in a range of your own (or
anywhere in the playground while you experiment), and you have a starting point
that will not collide with anyone else's.

## Licence and credit

The scheme and the `effect-authors.json` format are my own work, MIT-licensed
like the rest of this repo. That no stock effect uses an id at `low16 0x2000`
or above comes from surveying the public Zoom catalogs. Reserving space above
the sort index is not a new idea: ELynx's ZDL-era work reserves a sort index
of its own, and this is its wider-format sibling.
