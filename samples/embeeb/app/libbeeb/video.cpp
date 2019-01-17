/*
 * video.cpp
 *
 *
 */

#include "video.hpp"
#include "libbeeb.h"
#include "beeb.h"
#include "bbctext.h"
#include <math.h>

typedef uint8_t GLYPH_ALPHA[20][16]; // i.e. a hi-res char is 16x20 pixels
typedef uintBPP_t GLYPH_COLOR[20][16];




class BBCGlyphSet {
public:
    GLYPH_ALPHA alphaGlyphs[96];
    GLYPH_COLOR* colorGlyphs[8][8][96];
    
    BBCGlyphSet(ROM_CHAR* romchars);
    GLYPH_COLOR* getColorGlyph(uint8_t ch, uint8_t bg, uint8_t fg);
};



BBCGlyphSet::BBCGlyphSet(ROM_CHAR* romchars) {
    memset(colorGlyphs, 0, sizeof(colorGlyphs));
	for (int i=0 ; i<96; i++) {
		uint8_t tmp[20][12];
		for (int y=0 ; y<10 ; y++) {
			for (int x=0 ; x<6 ; x++) {
				float p = romchars[i][y][x] ? 1 : 0;
				tmp[y*2][x*2] = p;
				tmp[y*2][x*2+1]   = p;
				tmp[y*2+1][x*2]   = p;
				tmp[y*2+1][x*2+1] = p;
				if (!p) {
					// IF pixelWest=1 AND pixelNorth=1 AND pixelNW=0 THEN SetNWSubPixel()
					if (y>0 && x>0 && romchars[i][y][x-1] && romchars[i][y-1][x] && !romchars[i][y-1][x-1]) tmp[y*2][x*2] = 1;
					// IF pixelEast=1 AND pixelNorth=1 AND pixelNE=0 THEN SetNESubPixel()
					if (y>0 && x<5 && romchars[i][y][x+1] && romchars[i][y-1][x] && !romchars[i][y-1][x+1]) tmp[y*2][x*2+1] = 1;
					// IF pixelWest=1 AND pixelSouth=1 AND pixelSW=0 THEN SetSWSubPixel()
					if (y<9 && x>0 && romchars[i][y][x-1] && romchars[i][y+1][x] && !romchars[i][y+1][x-1]) tmp[y*2+1][x*2] = 1;
					// IF pixelEast=1 AND pixelSouth=1 AND pixelSE=0 THEN SetSESubPixel()
					if (y<9 && x<5 && romchars[i][y][x+1] && romchars[i][y+1][x] && !romchars[i][y+1][x+1]) tmp[y*2+1][x*2+1] = 1;
					
				}
			}
		}
		// Expand from 12x20 to 16x20
		for (int y=0 ; y<20 ; y++) {
			for (int sx=0,dx=0 ; dx<16 ; dx+=4,sx+=3) {
                float p0 = tmp[y][sx];
                float p1 = tmp[y][sx+1];
                float p2 = tmp[y][sx+2];
				alphaGlyphs[i][y][dx+0] = 15* p0;
				alphaGlyphs[i][y][dx+1] = 15* (p0/3 + (p1*2)/3);
				alphaGlyphs[i][y][dx+2] = 15* ((p1*2)/3 + p2/3);
				alphaGlyphs[i][y][dx+3] = 15* p2;
			}
		}
	}
	
}



// Static data generated by startup code
static BBCGlyphSet* mode7normalGlyphs;
static BBCGlyphSet* mode7graphicsGlyphs;
static BBCGlyphSet* mode7separatedGlyphs;
static uintBPP_t physical_colours[8];
static uint8_t table4bppPal[4][256][16];
static uintBPP_t table4bpp[4][256][16];

// Constant static data
static const int screenAddrAdd[4]={0x4000,0x3000,0x6000,0x5800};
static const uint8_t cursorTable[8] = {0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x20};
static const uint8_t cursorFlashMask[4] = {0x00, 0x00, 0x10, 0x20};
static const uint8_t crtcmask[32]={0xFF,0xFF,0xFF,0xFF,0x7F,0x1F,0x7F,0x7F,0xF3,0x1F,0x7F,0x1F,0x3F,0xFF,0x3F,0xFF,0x3F,0xFF};


