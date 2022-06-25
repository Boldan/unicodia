# What is Unicodia?

It is a simple Unicode encyclopedia and replacement for Character Map. Right now Windows only.

**Lifecycle phase: 4** (beta). Usable but some problems with functionality/bugs.

**I’m in Ukraine torn with war, so I’ll release often.** See “war release” tag in Issues.

# How to build?
* Slight C++20 and std::filesystem here → so need either MSYS or recent Qt with MinGW 11
* Also need cURL (present in W10 18H2+), [7-zip](https://7-zip.org), [UTransCon](https://github.com/Mercury13/utranslator).
* Configure and run tape.bat file.
* Configure and run rel.bat file.

# How to develop?
* Download/find all tools mentioned above.
* Ensure that your Qt has MinGW 11. Or make a custom kit of some MinGW Qt and MSYS compiler; tested Qt 6.1.2, 6.1.3, 6.2.0, 6.2.4
* Compile and run AutoBuilder.
* Move UcAuto.cpp to Unicodia/Uc.
* Compile Unicodia.

# Compatibility and policies

## Platforms
**Win7/10/11 x64 only.** Rationale:
* WXP, WVista and W8 are completely abandoned by all imaginable software.
* No obstacles for x86, just untested because no one compiled Qt for x86.
* Recently checked Windows 11, and it works.

## Tofu/misrenderings
* **W10/11 should support everything possible, W7 base plane only**.
* Some base plane scripts (e.g. Georgian Nuskhuri) will not be shown the best way in W7, but will be shown.
* Of course, same for plane 1 scripts: if I find that some font supports e.g. Lycian, I’ll include it to font chain.
* Three scripts of plane 1 are considered “extremely important” and **will** be supported in W7: Phoenician, Aramaic, Brahmi. I’ll also support Gothic, just because its serif look is really “gothic”.
* Small misrenderings in descriptions are tolerable, I’ll fix them only if samples are bad, or if the font has other problems.

## Future functionality
* Own GlyphWiki loader.
* Emoji tools: ligatures, more comprehensive reference.
* Better CJK reference.

# What do utilities do?
* AutoBuilder — build UcAuto.cpp from Unicode base.
  * **Warning**: transition to older/newer Unicode requires a bit of handwork.
* PanoseTool — early tool that used to remove fonts’ declared script support. Left for history. Current Unicodia uses custom font loading code based on PanoseTool + font matching flags.
* SmartCopy — copies the file if it actually differs.
* TapeMaker — creates a tape file for graphic emoji.
* [UTranslator](https://github.com/Mercury13/utranslator) — translation tool.
