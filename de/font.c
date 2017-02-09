#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/log.h>
#include "font.h"
#include "fontset.h"
#include "brush.h"
#include "precompose.h"

static DEFont *fonts = NULL;

DEFont *de_load_font(const char *fontname) {
  XftFont *font;
  DEFont *fnt;

  assert(fontname != NULL);

  /* There shouldn't be that many fonts... */
  for (fnt = fonts; fnt != NULL; fnt = fnt->next) {
    if (strcmp(fnt->pattern, fontname) == 0) {
      fnt->refcount++;
      return fnt;
    }
  }

#define CF_FALLBACK_FONT_NAME "fixed"
    if(strncmp(fontname, "xft:", 4)==0){
        font=XftFontOpenName(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname+4);
    }else{
        font=XftFontOpenXlfd(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname);
    }

    if(font==NULL){
        if(strcmp(fontname, CF_FALLBACK_FONT_NAME)!=0){
            warn("Could not load font \"%s\", trying \"%s\"",
             fontname, CF_FALLBACK_FONT_NAME);
            return de_load_font(CF_FALLBACK_FONT_NAME);
        }
        return NULL;
    }

  fnt = ALLOC(DEFont);

  if (fnt == NULL) return NULL;

  fnt->font=font;
  fnt->pattern = scopy(fontname);
  fnt->next = NULL;
  fnt->prev = NULL;
  fnt->refcount = 1;

  LINK_ITEM(fonts, fnt, next, prev);

  return fnt;
}

bool de_set_font_for_style(DEStyle *style, DEFont *font) {
  if (style->font != NULL) de_free_font(style->font);

  style->font = font;
  font->refcount++;
  return TRUE;
}

bool de_load_font_for_style(DEStyle *style, const char *fontname) {
  if (style->font != NULL) de_free_font(style->font);
  style->font = de_load_font(fontname);
  return TRUE;
}

void de_free_font(DEFont *font) {
  if (--font->refcount != 0) return;
  if(font->font!=NULL){
      XftFontClose(ioncore_g.dpy, font->font);
  }
  UNLINK_ITEM(fonts, font, next, prev);
  free(font);
}

void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte) {
  if (brush->d->font == NULL) {
    DE_RESET_FONT_EXTENTS(fnte);
    return;
  }

  defont_get_font_extents(brush->d->font, fnte);
}

void defont_get_font_extents(DEFont *font, GrFontExtents *fnte) {
    if(font->font!=NULL){
        fnte->max_height=font->font->ascent+font->font->descent;
        fnte->max_width=font->font->max_advance_width;
        fnte->baseline=font->font->ascent;
        return;
    }
}

uint debrush_get_text_width(DEBrush *brush, const char *text, uint len) {
  if (brush->d->font == NULL || text == NULL || len == 0) return 0;

  return defont_get_text_width(brush->d->font, text, len);
}

uint defont_get_text_width(DEFont *font, const char *text, uint len) {
    if(font->font!=NULL){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8)
            XftTextExtentsUtf8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        else
            XftTextExtents8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        return extents.xOff;
    }else{
        return 0;
    }
}

void debrush_do_draw_string_default(DEBrush *brush, int x, int y, const char *str, int len, bool needfill, DEColourGroup *colours) {
    Window win = brush->win;
    XftDraw *draw;
    XftFont *font;

    if(brush->d->font==NULL)
        return;

    font=brush->d->font->font;
    draw=debrush_get_draw(brush, win);

    if(needfill){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8){
            XftTextExtentsUtf8(ioncore_g.dpy, font, (XftChar8*)str, len,
                               &extents);
        }else{
            XftTextExtents8(ioncore_g.dpy, font, (XftChar8*)str, len, &extents);
            XftDrawRect(draw, &(colours->bg), x-extents.x, y-extents.y,
                        extents.width+10, extents.height);
        }
    }

    if(ioncore_g.enc_utf8){
        XftDrawStringUtf8(draw, &(colours->fg), font, x, y, (XftChar8*)str,
                          len);
    }else{
        XftDrawString8(draw, &(colours->fg), font, x, y, (XftChar8*)str, len);
    }
}

void debrush_do_draw_string(DEBrush *brush, int x, int y, const char *str, int len, bool needfill, DEColourGroup *colours) {
  CALL_DYN(debrush_do_draw_string, brush, (brush, x, y, str, len, needfill, colours));
}

void debrush_draw_string(DEBrush *brush, int x, int y, const char *str, int len, bool needfill) {
  DEColourGroup *cg = debrush_get_current_colour_group(brush);
  if (cg != NULL) debrush_do_draw_string(brush, x, y, str, len, needfill, cg);
}
