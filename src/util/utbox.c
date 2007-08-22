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

#include "ddutil.h"
#include "utbox.h"

/*--------------------------------------------------------------------------------------------------
  Make a new box (rectangle) value.
  This is now a macro since that allows the compiler to inline this simple piece of code.
  Example speedup: 13.13 sec as function, 12.69 as macro = 3.4% speedup for free.
--------------------------------------------------------------------------------------------------*/
#ifndef utMakeBox
utBox utMakeBox(
    int32 left,
    int32 bottom,
    int32 right,
    int32 top)
{
    utBox box;

    utBoxSetLeft(box, left);
    utBoxSetBottom(box, bottom);
    utBoxSetRight(box, right);
    utBoxSetTop(box, top);
    return box;
}
#endif

/*--------------------------------------------------------------------------------------------------
  Make an empty box.
  // major optimization: use a global constant variable instead of a non-inlineable function.
--------------------------------------------------------------------------------------------------*/
utBox utMakeEmptyBox(void)
{
    utBox box;

    utBoxSetLeft(box, INT32_MAX);
    utBoxSetBottom(box, INT32_MAX);
    utBoxSetRight(box, INT32_MIN);
    utBoxSetTop(box, INT32_MIN);
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Make a full box.
  // major optimization: use a global constant variable instead of a non-inlineable function.
--------------------------------------------------------------------------------------------------*/
utBox utMakeFullBox(void)
{
    utBox box;

    utBoxSetLeft(box, INT32_MIN);
    utBoxSetBottom(box, INT32_MIN);
    utBoxSetRight(box, INT32_MAX);
    utBoxSetTop(box, INT32_MAX);
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Swap left with right and bottom with top if needed to have normal order.
--------------------------------------------------------------------------------------------------*/
utBox utCorrectBox(
    utBox box)
{
    int32 temp;

    if(utBoxGetLeft(box) > utBoxGetRight(box)) {
        temp = utBoxGetLeft(box);
        utBoxGetLeft(box) = utBoxGetRight(box);
        utBoxGetRight(box) = temp;
    }
    if(utBoxGetTop(box) < utBoxGetBottom(box)) {
        temp = utBoxGetTop(box);
        utBoxGetTop(box) = utBoxGetBottom(box);
        utBoxGetBottom(box) = temp;
    }
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the box is correct in the sense that utCorrectBox would not change it.
--------------------------------------------------------------------------------------------------*/
bool utBoxCorrect(
    utBox box)
{
    if(utBoxGetLeft(box) > utBoxGetRight(box)) {
        return false;
    }
    if(utBoxGetTop(box) < utBoxGetBottom(box)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Find the union of two rectangles.
--------------------------------------------------------------------------------------------------*/
utBox utBoxUnion(
    utBox box1,
    utBox box2)
{
    utBoxSetLeft(box1, utMin(utBoxGetLeft(box1), utBoxGetLeft(box2)));
    utBoxSetBottom(box1, utMin(utBoxGetBottom(box1), utBoxGetBottom(box2)));
    utBoxSetRight(box1, utMax(utBoxGetRight(box1), utBoxGetRight(box2)));
    utBoxSetTop(box1, utMax(utBoxGetTop(box1), utBoxGetTop(box2)));
    return box1;
}

/*--------------------------------------------------------------------------------------------------
  Find the intersection of two boxes.
--------------------------------------------------------------------------------------------------*/
utBox utBoxIntersection(
    utBox box1,
    utBox box2)
{
    utBoxSetLeft(box1, utMax(utBoxGetLeft(box1), utBoxGetLeft(box2)));
    utBoxSetBottom(box1, utMax(utBoxGetBottom(box1), utBoxGetBottom(box2)));
    utBoxSetRight(box1, utMin(utBoxGetRight(box1), utBoxGetRight(box2)));
    utBoxSetTop(box1, utMin(utBoxGetTop(box1), utBoxGetTop(box2)));
    return box1;
}

/*--------------------------------------------------------------------------------------------------
  Determine if two boxes intersect.  Touching counts.
  Macro version is faster.
--------------------------------------------------------------------------------------------------*/
#if 0
bool utBoxesIntersect(
    utBox box1,
    utBox box2)
{
    if(utBoxGetRight(box1) < utBoxGetLeft(box2) ||
        utBoxGetLeft(box1) > utBoxGetRight(box2)) {
        return false;
    }
    if(utBoxGetTop(box1) < utBoxGetBottom(box2) ||
        utBoxGetBottom(box1) > utBoxGetTop(box2)) {
        return false;
    }
    return true;
}
#endif

/*--------------------------------------------------------------------------------------------------
  Determine if two boxes overlap.  Touching does not count.
  Macro version is faster.
--------------------------------------------------------------------------------------------------*/
#if 0
bool utBoxesOverlap(
    utBox box1,
    utBox box2)
{
    if(utBoxGetRight(box1) <= utBoxGetLeft(box2) ||
        utBoxGetLeft(box1) >= utBoxGetRight(box2)) {
        return false;
    }
    if(utBoxGetTop(box1) <= utBoxGetBottom(box2) ||
        utBoxGetBottom(box1) >= utBoxGetTop(box2)) {
        return false;
    }
    return true;
}
#endif

/*--------------------------------------------------------------------------------------------------
  Determine if the box has no area. Single points are considered empty.
--------------------------------------------------------------------------------------------------*/
bool utBoxEmpty(
    utBox box)
{
    return utBoxGetLeft(box) >= utBoxGetRight(box) ||
        utBoxGetBottom(box) >= utBoxGetTop(box);
}

/*--------------------------------------------------------------------------------------------------
  Expand the box to include the point.
--------------------------------------------------------------------------------------------------*/
utBox utExpandBox(
    utBox box,
    int32 x,
    int32 y)
{
    if(x < utBoxGetLeft(box)) {
        utBoxSetLeft(box, x);
    }
    if(y < utBoxGetBottom(box)) {
        utBoxSetBottom(box, y);
    }
    if(x > utBoxGetRight(box)) {
        utBoxSetRight(box, x);
    }
    if(y > utBoxGetTop(box)) {
        utBoxSetTop(box, y);
    }
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Enlarge the box by the same amount in all dimensions.
  Note that we can make boxes smaller by passing a negative value.
--------------------------------------------------------------------------------------------------*/
utBox utEnlargeBox(
    utBox box,
    int32 value)
{
    utBoxSetLeft(box, utBoxGetLeft(box) - value);
    utBoxSetBottom(box, utBoxGetBottom(box) - value);
    utBoxSetRight(box, utBoxGetRight(box) + value);
    utBoxSetTop(box, utBoxGetTop(box) + value);
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Move the box.
--------------------------------------------------------------------------------------------------*/
utBox utMoveBox(
    utBox box,
    int32 deltaX,
    int32 deltaY)
{
    utBoxSetLeft(box, utBoxGetLeft(box) + deltaX);
    utBoxSetBottom(box, utBoxGetBottom(box) + deltaY);
    utBoxSetRight(box, utBoxGetRight(box) + deltaX);
    utBoxSetTop(box, utBoxGetTop(box) + deltaY);
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the box contains or touches the point.
--------------------------------------------------------------------------------------------------*/
bool utBoxContainsPoint(
    utBox box,
    int32 x,
    int32 y)
{
    return utBoxGetLeft(box) <= x && x <= utBoxGetRight(box) &&
        utBoxGetBottom(box) <= y && y <= utBoxGetTop(box);
}

/*--------------------------------------------------------------------------------------------------
  Determine if box1 completely encloses box2. Edges may be shared.
--------------------------------------------------------------------------------------------------*/
bool utBoxContainsBox(
    utBox box1,
    utBox box2)
{
    if(utBoxGetLeft(box2) < utBoxGetLeft(box1)) {
        return false;
    }
    if(utBoxGetRight(box2) > utBoxGetRight(box1)) {
        return false;
    }
    if(utBoxGetBottom(box2) < utBoxGetBottom(box1)) {
        return false;
    }
    if(utBoxGetTop(box2) > utBoxGetTop(box1)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Make a new point value.
--------------------------------------------------------------------------------------------------*/
utPoint utMakePoint(
    int32 x,
    int32 y)
{
    utPoint point;

    utPointSetX(point, x);
    utPointSetY(point, y);
    return point;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the two points are equal.
--------------------------------------------------------------------------------------------------*/
bool utPointsEqual(
    utPoint point1,
    utPoint point2)
{
    if(utPointGetX(point1) == utPointGetX(point2) &&
        utPointGetY(point1) == utPointGetY(point2)) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Compare the boxes to order them in a relation.
--------------------------------------------------------------------------------------------------*/
int utCompareBoxes(
    utBox box1,
    utBox box2)
{
    if(utBoxGetBottom(box1) != utBoxGetBottom(box2)) {
        return utBoxGetBottom(box1) - utBoxGetBottom(box2);
    }
    if(utBoxGetLeft(box1) != utBoxGetLeft(box2)) {
        return utBoxGetLeft(box1) - utBoxGetLeft(box2);
    }
    if(utBoxGetRight(box1) != utBoxGetRight(box2)) {
        return utBoxGetRight(box1) - utBoxGetRight(box2);
    }
    return utBoxGetTop(box1) - utBoxGetTop(box2);
}

/*--------------------------------------------------------------------------------------------------
  Find the distance between a box and the point.
// Could be optimized with a good integer absolute value function. At
// a minimum, we could use: (2 fewer comparisons, 1 less addition of zero)
//    if(utBoxGetLeft(box) > x) {
//        distx = utBoxGetLeft(box) - x;
//    } else {
//        distx = x - utBoxGetRight(box);
//    }
//    if(utBoxGetBottom(box) > y) {
//        disty = utBoxGetBottom(box) - y;
//    } else {
//        disty = y - utBoxGetTop(box);
//    }
//    return distx + disty;
--------------------------------------------------------------------------------------------------*/
uint32 utDistanceBetweenBoxAndPoint(
   utBox box,
   int32 x,
   int32 y)
{
    uint32 dist = 0;

    if(utBoxGetLeft(box) > x) {
        dist += utBoxGetLeft(box) - x;
    } else if(utBoxGetRight(box) < x) {
        dist += x - utBoxGetRight(box);
    }
    if(utBoxGetBottom(box) > y) {
        dist += utBoxGetBottom(box) - y;
    } else if(utBoxGetTop(box) < y) {
        dist += y - utBoxGetTop(box);
    }
    return dist;
}

/*--------------------------------------------------------------------------------------------------
  Find the width of a box.
--------------------------------------------------------------------------------------------------*/
uint32 utBoxGetWidth(
    utBox box)
{
    utAssert(utBoxGetRight(box) >= utBoxGetLeft(box));
    return utBoxGetRight(box) - utBoxGetLeft(box);
}

/*--------------------------------------------------------------------------------------------------
  Find the center coordinate of a box. Can't overflow like (l+r)/2 can.
--------------------------------------------------------------------------------------------------*/
int32 utBoxGetCenterX(
    utBox box)
{
    return ((utBoxGetRight(box) - utBoxGetLeft(box)) >> 1) + utBoxGetLeft(box);
}

/*--------------------------------------------------------------------------------------------------
  Find the height of a box.
--------------------------------------------------------------------------------------------------*/
uint32 utBoxGetHeight(
    utBox box)
{
    utAssert(utBoxGetTop(box) >= utBoxGetBottom(box));
    return utBoxGetTop(box) - utBoxGetBottom(box);
}

/*--------------------------------------------------------------------------------------------------
  Find the center coordinate of a box. Can't overflow like (t+b)/2 can.
--------------------------------------------------------------------------------------------------*/
int32 utBoxGetCenterY(
    utBox box)
{
    return ((utBoxGetTop(box) - utBoxGetBottom(box)) >> 1) + utBoxGetBottom(box);
}

/*--------------------------------------------------------------------------------------------------
  Find the largest dimension (edge) of a box.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxLargestDimension(
    utBox box)
{
    return utMax(utBoxGetWidth(box), utBoxGetHeight(box));
}

/*--------------------------------------------------------------------------------------------------
  Find the smallest dimension (edge) of a box.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxSmallestDimension(
    utBox box)
{
    return utMin(utBoxGetWidth(box), utBoxGetHeight(box));
}

/*--------------------------------------------------------------------------------------------------
  Convert a box into a rectalinear line segment (represented as a 0-width box) and a width.
--------------------------------------------------------------------------------------------------*/
void utConvertBoxToLineAndWidth(
    utBox box,
    utBox * retLine,
    uint32 * retWidth)
{
    uint32 height = utBoxGetHeight(box);
    uint32 width = utBoxGetWidth(box);

    if(height > width) {
        *retWidth = width;
    } else {
        *retWidth = height;
    }
    utAssert(!(*retWidth & 1)); /* must be even */
    *retLine = utEnlargeBox(box, -(int32)(*retWidth >> 1));
}    

/*--------------------------------------------------------------------------------------------------
  Translate the box's coordinates as follows:
    1. reflect about the x-axis if reflect is set
    2. rotate about origin counter clockwise rotation*90 degrees
    3. move the box by the x and y offsets
--------------------------------------------------------------------------------------------------*/
utBox utTranslateBox(
    utBox box,
    utTranslation translation)
{
    int32 temp;
    uint8 rotation = 0x3 & utTranslationGetRotation(translation);
    int32 xOffset = utTranslationGetX(translation);
    int32 yOffset = utTranslationGetY(translation);

    if(utTranslationReflect(translation)) {
        temp = utBoxGetBottom(box);
        utBoxSetBottom(box, -utBoxGetTop(box));
        utBoxSetTop(box, -temp);
    }
    while(rotation > 0) {
        box = utMakeBox(-utBoxGetTop(box), utBoxGetLeft(box), -utBoxGetBottom(box), utBoxGetRight(box));
        rotation--;
    }
    utBoxSetLeft(box, utBoxGetLeft(box) + xOffset);
    utBoxSetBottom(box, utBoxGetBottom(box) + yOffset);
    utBoxSetRight(box, utBoxGetRight(box) + xOffset);
    utBoxSetTop(box, utBoxGetTop(box) + yOffset);
    return box;
}

/*--------------------------------------------------------------------------------------------------
  Translate the point's coordinates as follows:
    1. reflect about the x-axis if reflect is set
    2. rotate about origin counter clockwise rotation*90 degrees
    3. move the point by the x and y offsets
--------------------------------------------------------------------------------------------------*/
utPoint utTranslatePoint(
    utPoint point,
    utTranslation translation)
{
    uint8 rotation = 0x3 & utTranslationGetRotation(translation);
    int32 xOffset = utTranslationGetX(translation);
    int32 yOffset = utTranslationGetY(translation);

    if(utTranslationReflect(translation)) {
        utPointSetY(point, -utPointGetY(point));
    }
    while(rotation > 0) {
        point = utMakePoint(-utPointGetY(point), utPointGetX(point));
        rotation--;
    }
    utPointSetX(point, utPointGetX(point) + xOffset);
    utPointSetY(point, utPointGetY(point) + yOffset);
    return point;
}

/*--------------------------------------------------------------------------------------------------
  Return a translation structure.
--------------------------------------------------------------------------------------------------*/
utTranslation utMakeTranslation(
    bool reflect,
    uint8 rotation,
    int32 x,
    int32 y)
{
    utTranslation translation;

    utTranslationGetX(translation) = x;
    utTranslationGetY(translation) = y;
    utTranslationGetRotation(translation) = rotation;
    utTranslationReflect(translation) = reflect;
    return translation;
}

/*--------------------------------------------------------------------------------------------------
  Combine two translations.
--------------------------------------------------------------------------------------------------*/
utTranslation utCombineTranslations(
    utTranslation first,
    utTranslation second)
{
    utPoint point = utTranslatePoint(utMakePoint(utTranslationGetX(first), utTranslationGetY(first)),
        second);
    bool reflect;
    uint8 rotation;

    if(utTranslationReflect(second)) {
        reflect = !utTranslationReflect(first);
        rotation = (utTranslationGetRotation(second) - utTranslationGetRotation(first)) & 0x3;
    } else {
        reflect = utTranslationReflect(first);
        rotation = (utTranslationGetRotation(second) + utTranslationGetRotation(first)) & 0x3;
    }
    return utMakeTranslation(reflect, rotation, utPointGetX(point), utPointGetY(point));
}

/*--------------------------------------------------------------------------------------------------
  Return a translation that would undo this one.
--------------------------------------------------------------------------------------------------*/
utTranslation utInvertTranslation(
    utTranslation translation)
{
    utPoint point;

    if(!utTranslationReflect(translation)) {
        utTranslationGetRotation(translation) = -utTranslationGetRotation(translation);
    }
    /* Find out where origin got moved to so we can correct it */
    point = utTranslatePoint(utMakePoint(utTranslationGetX(translation),
        utTranslationGetY(translation)), translation);
    utTranslationGetX(translation) -= utPointGetX(point);
    utTranslationGetY(translation) -= utPointGetY(point);
    return translation;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the two translations are the same.
--------------------------------------------------------------------------------------------------*/
bool utTranslationsEqual(
    utTranslation translation1,
    utTranslation translation2)
{
    if(utTranslationGetRotation(translation1) != utTranslationGetRotation(translation2)) {
        return false;
    }
    if(utTranslationReflect(translation1) != utTranslationReflect(translation2)) {
        return false;
    }
    if(utTranslationGetX(translation1) != utTranslationGetX(translation2)) {
        return false;
    }
    if(utTranslationGetY(translation1) != utTranslationGetY(translation2)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Return true if the box is a horizontal line or a point.
--------------------------------------------------------------------------------------------------*/
bool utBoxIsHorLine(
    utBox box)
{
    return (utBoxGetWidth(box) == 0);
}

/*--------------------------------------------------------------------------------------------------
  Return true if the box is a vertical line or a point.
--------------------------------------------------------------------------------------------------*/
bool utBoxIsVertLine(
    utBox box)
{
    return (utBoxGetHeight(box) == 0);
}


/*--------------------------------------------------------------------------------------------------
  Return true if the box is a line or a point.
--------------------------------------------------------------------------------------------------*/
bool utBoxIsLine(
    utBox box)
{
    return (utBoxIsVertLine(box) || utBoxIsHorLine(box));
}

/*--------------------------------------------------------------------------------------------------
  Return true if the box is a single point;
--------------------------------------------------------------------------------------------------*/
bool utBoxIsPoint(
    utBox box)
{
    return (utBoxIsVertLine(box) && utBoxIsHorLine(box));
}

/*--------------------------------------------------------------------------------------------------
  Return the vertical distance between two boxes.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxVertDist(
    utBox a,
    utBox b)
{
    int32 temp = utBoxGetBottom(a) - utBoxGetTop(b);

    if(temp > 0) {
        return temp;
    }
    temp = utBoxGetBottom(b) - utBoxGetTop(a);
    if(temp > 0) {
        return temp;
    }
    return 0;
}

/*--------------------------------------------------------------------------------------------------
  Return the horizontal distance between two boxes.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxHorDist(
    utBox a,
    utBox b)
{
    int32 temp = utBoxGetLeft(a) - utBoxGetRight(b);

    if(temp > 0) {
        return temp;
    }
    temp = utBoxGetLeft(b) - utBoxGetRight(a);
    if(temp > 0) {
        return temp;
    }
    return 0;
}

/*--------------------------------------------------------------------------------------------------
  Return the Manhatten distance between two boxes.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxDist(
    utBox a,
    utBox b)
{
    return utFindBoxHorDist(a, b) + utFindBoxVertDist(a, b);
}

/*--------------------------------------------------------------------------------------------------
  Return true if the box is at least as wide as it is tall.
  NOTE:  a box can be horizontal AND vertical if it's square.
--------------------------------------------------------------------------------------------------*/
bool utBoxIsHorizontal(
    utBox box)
{
    return utBoxGetWidth(box) >= utBoxGetHeight(box);
}

/*--------------------------------------------------------------------------------------------------
  Return true if a box is at least as tall as it is wide.  
  NOTE:  a box can be horizontal AND vertical if it's square.
--------------------------------------------------------------------------------------------------*/
bool utBoxIsVertical(
    utBox box)
{
    return utBoxGetHeight(box) >= utBoxGetWidth(box);
}

/*--------------------------------------------------------------------------------------------------
  Return true if one box is horizontal and the other is vertical.
  Squares are always perpendicular to everything.
--------------------------------------------------------------------------------------------------*/
bool utBoxesPerpendicular(
    utBox a,
    utBox b)
{
    if(utBoxIsHorizontal(a) && utBoxIsHorizontal(b)) {
        return false;
    }
    if(utBoxIsVertical(a) && utBoxIsVertical(b)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Return true if the box is valid.
--------------------------------------------------------------------------------------------------*/
bool utBoxValid(
    utBox box)
{
    if(utBoxGetLeft(box) > utBoxGetRight(box)) {
        return false;
    }
    if(utBoxGetBottom(box) > utBoxGetTop(box)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Return true if small is inside of large; it can share edges.
--------------------------------------------------------------------------------------------------*/
bool utBoxCoveredByBox(
    utBox small,
    utBox big)
{
    if(utBoxGetLeft(big) <= utBoxGetLeft(small) && utBoxGetLeft(small) <= utBoxGetRight(big) &&
        utBoxGetLeft(big) <= utBoxGetRight(small) && utBoxGetRight(small) <= utBoxGetRight(big) &&
        utBoxGetBottom(big) <= utBoxGetBottom(small) && utBoxGetBottom(small) <= utBoxGetTop(big) &&
        utBoxGetBottom(big) <= utBoxGetTop(small) && utBoxGetTop(small) <= utBoxGetTop(big)) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Find a point on box a that is nearest to box b.
--------------------------------------------------------------------------------------------------*/
utBox utFindBoxOnBoxNearestBox(
    utBox a,
    utBox b)
{
    utBox retval;
    int32 left = utBoxGetLeft(b);
    int32 bottom = utBoxGetBottom(b);
    int32 right = utBoxGetRight(b);
    int32 top = utBoxGetTop(b);

    left = utMax(left, utBoxGetLeft(a));
    left = utMin(left, utBoxGetRight(a));
    bottom = utMax(bottom, utBoxGetBottom(a));
    bottom = utMin(bottom, utBoxGetTop(a));
    right = utMax(right, utBoxGetLeft(a));
    right = utMin(right, utBoxGetRight(a));
    top = utMax(top, utBoxGetBottom(a));
    top = utMin(top, utBoxGetTop(a));
    retval = utMakeBox(left, bottom, right, top);
    return retval;
}

/*--------------------------------------------------------------------------------------------------
  Return true if the boxes are equal.
--------------------------------------------------------------------------------------------------*/
bool utBoxesEqual(
    utBox a,
    utBox b)
{
    return (utBoxGetLeft(a) == utBoxGetLeft(b)) &&
       (utBoxGetBottom(a) == utBoxGetBottom(b)) &&
       (utBoxGetRight(a) == utBoxGetRight(b)) &&
       (utBoxGetTop(a) == utBoxGetTop(b));
}

/*--------------------------------------------------------------------------------------------------
  Return true if one box crosses completely over the other box.
--------------------------------------------------------------------------------------------------*/
bool utBoxCrossesBox(
    utBox a,
    utBox b)
{
    if(!utBoxesIntersect(a, b)) {
        return false;
    }
    if((utBoxGetLeft(a) < utBoxGetLeft(b)) && (utBoxGetRight(a) > utBoxGetRight(b))) {
        return true;
    }
    if((utBoxGetBottom(a) < utBoxGetBottom(b)) && (utBoxGetTop(a) > utBoxGetTop(b))) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Return true if one box crosses completely over the other box in the given direction.
--------------------------------------------------------------------------------------------------*/
bool utBoxCrossesBoxInDirection(
    utBox a,
    utBox b,
    bool horizontal)
{
    if(!utBoxesIntersect(a, b)) {
        return false;
    }
    if(horizontal) {
        return (utBoxGetLeft(a) < utBoxGetLeft(b)) && (utBoxGetRight(a) > utBoxGetRight(b));
    } /* else */
    return (utBoxGetBottom(a) < utBoxGetBottom(b)) && (utBoxGetTop(a) > utBoxGetTop(b));
}

/*--------------------------------------------------------------------------------------------------
  Return true if the line runs across the box cutting it into two smaller
  boxes.  If the line is not entirely enclosed by the box or if it runs
  along the edge return false;
--------------------------------------------------------------------------------------------------*/
bool utLineDividesBox(
    utBox line,
    utBox box)
{
    utBox lineMinPoint = utMakeBoxPoint(utBoxGetLeft(line), utBoxGetBottom(line));
    utBox lineMaxPoint = utMakeBoxPoint(utBoxGetRight(line), utBoxGetTop(line));
    utBox boxMinPoint = utMakeBoxPoint(utBoxGetLeft(box), utBoxGetBottom(box));
    utBox boxMaxPoint = utMakeBoxPoint(utBoxGetRight(box), utBoxGetTop(box));

    if(!utBoxIsLine(line)) {
        return false;
    }
    if(!utBoxContainsBox(box, line)) {
        return false;
    }
    if(utBoxesEqual(lineMinPoint, boxMinPoint)) {
        return false;
    }
    if(utBoxesEqual(lineMaxPoint, boxMaxPoint)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Make a box that represents the x, y coordinate.
--------------------------------------------------------------------------------------------------*/
utBox utMakeBoxPoint(
    int32 x,
    int32 y)
{
    return utMakeBox(x, y, x, y);
}

/*--------------------------------------------------------------------------------------------------
  Return the middle of the box rounded to the left and bottom.
--------------------------------------------------------------------------------------------------*/
uint32 utFindPointBoxEdgeDist(
    utBox point,
    utBox box)
{
    utBox edge = utMakeBox(utBoxGetLeft(box), utBoxGetTop(box), utBoxGetRight(box), utBoxGetTop(box));
    uint32 minDist = utFindBoxDist(edge, point);
    uint32 dist;

    utAssert(utBoxIsPoint(point));
    edge = utMakeBox(utBoxGetLeft(box), utBoxGetBottom(box), utBoxGetRight(box), utBoxGetBottom(box));
    dist = utFindBoxDist(edge, point);
    if(dist < minDist) {
        minDist = dist;
    }
    edge = utMakeBox(utBoxGetLeft(box), utBoxGetBottom(box), utBoxGetLeft(box), utBoxGetTop(box));
    dist = utFindBoxDist(edge, point);
    if(dist < minDist) {
        minDist = dist;
    }
    edge = utMakeBox(utBoxGetRight(box), utBoxGetBottom(box), utBoxGetRight(box), utBoxGetTop(box));
    dist = utFindBoxDist(edge, point);
    if(dist < minDist) {
        minDist = dist;
    }
    return minDist;
}

/*--------------------------------------------------------------------------------------------------
  Return the middle of the box rounded to the left and bottom.
--------------------------------------------------------------------------------------------------*/
#if 0
//  WARNING: can overflow. See utBoxGetCenterX for a better calculation.
utBox utBoxGetMiddle(
    utBox box)
{
    int32 x = (utBoxGetLeft(box) + utBoxGetRight(box)) >> 1;
    int32 y = (utBoxGetBottom(box) + utBoxGetTop(box)) >> 1;
    utBox middle = utMakeBoxPoint(x, y);

    return middle;
}
#endif

/*--------------------------------------------------------------------------------------------------
  Make a new line from two points.
--------------------------------------------------------------------------------------------------*/
utLine utMakeLineFromPoints(
    utPoint start,
    utPoint stop)
{
    utLine line;

    line.start = start;
    line.stop = stop;
    return line;
}

/*--------------------------------------------------------------------------------------------------
  Make a line from x and y coordinates.
--------------------------------------------------------------------------------------------------*/
utLine utMakeLine(
    int32 x1,
    int32 y1,
    int32 x2,
    int32 y2)
{
    utLine line;

    line.start.x = x1;
    line.start.y = y1;
    line.stop.x = x2;
    line.stop.y = y2;
    return line;
}

/*--------------------------------------------------------------------------------------------------
  Find the bounding box of a line.
--------------------------------------------------------------------------------------------------*/
utBox utFindLineBox(
    utLine line)
{
    return utCorrectBox(utMakeBox(utLineGetX1(line), utLineGetY1(line),
        utLineGetX2(line), utLineGetY2(line)));
}

/*--------------------------------------------------------------------------------------------------
  Find the line's change in X, which can be positive or negative.
--------------------------------------------------------------------------------------------------*/
uint32 utLineGetDX(
    utLine line)
{
    return utLineGetX2(line) - utLineGetX1(line);
}

/*--------------------------------------------------------------------------------------------------
  Find the line's change in Y, which can be positive or negative.
--------------------------------------------------------------------------------------------------*/
uint32 utLineGetDY(
    utLine line)
{
    return utLineGetY2(line) - utLineGetY1(line);
}

/*--------------------------------------------------------------------------------------------------
  Move the line by the ammount.
--------------------------------------------------------------------------------------------------*/
utLine utMoveLine(
    utLine line,
    int32 deltaX,
    int32 deltaY)
{
    utLineSetX1(line, utLineGetX1(line) + deltaX);
    utLineSetY1(line, utLineGetY1(line) + deltaY);
    utLineSetX2(line, utLineGetX2(line) + deltaX);
    utLineSetY2(line, utLineGetY2(line) + deltaY);
    return line;
}

/*--------------------------------------------------------------------------------------------------
  Translate the position of the line.
--------------------------------------------------------------------------------------------------*/
utLine utTranslateLine(
    utLine line,
    utTranslation translation)
{
    int32 temp;
    uint8 rotation = 0x3 & utTranslationGetRotation(translation);
    int32 xOffset = utTranslationGetX(translation);
    int32 yOffset = utTranslationGetY(translation);

    if(utTranslationReflect(translation)) {
        temp = utLineGetY1(line);
        utLineSetY1(line, -utLineGetY2(line));
        utLineSetY2(line, -temp);
    }
    while(rotation > 0) {
        line = utMakeLine(-utLineGetY2(line), utLineGetX1(line),
            -utLineGetY1(line), utLineGetX2(line));
        rotation--;
    }
    utLineSetX1(line, utLineGetX1(line) + xOffset);
    utLineSetY1(line, utLineGetY1(line) + yOffset);
    utLineSetX2(line, utLineGetX2(line) + xOffset);
    utLineSetY2(line, utLineGetY2(line) + yOffset);
    return line;
}

/*--------------------------------------------------------------------------------------------------
  Determine if a line is horizontal.
--------------------------------------------------------------------------------------------------*/
bool utLineHorizontal(
    utLine a)
{
    return (utLineGetY1(a) == utLineGetY2(a));
}

/*--------------------------------------------------------------------------------------------------
  Determine if a line is vertical.
--------------------------------------------------------------------------------------------------*/
bool utLineVertical(
    utLine a)
{
    return (utLineGetX1(a) == utLineGetX2(a));
}

/*--------------------------------------------------------------------------------------------------
  Determine if a line is rectalinear.
--------------------------------------------------------------------------------------------------*/
bool utLineRectalinear(
    utLine a)
{
    return utLineVertical(a) || utLineHorizontal(a);
}

/*--------------------------------------------------------------------------------------------------
  Determine if two lines are equal.
--------------------------------------------------------------------------------------------------*/
bool utLinesEqual(
    utLine a,
    utLine b)
{
    return utLineGetX1(a) == utLineGetX1(b) && utLineGetY1(a) == utLineGetY1(b) &&
        utLineGetX2(a) == utLineGetX2(b) && utLineGetY2(a) == utLineGetY2(b);
}

/*--------------------------------------------------------------------------------------------------
  Determine if two lines intersect or overlap. Rectalinear lines only!
--------------------------------------------------------------------------------------------------*/
bool utLinesIntersect(
    utLine a,
    utLine b)
{
    utAssert(utLineRectalinear(a) && utLineRectalinear(b)); 
    return utBoxesIntersect(utFindLineBox(a), utFindLineBox(b));
}

/*--------------------------------------------------------------------------------------------------
  Find the area of a box.
--------------------------------------------------------------------------------------------------*/
int64 utFindBoxAreaint64(
    utBox box)
{
    if(utBoxEmpty(box)) {
        return 0;
    }
    return ((int64)utBoxGetWidth(box))*((int64)utBoxGetHeight(box));
}

/*--------------------------------------------------------------------------------------------------
  Find the area of a box. Can overflow since only a uint32 is returned.
--------------------------------------------------------------------------------------------------*/
uint32 utFindBoxAreauint32(
    utBox box)
{
    return utBoxEmpty(box) ? 0 : (utBoxGetWidth(box) * utBoxGetHeight(box));
}