#define VDISPENABLE 1
#define HDISPENABLE 2
#define SKEWDISPENABLE 4
#define SCANLINEDISPENABLE 8
#define USERDISPENABLE 16
#define EVERYTHINGENABLED  (VDISPENABLE|HDISPENABLE|SKEWDISPENABLE|SCANLINEDISPENABLE|USERDISPENABLE)

#define halfClock (!ulactrl.isHighFreqClock)
#define interlacedSyncAndVideo (crtc.interlaceMode==3)


class VideoStaticDataInit {
public:
	VideoStaticDataInit();
};

static VideoStaticDataInit s_staticDataInit;




uintBPP_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
#ifdef USE_32BPP
# ifdef PLATFORM_WEB
    return 0xFF000000 | (b<<16) | (g<<8) | r;
# else
    return 0xFF000000 | (r<<16) | (g<<8) | b;
# endif
#else
    return ((r>>3)<<11) | ((g>>2)<<6)| (b>>3);
#endif
}

GLYPH_COLOR* BBCGlyphSet::getColorGlyph(uint8_t ch, uint8_t fg, uint8_t bg) {
    GLYPH_COLOR* glyph = colorGlyphs[bg][fg][ch];
    if (!glyph) {
        glyph = colorGlyphs[bg][fg][ch] = (GLYPH_COLOR*)malloc(sizeof(GLYPH_COLOR));
        uint8_t fgR = (fg&1) ? 255 : 0;
        uint8_t fgG = (fg&2) ? 255 : 0;
        uint8_t fgB = (fg&4) ? 255 : 0;
        uint8_t bgR = (bg&1) ? 255 : 0;
        uint8_t bgG = (bg&2) ? 255 : 0;
        uint8_t bgB = (bg&4) ? 255 : 0;
        uintBPP_t blendedColors[16];
        for (int a=0 ; a<16 ; a++) {
            float alpha = a / 15.0f;
            alpha = pow(alpha, 1.0 / 2.2);
            uint8_t blendedR = bgR + (fgR-bgR)*alpha;
            uint8_t blendedG = bgG + (fgG-bgG)*alpha;
            uint8_t blendedB = bgB + (fgB-bgB)*alpha;
            blendedColors[a] = makeColor(blendedR, blendedG, blendedB);
        }
        for (int y=0 ; y<20 ; y++) {
            for (int x=0 ; x<16 ; x++) {
                (*glyph)[y][x] = blendedColors[alphaGlyphs[ch][y][x]];
            }
        }
    }
    return glyph;
}




VideoStaticDataInit::VideoStaticDataInit() {
	

	physical_colours[0]=makeColor(0,0,0);
	physical_colours[1]=makeColor(255,0,0);
	physical_colours[2]=makeColor(0,255,0);
	physical_colours[3]=makeColor(255,255,0);
	physical_colours[4]=makeColor(0,0,255);
	physical_colours[5]=makeColor(255,0,255);
	physical_colours[6]=makeColor(0,255,255);
	physical_colours[7]=makeColor(255,255,255);

    // Generate a table of palette indexes for every possible screen byte value in all display modes
    // See the Advanced User Guide pages 380 & 381 for why this is the way it is.
    for (int temp=0;temp<256;temp++) {
		int temp2=temp;
		for (int c=0;c<16;c++) {
			uint8_t left=0;
			if (temp2&2)   left|=1;
			if (temp2&8)   left|=2;
			if (temp2&32)  left|=4;
			if (temp2&128) left|=8;
			table4bppPal[3][temp][c]=left;
			temp2<<=1; temp2|=1;
		}
		for (int c=0;c<16;c++) {
			table4bppPal[2][temp][c]=table4bppPal[3][temp][c>>1];
			table4bppPal[1][temp][c]=table4bppPal[3][temp][c>>2];
			table4bppPal[0][temp][c]=table4bppPal[3][temp][c>>3];
		}
	}

	mode7normalGlyphs = new BBCGlyphSet(teletext_characters);
	mode7graphicsGlyphs = new BBCGlyphSet(teletext_graphics);
	mode7separatedGlyphs = new BBCGlyphSet(teletext_separated_graphics);
		
}


