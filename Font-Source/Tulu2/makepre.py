# Make pre-forms ligatures

CH_NORM = ''
NORMAL_CHAR = {}

BASE_CHARS = {
    'K'   : NORMAL_CHAR,
    'Kh'  : NORMAL_CHAR,
    'G'   : NORMAL_CHAR,
    'Gh'  : NORMAL_CHAR,
    'Ng'  : NORMAL_CHAR,
    'C'   : NORMAL_CHAR,
    'Ch'  : NORMAL_CHAR,
    'J'   : NORMAL_CHAR,
    'Jh'  : NORMAL_CHAR,
    'Ny'  : NORMAL_CHAR,
    'Tt'  : NORMAL_CHAR,
    'Tth' : NORMAL_CHAR,
    'Dd'  : NORMAL_CHAR,
    'Ddh' : NORMAL_CHAR,
    'Nn'  : NORMAL_CHAR,
    'T'   : NORMAL_CHAR,
    'Th'  : NORMAL_CHAR,
    'D'   : NORMAL_CHAR,
    'Dh'  : NORMAL_CHAR,
    'N'   : NORMAL_CHAR,
    'P'   : NORMAL_CHAR,
    'Ph'  : NORMAL_CHAR,
    'B'   : NORMAL_CHAR,
    'Bh'  : NORMAL_CHAR,
    'M'   : NORMAL_CHAR,
    'Y'   : NORMAL_CHAR,
    'R'   : NORMAL_CHAR,
    'L'   : NORMAL_CHAR,
    'V'   : NORMAL_CHAR,
    'Sh'  : NORMAL_CHAR,
    'Ss'  : NORMAL_CHAR,
    'S'   : NORMAL_CHAR,
    'H'   : NORMAL_CHAR,
    'Ll'  : NORMAL_CHAR,
    'Rr'  : NORMAL_CHAR,
    'Lll' : NORMAL_CHAR,
};


VOWELS = {
    'Ee'  : { "OoLink" : True,  },
    'Ai'  : { "OoLink" : False, },
}


def findGlyph(font, name, shouldExist):
    for glyph in font.glyphs():
        if glyph.glyphname == name:
            return glyph
    if shouldExist:
        raise Exception("Char " + name + " should exist")
    return None


NEWGLYPH_NAME = ".New.Glyph"

#
#  Ensures that glyph is in the font, returns it
#  The sign of newly-created glyph is glyphName == NEWGLYPH_NAME
#     Change it ASAP
#
def ensureGlyph(font, name):
    glyph = findGlyph(font, name, False)
    if glyph == None:
        glyph = font.createChar(-1, NEWGLYPH_NAME)
    return glyph


def findAnchor(glyph, name):
    anchors = glyph.anchorPoints
    for p in anchors:
        if p[0] == name:
            return p
    return None


def addAnchor(baseGlyph, newGlyph, name, offset, minX):
    anchor = findAnchor(baseGlyph, name)
    if anchor != None:
        type = anchor[1]
        normalX = anchor[2] + offset
        x = max(normalX, minX)
        y = anchor[3]
        newGlyph.addAnchorPoint(name, type, x, y)


#
#  Adds ligature:
#  basePrefix: smth like "Gukh." or "Shape."
#  baseCode : smth like "K" or "Kh"
#  baseProp : properties of base, e.g. which version of medial to insert
#  medCode : smth like "Y"
#  medProp : properties of medial glyph (to find proper place for virama)
#  shouldExist: [+] base character should exist, die with error
#               [-] skip it silently
#               (medial should exist, ligature is created if missing)
#
def addLig(font, baseCode, vowCode, vowProp):
    baseName = 'Tutg.' + baseCode
    vowName = 'Uml.' + vowCode
    ligName = 'Wa.' + baseCode + vowCode
    baseGlyph = findGlyph(font, baseName, True)
    vowGlyph = findGlyph(font, vowName, True)
    ligGlyph = ensureGlyph(font, ligName)
    if ligGlyph.glyphname == NEWGLYPH_NAME:
        ligGlyph.glyphname = ligName
        # Ligature: ligature ← components
        ligGlyph.addPosSub("Tutg workar-1", baseName + " " + vowName)
        if vowProp["OoLink"]:
            # Single sub: source → dest
            baseGlyph.addPosSub("Link cons+Ee-1", ligName)
    # Ligate vowel, then base
    ligGlyph.clear()
    ligGlyph.addReference(vowName)
    baseOffset = vowGlyph.width
    matrix = (1, 0, 0, 1, baseOffset, 0)    
    ligGlyph.addReference(baseName, matrix)
    ligGlyph.width = baseOffset + baseGlyph.width
    ligGlyph.color = 0x4FD5FF

# Run, run!
fontforge.runInitScripts()
font = fontforge.activeFont()
for baseCode, baseProp in BASE_CHARS.items():
    for vowCode, vowProp in VOWELS.items():
        addLig(font, baseCode, vowCode, vowProp)
