/*
 * Copyright (C) 2003 ViASIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

#ifndef UTBOX_H
#define UTBOX_H

typedef struct utLine_ utLine;
typedef struct utTranslation_ utTranslation;
typedef struct utBox_ utBox;
typedef struct utPoint_ utPoint;

/*--------------------------------------------------------------------------------------------------
   This structure describes how to translate a coordinate system.  To
   translate, first reflect points about the x-axis, then rotate in the
   clockwise direction rotation*90 degrees, then move by the x and y offsets
--------------------------------------------------------------------------------------------------*/
struct utTranslation_ {
    int32 x, y;
    uint8 rotation; 
    bool reflect;
};
#define utTranslationGetX(translation) ((translation).x)
#define utTranslationGetY(translation) ((translation).y)
#define utTranslationGetRotation(translation) ((translation).rotation)
#define utTranslationSetX(translation, val) ((translation).x = (val))
#define utTranslationSetY(translation, val) ((translation).y = (val))
#define utTranslationSetRotation(translation, val) ((translation).rotation = (val))
#define utTranslationReflect(translation) ((translation).reflect)
extern utTranslation utMakeTranslation(bool reflect, uint8 rotation, int32 x, int32 y);
#define utMakeIdentityTranslation() utMakeTranslation(false, 0, 0, 0)
extern utTranslation utCombineTranslations(utTranslation first, utTranslation second);
extern utTranslation utTranslationInvert(utTranslation translation);
extern bool utTranslationsEqual(utTranslation translation1, utTranslation translation2);

struct utPoint_ {
    int32 x, y;
};

#define utPointGetX(point) ((point).x)
#define utPointGetY(point) ((point).y)
#define utPointSetX(point, value) (utPointGetX(point) = (value))
#define utPointSetY(point, value) (utPointGetY(point) = (value))
extern utPoint utMakePoint(int32 x, int32 y);
extern utPoint utTranslatePoint(utPoint point, utTranslation translation);
extern bool utPointsEqual(utPoint point1, utPoint point2);

struct utBox_ {
    int32 left, bottom, right, top;
};

#define utBoxGetLeft(box) ((box).left)
#define utBoxGetBottom(box) ((box).bottom)
#define utBoxGetRight(box) ((box).right)
#define utBoxGetTop(box) ((box).top)
#define utBoxSetLeft(box, value) (utBoxGetLeft(box) = (value))
#define utBoxSetBottom(box, value) (utBoxGetBottom(box) = (value))
#define utBoxSetRight(box, value) (utBoxGetRight(box) = (value))
#define utBoxSetTop(box, value) (utBoxGetTop(box) = (value))
extern void utPrintBox(char * desc, utBox box);
#if __GNUC__   /* see utbox.c for rationale - macro is faster */
#define utMakeBox(left,bottom,right,top) ((utBox){(int32) (left),(int32) (bottom),(int32) (right),(int32) (top)})
#else
extern utBox utMakeBox(int32 left, int32 bottom, int32 right, int32 top);
#endif
extern utBox utMakeEmptyBox(void);
extern utBox utMakeFullBox(void);
extern utBox utCorrectBox(utBox box);
extern bool utBoxCorrect(utBox box);
extern bool utBoxEven(utBox box);
extern bool utPointEven(utPoint point);
extern utBox utBoxUnion(utBox box1, utBox box2);
extern utBox utExpandBox(utBox box, int32 x, int32 y);
extern utBox utBoxIntersection(utBox box1, utBox box2);
#define utBoxesIntersect(box1, box2)                \
      ((utBoxGetRight(box1) >= utBoxGetLeft(box2)) &&   \
       (utBoxGetLeft(box1) <= utBoxGetRight(box2)) &&   \
       (utBoxGetTop(box1) >= utBoxGetBottom(box2)) &&   \
       (utBoxGetBottom(box1) <= utBoxGetTop(box2)))         /* Touching counts. */
#define utBoxesOverlap(box1, box2)                  \
      ((utBoxGetRight(box1) > utBoxGetLeft(box2)) &&    \
       (utBoxGetLeft(box1) < utBoxGetRight(box2)) &&    \
       (utBoxGetTop(box1) > utBoxGetBottom(box2)) &&    \
       (utBoxGetBottom(box1) < utBoxGetTop(box2)))          /* Touching does not count. */