Video::Video() {
	bitmapWidth = SURFACE_WIDTH;
	bitmapHeight = SURFACE_HEIGHT;
    mode7col = 7; // white
 
    mode7nextGlyphs = mode7normalGlyphs;
    mode7curGlyphs = mode7normalGlyphs;
    mode7heldGlyphs = mode7normalGlyphs;
    
}


void Video::setViewSize(int width, int height) {
    viewWidth = width;
    viewHeight = height;
    usedLeftPrev = SURFACE_WIDTH; // force viewed area to be recalc'd on next paint()
}


void Video::paint() {


    // Has used portion of screen changed? NB: only check on odd frames cos of interlacing
    if (frameCount&1 && (usedLeft!=usedLeftPrev || usedRight!=usedRightPrev || usedTop!=usedTopPrev || usedBottom!=usedBottomPrev)) {

        // Clear any area that may have been written during previous video configs
        if (usedBottom < usedBottomPrev) {
            memset(bitmapPixels+usedBottom*bitmapStride, 0, (usedBottomPrev-usedBottom)*bitmapStride);
        }

        usedLeftPrev = usedLeft;
        usedRightPrev = usedRight;
        usedTopPrev = usedTop;
        usedBottomPrev = usedBottom;
        
        // Correct maxima
        usedRight += (ulactrl.isHighFreqClock ? 8 : 16);
        usedBottom ++;

        int usedWidth = usedRight-usedLeft;
        int usedHeightActual = usedBottom-usedTop;
        float midY = (usedTop+usedBottom)/2;

        // In graphics modes scanlines are doubled. Rather than double the drawing work we draw
        // a half-height image (i.e. single scanlines) and let the viewport scale it
        bool doubledScanlines = !ulactrl.teletextMode;
        int usedHeightApparent = doubledScanlines ? (usedHeightActual << 1) : usedHeightActual;
        
        // Center the displayed part of the Beeb's screen so it's as large as possible within the view
        float viewAspect = viewWidth/(float)viewHeight;
        float usedAspect = usedWidth / (float)usedHeightApparent;
        if (viewAspect > usedAspect) {
            float midX = (usedLeft+usedRight)/2;
            usedWidth = usedHeightApparent * viewAspect;
            usedLeft = midX - usedWidth/2;
            usedRight = midX + usedWidth/2;
        } else {
            usedHeightApparent = usedWidth / viewAspect;
            usedHeightActual = doubledScanlines ? (usedHeightApparent>>1):usedHeightApparent;
            usedTop = midY - usedHeightActual/2;
            usedBottom = midY + usedHeightActual/2;
        }

    
        the_beeb->displayCallbacks->setVisibleArea(usedLeft, usedTop, usedWidth, usedHeightActual);
        
        usedLeft = usedTop = SURFACE_WIDTH;
        usedRight = usedBottom = 0;
    }
    
    // Decrement dirty counter (controls whether invalid display regions are cleared or not during rasterization)
    if (dirtyFrameCount > 0) {
        dirtyFrameCount --;
	}
    
	the_beeb->displayCallbacks->drawFrame();
}

void Video::resetcrtc() {
	horzCounter=vertCounter=scanlineCounter=0;
	crtc.scanLinesPerChar=10;
}

void Video::writecrtc(uint16_t addr, uint8_t val) {
	if (!(addr&1)) {
		crtci=val&31;
	} else {
		crtc.raw[crtci] = val&crtcmask[crtci];
        if (crtci == 8) {
            uint8_t skew = (val & 0x30) >> 4;
            if (skew < 3) {
                dispEnabled |= USERDISPENABLE;
            } else {
                dispEnabled &= ~USERDISPENABLE;
            }
        }
        if (crtci < 10) {
            dirtyFrameCount = 4;
        }
	}
}

uint8_t Video::readcrtc(uint16_t addr) {
	if (!(addr&1)) return crtci;
	return crtc.raw[crtci];
}



