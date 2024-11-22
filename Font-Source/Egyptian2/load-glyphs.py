import fontforge
import psMat
import os
import shutil

TEMPFILENAME = '~UnicodiaSesh.ttf'
OUTFILENAME = 'UnicodiaSesh.ttf'
HINTER = 'd:/Soft/FontEditing/ttfautohint.exe'
INKSCAPE = 'c:/Program Files/Inkscape/bin/inkscape.com'

fontforge.runInitScripts()
font = fontforge.activeFont()
nHandGlyphs = sum(1 for _ in font.glyphs())
nSelfIntersecting = 0

log = open('sesh.log', 'w')

# checks whether codepoint is good
def isCpGood(code):
    return (code >= 0x13460) and (code <= 0x143FF);

log.write("Loading SVG\n");

# Gets SVG’s height in units
# No complete XML parsing
# Very basic error checking
def getSvgHeight(fname):
    # Read data
    f = open(fname, 'r')
    data = f.read()
    f.close()
    # Try to get data
    trigger1a = 'height="'
    trigger2 =  '"'
    trigger1b = "height='"   # same length!
    pos1 = data.find(trigger1a)
    if (pos1 < 0):
        pos1 = data.find(trigger1b)
        trigger2 = "'"
        if (pos1 < 0):
            raise Exception('Cannot find trigger1: {}'.format(fname))
    pos1 += len(trigger1a)
    # debug: raise Exception("{}:{}".format(fname, pos1))
    pos2 = data.find(trigger2, pos1)
    sNumber = data[pos1:pos2]
    # Some SVGs have measurement unit under 'height'
    if (sNumber.endswith('px') or sNumber.endswith('pt')):
        sNumber = sNumber[:-2]
    return float(sNumber)
    
# @return  [+] smth was deleted
def removeSmallPaths(layer, maxSize):
    nPaths = len(layer)
    wasDel = False
    for i in reversed(range(nPaths)):
        contour = layer[i]
        # Small, probably traces of intersections?
        [x1,y1,x2,y2] = contour.boundingBox()
        if (x2 - x1 <= maxSize) and (y2 - y1 <= maxSize):
            del layer[i]
            wasDel = True
    return wasDel

def removeObviousPaths(layer):
    nPaths = len(layer)
    for i in reversed(range(nPaths)):
        contour = layer[i]
        # Closed?
        if not contour.closed:
            del layer[i]
    removeSmallPaths(layer, 2)

SIMPVALUE = 0.6
SMALLSIMPVALUE = 0.6
BIGSMALLVALUE = 5

def removeMicroIntersections(layer):
    nContours = len(layer)
    for i in reversed(range(nContours)):
        contour = layer[i]
        selfInter = contour.selfIntersects()
        if selfInter:
            # just do whatever possible
            tempLayer = fontforge.layer()
            tempLayer += contour.dup()
            if len(tempLayer) != 1:
                raise Exception('Temp layer has !=1 contours')
            tempLayer.removeOverlap()
            removeSmallPaths(tempLayer, BIGSMALLVALUE)
            # Managed to get empty layer? Probably just reverse contour?
            if (len(tempLayer) != 1) or tempLayer.selfIntersects():
                tempLayer = fontforge.layer()
                tempLayer += contour.dup()
                tempLayer[0].reverseDirection()
                tempLayer.removeOverlap()
                removeSmallPaths(tempLayer, BIGSMALLVALUE)
            # Got exactly one, replace
            if (len(tempLayer) == 1) and not tempLayer.selfIntersects():
                layer[i] = tempLayer[0]

def improveGlyph(glyph, logBad):
    global nSelfIntersecting
    fg = glyph.layers[1]
    # Remove open paths
    removeObviousPaths(fg)
    # Round and add extrema
    #fg.round()
    fg.addExtrema("all")
    #fg.round()
    # Simplify to get rid of poor extrema
    removeMicroIntersections(fg)
    fg.simplify(SIMPVALUE, ['mergelines'])
    #fg.round()
    # Hint
    selfInter = fg.selfIntersects()
    glyph.foreground = fg
    isOk = False
    if not selfInter:
        glyph.correctDirection()
        isOk = True
    elif logBad:
        nSelfIntersecting += 1
        log.write("{} self-intersects, {} so far\n".format(
                glyph.glyphname, nSelfIntersecting))
    #glyph.removeOverlap()
    # Correct direction
    return isOk