extern bool utBoxEmpty(utBox box);
extern bool utBoxContainsPoint(utBox box, int32 x, int32 y);
extern bool utBoxContainsBox(utBox box1, utBox box2);
extern utBox utEnlargeBox(utBox box, int32 value);
extern utBox utMoveBox(utBox box, int32 deltaX, int32 deltaY);
extern int utCompareBoxes(utBox box1, utBox box2);              /* for qsort */
extern uint32 utDistanceBetweenBoxAndPoint(utBox box, int32 x, int32 y);
extern uint32 utBoxGetWidth(utBox box);
extern int32 utBoxGetCenterX(utBox box);
extern uint32 utBoxGetHeight(utBox box);
extern int32 utBoxGetCenterY(utBox box);
extern uint32 utFindBoxLargestDimension(utBox box);
extern uint32 utFindBoxSmallestDimension(utBox box);
extern void utConvertBoxToLineAndWidth(utBox box, utBox * line, uint32 * width);
extern utBox utTranslateBox(utBox box, utTranslation translation);
extern bool utBoxesEqual(utBox a, utBox b);
extern bool utBoxCrossesBox(utBox a, utBox b);
extern bool utBoxCrossesBoxInDirection(utBox a, utBox b, bool horizontal);
extern bool utLineDividesBox(utBox line, utBox box);
extern utBox utMakeBoxPoint(int32 x, int32 y);
#define utShrinkBox(box, value) utEnlargeBox(box, -(value))
extern utBox utBoxGetMiddle(utBox box);
extern uint32 utFindPointBoxEdgeDist(utBox point, utBox box);
extern bool utBoxIsLine(utBox box);
extern bool utBoxIsLine(utBox box);
extern bool utBoxIsPoint(utBox box);
extern uint32 utFindBoxVertDist(utBox a, utBox b);
extern uint32 utFindBoxHorDist(utBox a, utBox b);
extern uint32 utFindBoxDist(utBox a, utBox b);
extern bool utBoxesPerpendicular(utBox a, utBox b);
extern bool utBoxIsHorizontal(utBox box);
extern bool utBoxVertcal(utBox box);
extern bool utBoxIsHorLine(utBox box);
extern bool utBoxIsVertLine(utBox box);
extern bool utBoxValid(utBox box);
extern bool utBoxInBox(utBox s, utBox b);
extern bool utBoxCoveredByBox(utBox s, utBox b);
extern utBox utFindBoxOnBoxNearestBox(utBox a, utBox b);
extern int64 utFindBoxAreaint64(utBox box);
extern uint32 utFindBoxAreauint32(utBox box);

struct utLine_ {
    utPoint start, stop;
};

#define utLineGetStartPoint(line) ((line).start)
#define utLineGetStopPoint(line) ((line).stop)
#define utLineGetX1(line) (utPointGetX(utLineGetStartPoint(line)))
#define utLineGetY1(line) (utPointGetY(utLineGetStartPoint(line)))
#define utLineGetX2(line) (utPointGetX(utLineGetStopPoint(line)))
#define utLineGetY2(line) (utPointGetY(utLineGetStopPoint(line)))
#define utLineSetStartPoint(line, point) ((line).start = (point))
#define utLineSetStopPoint(line, point) ((line).stop = (point))
#define utLineSetX1(line, x_) ((line).start.x = (x_))
#define utLineSetY1(line, y_) ((line).start.y = (y_))
#define utLineSetX2(line, x_) ((line).stop.x = (x_))
#define utLineSetY2(line, y_) ((line).stop.y = (y_))
extern utLine utMakeLineFromPoints(utPoint start, utPoint stop);
extern utLine utMakeLine(int32 x1, int32 y1, int32 x2, int32 y2);
extern utBox utFindLineBox(utLine line);
extern utLine utMoveLine(utLine line, int32 deltaX, int32 deltaY);
extern uint32 utLineGetDX(utLine line);
extern uint32 utLineGetDY(utLine line);
extern utLine utTranslateLine(utLine line, utTranslation translation);
extern bool utLineHorizontal(utLine a);
extern bool utLineVertical(utLine a);
extern bool utLineRectalinear(utLine a);
extern bool utLinesEqual(utLine a, utLine b);
extern bool utLinesIntersect(utLine a, utLine b);

#endif  /* UTBOX_H */