void Video::writeula(uint16_t addr, uint8_t val) {
	//int c;
	if (!(addr&1)) {
    
        // If flash is changing
		if ((ulactrl.raw^val)&1) {
			if (val&1) {
				for (int c=0;c<16;c++) {
					if (bakpal[c]&8) {
                        ulapal[c]=physical_colours[bakpal[c]&7];
                    }
					else {
                        ulapal[c]=physical_colours[(bakpal[c]&7)^7];
                    }
				}
			}
			else {
				for (int c=0;c<16;c++) {
					ulapal[c]=physical_colours[(bakpal[c]&7)^7];
                }
			}
		}
		ulactrl.raw=val;
	}
	else {
		bakpal[val>>4]=val&15;
		ulapal[val>>4]=physical_colours[(val&7)^7];
		if (val&8 && ulactrl.selectedFlashColour) {
            ulapal[val>>4]=physical_colours[val&7];
        }
    }
    
    // Recreate the graphics lookup table
    for (int i=0; i<4 ; i++) {
        for (int j=0; j<256 ; j++) {
            for (int k=0; k<16 ; k++) {
                table4bpp[i][j][k] = ulapal[table4bppPal[i][j][k]];
            }
        }
    }

}

// A single read+write that blits 1,2, or 4 pixels depending on machine word size and color depth
#define NB *p++=*q++
#define NF(c) *p++=c
#ifdef USE_32BPP
 #ifdef __LP64__
  #define BLIT8 NB;NB;NB;NB;
  #define FILL8(c) NF(c);NF(c);NF(c);NF(c);
  #define CLEARCOL 0xff000000ff000000
 #else
  #define BLIT8 NB;NB;NB;NB;NB;NB;NB;NB;
  #define FILL8(c) NF(c);NF(c);NF(c);NF(c);NF(c);NF(c);NF(c);NF(c);
  #define CLEARCOL 0xff000000
 #endif
#else
 #ifdef __LP64__
  #define BLIT8 NB;NB;
  #define FILL8(c) NF(c);NF(c);
  #define CLEARCOL 0
 #else
  #define BLIT8 NB;NB;NB;NB;
  #define FILL8(c) NF(c);NF(c);NF(c);NF(c);
  #define CLEARCOL 0
 #endif
#endif




void Video::renderColour(int offset, uintN_t col) {
	uintN_t* p = (uintN_t*)(bitmapPixels + offset*sizeof(uintBPP_t));
#ifdef USE_32BPP
 #ifdef __LP64__
	col = col | (col << 32);
 #endif
#else
 #ifdef __LP64__
	col = col | (col << 16) | (col << 32) | (col << 48);
 #else
	col = col | (col << 16);
#endif
#endif
	FILL8(col);
    FILL8(col);
}

void Video::renderMode7charline(BBCGlyphSet* glyphSet, uint8_t dat, int x, int row, uint8_t bg, uint8_t fg) {
    GLYPH_COLOR* glyph = glyphSet->getColorGlyph((dat>=96)?0:dat, bg, fg);
	uintN_t* p = (uintN_t*)(bitmapPixels + x*sizeof(uintBPP_t));
    uintN_t* q = (uintN_t*)&(*glyph)[row][0];
    BLIT8;BLIT8;
}