CELLHEIGHT = 1000
CELLWIDTH = 1100
BOTTOMHANG = 125
BEARING = 40

def fixBearings(glyph):
    glyph.left_side_bearing = BEARING
    glyph.right_side_bearing = BEARING

def loadGlyph(glyph, fname, svgHeight, logBad):
    glyph.importOutlines(fname, scale=False, correctdir=True)
    # Get transformation matrix
    mat1 = psMat.translate(0, svgHeight - 800)  # move over baseline
    mat2 = psMat.scale(CELLHEIGHT / svgHeight) # And now to CELLSIZE
    mat3 = psMat.translate(0, -BOTTOMHANG)
    mat = psMat.compose(mat1, mat2)
    mat = psMat.compose(mat, mat3)
    glyph.transform(mat)
    # Check width by ACTUAL (not requested) width
    [x1,y1,x2,y2] = glyph.boundingBox()
    actualWidth = x2 - x1
    actualHeight = y2 - y1
    if (actualWidth < 300) and (actualHeight < 300):
        log.write("{} is really small\n".format(
                glyph.glyphname))
    if actualWidth > CELLWIDTH:
        newHeight = CELLHEIGHT * CELLWIDTH / actualWidth
        remHeight = CELLHEIGHT - newHeight
        halfRem = remHeight / 2
        mat1 = psMat.translate(0, BOTTOMHANG)
        mat2 = psMat.scale(CELLWIDTH / actualWidth)
        mat3 = psMat.translate(0, halfRem - BOTTOMHANG)
        mat = psMat.compose(mat1, mat2)
        mat = psMat.compose(mat, mat3)
        glyph.transform(mat)
    fixBearings(glyph)
    return improveGlyph(glyph, logBad)

# import hieroglyphs
def loadUnikemet():
    file = open('Unikemet.txt', 'r')
    nCps = 0
    for line0 in file:
        line = line0.strip()
        if (line != '') and (not line.startswith('#')):
            cols = line.split('\t')
            if (len(cols) >= 3):
                sCp = cols[0]
                sCommand = cols[1]
                sValue = cols[2]
                if (sCp.startswith('U+') and (sCommand == 'kEH_JSesh')):
                    sHex = sCp[2:]
                    code = int(sHex, base=16)
                    if (isCpGood(code)):
                        glyphName = "u{}_{}".format(sHex.upper(), sValue)
                        svgName = "svg/{}.svg".format(sValue)
                        cacheName = "cache/{}.svg".format(sValue)                        
                        manualName = "manual/{}_UnicodiaSesh.svg".format(glyphName)
                        svgHeight = getSvgHeight(svgName)  # requested rather than actual size
                        # Load SVG
                        glyph = font.createChar(code)
                              # both Unicode and fname, for troubleshooting
                        glyph.glyphname = glyphName
                        # Load?
                        if os.path.exists(manualName):
                            # Manual glyph
                            glyph.importOutlines(manualName, scale=False, correctdir=True)
                            glyph.simplify(SMALLSIMPVALUE, ['mergelines'])
                            fixBearings(glyph)
                            if glyph.selfIntersects():
                                log.write("{} manual, and still self-intersects!\n".format(glyphName))
                        elif os.path.exists(cacheName):
                            # Cached glyph: already ran software
                            loadGlyph(glyph, cacheName, svgHeight, True)
                        else:
                            # Unknown glyph
                            isGood = loadGlyph(glyph, svgName, svgHeight, False)
                            if not isGood:
                                # Run Inkscape
                                log.write("Forced to run Inkscape!\n")
                                cmdline = '"{}" --actions=select-all;path-union --export-filename={} {}'
                                os.system(cmdline.format(INKSCAPE, cacheName, svgName))
                                glyph.clear()
                                loadGlyph(glyph, cacheName, svgHeight, True)
                        nCps += 1
                        if nCps >= 5000:
                            return

loadUnikemet()

log.write("Generating font\n")
font.generate(TEMPFILENAME)

# Run external hinter
CMDLINE = '{} --stem-width-mode=sss --symbol {} {}'
os.system(CMDLINE.format(HINTER, TEMPFILENAME, OUTFILENAME))

log.write("OK\n")
log.close()