void Video::pollvideo(int aclocks) {

    while (aclocks--) {
        clocks++;
        if (!halfClock || (clocks&1)) {

            // Handle HSync
            if (inHSync) {
                hpulseCounter = (hpulseCounter + 1) & 0x0F;
                if (hpulseCounter == (crtc.horzPulseWidth >> 1)) {
                    bitmapX = 0;

                    // Half-clock horizontal movement
                    if (crtc.horzPulseWidth & 1) {
                        bitmapX -= 4;
                    }

                    bitmapY++;
                    // If no VSync occurs this frame, go back to the top and force a repaint
                    if (bitmapY >= 384) {
                        // Arbitrary moment when TV will give up and start flyback in the absence of an explicit VSync signal
                        bitmapY = 0;
                        paint();
                    }
                } else if (hpulseCounter == crtc.horzPulseWidth) {
                    inHSync = false;
                }
            }

            // Handle delayed display enable due to skew
            if (horzCounter == crtc.displayBlankingDelay + (ulactrl.teletextMode ? 2 : 0)) {
                dispEnabled |= SKEWDISPENABLE;
            }

            // Latch next line screen address in case we are in the last line of a character row
            if (horzCounter == crtc.horzDisplayed) {
                nextLineStartAddr = addr;
            }

            // Handle end of horizontal displayed, accounting for display enable skew
            if (horzCounter == crtc.horzDisplayed + crtc.displayBlankingDelay + (ulactrl.teletextMode ? 2 : 0)) {
                dispEnabled &= ~(HDISPENABLE | SKEWDISPENABLE);
            }

            // Initiate HSync
            if (horzCounter == crtc.horzSyncPos) {
                inHSync = true;
                hpulseCounter = 0;
            }

            // Handle cursor
            uint16_t cursorPos = (crtc.cursorAddressHi<<8) | crtc.cursorAddressLo;
            if (horzCounter < crtc.horzDisplayed && cursorOn && !((addr ^ cursorPos) & 0x3fff)) {
                cursorDrawIndex = 3 - crtc.cursorBlankingDelay;
            }

            // Read data from address pointer if both horizontal and vertical display enabled
            uint8_t dat = 0;
            if ((dispEnabled & (HDISPENABLE | VDISPENABLE)) == (HDISPENABLE | VDISPENABLE)) {

                if (addr & 0x2000) {
                    // Mode 7 chunky addressing mode if MA13 set; address offset by scanline is ignored
                    dat = the_beeb->cpu.mem[0x7c00 | (addr & 0x3ff)];
                } else {
                    int laddr = (addrLine & 0x07) | (addr << 3);
                    // Perform screen address wrap around if MA12 set
                    if (addr & 0x1000) laddr += screenAddrAdd[the_beeb->sysvia.scrsize];
                    dat = the_beeb->cpu.mem[laddr & 0x7fff];
                }
            
                if (ulactrl.teletextMode) {
                    mode7dataQueue[0]=mode7dataQueue[1];
                    mode7dataQueue[1]=mode7dataQueue[2];
                    mode7dataQueue[2]=mode7dataQueue[3];
                    mode7dataQueue[3]=dat&0x7f;
                }

                addr++;
            }

            // Render data or border depending on display enable state
			uint16_t renderY = bitmapY; //
			if (ulactrl.teletextMode) {
				renderY = (bitmapY << 1) | (oddFrame ? 1 : 0);
			}
            if (bitmapX >= 0 && bitmapX < SURFACE_WIDTH && renderY < SURFACE_HEIGHT) {
            
                int offset = renderY * SURFACE_WIDTH + bitmapX;
                //bool doubled = !interlacedSyncAndVideo;
                if ((dispEnabled & EVERYTHINGENABLED) == EVERYTHINGENABLED) {
                
                    // Update min/max
                    if (bitmapX<usedLeft) usedLeft=bitmapX;
                    if (bitmapX>usedRight) usedRight=bitmapX;
                    if (renderY<usedTop) usedTop=renderY;
                    if (renderY>usedBottom) usedBottom=renderY;
                    
                    // Render 8 or 16 pixels!
                    if (ulactrl.teletextMode) {
                        teletextRender(offset, (scanlineCounter << 1) | (oddFrame ? 1 : 0));
                    } else {
                        uintN_t* p = (uintN_t*)(bitmapPixels + offset*sizeof(uintBPP_t));
                        uintN_t* q = (uintN_t*)table4bpp[ulactrl.charsPerLine][dat];
                        BLIT8;
                        if (!ulactrl.isHighFreqClock) {
                            BLIT8;
                        }
                    }
                } else {
                    if (dirtyFrameCount > 0) {
                        uintN_t* p = (uintN_t*)(bitmapPixels + offset*sizeof(uintBPP_t));
                        FILL8(CLEARCOL);
                        if (!ulactrl.isHighFreqClock) {
                            FILL8(CLEARCOL);
                        }
                    }
                    //if (renderY>highRenderY) highRenderY = renderY;
                }

                // Draw the cursor
                if (cursorDrawIndex) {
                    if (cursorOnThisFrame && (ulactrl.raw & cursorTable[cursorDrawIndex])) {
                        int pixelsPerChar = ulactrl.isHighFreqClock ? 8 : 16;
                        for (int i = 0; i < pixelsPerChar; ++i) {
                            *(uintBPP_t*)(bitmapPixels + (offset + i)*sizeof(uintBPP_t)) ^= 0xFFFFFF;// (uintBPP_t)-1;
                        }
                    }
                    if (++cursorDrawIndex == 7) cursorDrawIndex = 0;
                }
            }

            // Handle horizontal total
            if (drawHalfScanline && horzCounter == (crtc.horzTotal >> 1)) {
                // In interlace mode, the odd field is displaced from the even field by rasterizing
                // half a scanline directly after the VBlank in odd fields and forcing HBlank
                // immediately (since the vertical speed of the raster beam is constant). This is
                // then adjusted for even fields by rasterizing a further half a scanline before their
                // VBlank.
                horzCounter = 0;
                drawHalfScanline = false;
            } else if (horzCounter == crtc.horzTotal) {
                // We've hit the end of a line
                endOfLine();
                horzCounter = 0;
                dispEnabled |= HDISPENABLE;
            } else {
                horzCounter = (horzCounter + 1) & 0xff;
            }
        }

        bitmapX += 8;

    } // matches while

}

void Video::endOfLine() {
    uint8_t cursorEnd = interlacedSyncAndVideo ? (crtc.cursorEndRaster >> 1) : crtc.cursorEndRaster;
    if (scanlineCounter == cursorEnd) {
        cursorOn = false;
        cursorOff = true;
    }

    // Handle VSync
    if (inVSync) {
        vpulseCounter = (vpulseCounter + 1) & 0x0F;
        if (vpulseCounter == crtc.vertPulseWidth) {
            inVSync = false;
            if (oddFrame) drawHalfScanline = (crtc.interlaceMode & 1);
            the_beeb->sysvia.setVblankInt(false);
        }
    }

    int numScanlines = inVertAdjust ? (crtc.vertTotalAdj - 1) : (interlacedSyncAndVideo ? (crtc.scanLinesPerChar >> 1) : crtc.scanLinesPerChar);
    if (scanlineCounter == numScanlines) {
        // New screen row
        if (inVertAdjust) {
            // Finished vertical adjust
            endOfFrame();
            inVertAdjust = false;
        } else {
            // Handle vertical total
            if (vertCounter == crtc.vertTotal) {
                if (crtc.vertTotalAdj == 0) {
                    endOfFrame();
                } else {
                    inVertAdjust = true;
                }
            } else {
                // Still updating screen
                vertCounter = (vertCounter + 1) & 0x7F;

                // Initiate vsync
                if (vertCounter == crtc.vertSyncPos) {
                    inVSync = true;
                    vpulseCounter = 0;

                    oddFrame = !oddFrame;
                    if (oddFrame) drawHalfScanline = (crtc.interlaceMode & 1);
                    //if (clocks > 2) { // TODO: wat?
                        paint();
                    //}
                    bitmapY = 0;
                    the_beeb->sysvia.setVblankInt(true);
                    //this.teletext.vsync();
                    if (++mode7flashTime == 48) mode7flashTime = 0;
                    mode7flashOn = mode7flashTime < 16;
                    clocks = 0;
                    
                }
            }
        }

        scanlineCounter = 0;
        
        // teletext_verticalCharEnd();
        if (mode7secondHalfOfDouble) {
            mode7secondHalfOfDouble = false;
        } else {
            mode7secondHalfOfDouble = mode7wasDbl;
        }
        
        lineStartAddr = nextLineStartAddr;
        addrLine = (interlacedSyncAndVideo && oddFrame) ? 1 : 0;
        dispEnabled |= SCANLINEDISPENABLE;
        cursorOn = cursorOff = false;

        // Handle vertical displayed
        if (vertCounter == crtc.vertDisplayed) {
            dispEnabled &= ~VDISPENABLE;
        }
    } else {
        // Move to the next scanline
        scanlineCounter = (scanlineCounter + 1) & 0x1F;
        if (scanlineCounter == 8 && !ulactrl.teletextMode) {
            dispEnabled &= ~SCANLINEDISPENABLE;
        }
        addrLine += (interlacedSyncAndVideo ? 2 : 1);
    }

    addr = lineStartAddr;
    
    //teletext_endline();
    mode7col = 7; // white
    mode7bg = 0; // black
    mode7holdChar = false;
    mode7heldChar = 0x20;
    mode7nextGlyphs = mode7heldGlyphs = mode7normalGlyphs;
    mode7flash = false;
    mode7sep = false;
    mode7gfx = false;
    mode7dbl = mode7wasDbl = false;

    uint8_t cursorStartLine = crtc.cursorStartRaster & 31;
    if (!cursorOff && (scanlineCounter == cursorStartLine || (interlacedSyncAndVideo && scanlineCounter == (cursorStartLine >> 1)))) {
        cursorOn = true;
    }
}

void Video::endOfFrame() {
    vertCounter = 0;
    nextLineStartAddr = (crtc.displayStartAddrLo | (crtc.displayStartAddrHi << 8)) & 0x3FFF;
    dispEnabled |= VDISPENABLE;
    frameCount++;
    uint8_t cursorFlash = (crtc.cursorStartRaster & 0x60) >> 5;
    cursorOnThisFrame = (cursorFlash == 0) || (frameCount & cursorFlashMask[cursorFlash]);
}

void Video::teletextRender(int offset, uint8_t scanline) {
    uint8_t data = mode7dataQueue[0];
    mode7oldDbl = mode7dbl;

    mode7prevCol = mode7col;
    mode7curGlyphs = mode7nextGlyphs;

    bool prevFlash = mode7flash;
    if (data < 0x20) {
        mode7holdClear = false;
        mode7holdOff = false;

        switch (data) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                mode7gfx = false;
                mode7col = data;
                teletextSetNextChars();
                mode7holdClear = true;
                break;
            case 8:
                mode7flash = true;
                break;
            case 9:
                mode7flash = false;
                break;
            case 12:
            case 13:
                mode7dbl = (data & 1);
                if (mode7dbl) mode7wasDbl = true;
                break;
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
                mode7gfx = true;
                mode7col = data & 7;
                teletextSetNextChars();
                break;
            case 24:
                mode7col = mode7prevCol = mode7bg;
                break;
            case 25:
                mode7sep = false;
                teletextSetNextChars();
                break;
            case 26:
                mode7sep = true;
                teletextSetNextChars();
                break;
            case 28:
                mode7bg = 0;
                break;
            case 29:
                mode7bg = mode7col;
                break;
            case 30:
                mode7holdChar = true;
                break;
            case 31:
                mode7holdOff = true;
                break;
        }
        if (mode7holdChar && mode7dbl == mode7oldDbl) {
            data = mode7heldChar;
            if (data >= 0x40 && data < 0x60) data = 0x20;
            mode7curGlyphs = mode7heldGlyphs;
        } else {
            data = 0x20;
        }
    
    } else if (mode7gfx) {
        mode7heldChar = data;
        mode7heldGlyphs = mode7curGlyphs;
    }

    if (mode7oldDbl) {
        scanline = (scanline >> 1);
        if (mode7secondHalfOfDouble) {
            scanline += 10;
        }
    }

    if ((prevFlash && mode7flashOn) || (mode7secondHalfOfDouble && !mode7dbl)) {
        renderColour(offset, physical_colours[mode7bg&7]);
    } else {
        renderMode7charline(mode7curGlyphs, data -32, offset, scanline, mode7prevCol, mode7bg);
    }

    if (mode7holdOff) {
        mode7holdChar = false;
        mode7heldChar = 32;
    }
    if (mode7holdClear) {
        mode7heldChar = 32;
    }
}

void Video::teletextSetNextChars() {
    if (mode7gfx) {
        mode7nextGlyphs = mode7sep ? mode7separatedGlyphs : mode7graphicsGlyphs;
    } else {
        mode7nextGlyphs = mode7normalGlyphs;
    }
}



uint8_t* Video::serialize(bool saving, uint8_t* p) {
	SERIALIZE(horzCounter);
	SERIALIZE(vertCounter);
	SERIALIZE(crtc);
	SERIALIZE(crtci);
	SERIALIZE(ulactrl);
	SERIALIZE(bakpal);
	SERIALIZE(clocks);
	SERIALIZE(mode7col);
	SERIALIZE(mode7bg);
	SERIALIZE(mode7sep);
	SERIALIZE(mode7dbl);
	SERIALIZE(mode7gfx);
	SERIALIZE(mode7flash);
	return p;
}
