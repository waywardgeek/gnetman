/*
 * Copyright (C) 2008 Triade MDG Inc.
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

#include <assert.h>
#include "rdatabase.h"

extern void rShowBox(rBox a);
extern void rShowShape(rShape a);
extern void rShowSpace(rSpace a);
extern void rShowNode(rNode a);
extern void rShowGroup(rGroup a);
extern void rShowVertex(rVertex a);

static void rVertexReevaluate(rVertex vertex);
static void rSpaceInsertShape(rSpace space, rShape shape);
static void rNodeReevaluateBboxAfterDeletion(rNode node);


static void signalShapeDestruction(rShape shape);


void rStart()
{
    rDatabaseStart();
    rShapeSetDestructorCallback(signalShapeDestruction);
}

void rStop()
{
    rDatabaseStop();
}

/*----------------- Box -------------------------*/

static rBox rBoxCreate(
    int32 left,
    int32 bottom,
    int32 right,
    int32 top)
{
    rBox retval = rBoxAlloc();
    rBoxSetTop(retval, utMax(top,bottom));
    rBoxSetBottom(retval, utMin(top,bottom));
    rBoxSetLeft(retval, utMin(left, right));
    rBoxSetRight(retval, utMax(right,left));
    rBoxSetEmpty(retval, false);
    return retval;
}

static void rBoxSet(
    rBox   box,
    int32 left,
    int32 bottom,
    int32 right,
    int32 top)
{
    rBoxSetTop(box, utMax(top,bottom));
    rBoxSetBottom(box, utMin(top,bottom));
    rBoxSetLeft(box, utMin(left, right));
    rBoxSetRight(box, utMax(right,left));
    rBoxSetEmpty(box, false);
}


static rBox rBoxCreateEmpty(void)
{
    rBox retval = rBoxAlloc();
    rBoxSetEmpty(retval, true);
    return retval;
}


static uint32 rBoxCloseness(
    rBox a,
    rBox b)
{
    int32 dx,dy,sx,sy,wxa,wya,wxb,wyb;

    if(rBoxEmpty(a) || rBoxEmpty(b)) {
        return INT32_MAX;
    }
    
    /* distance */
    dy = utAbs(rBoxGetTop(a) - rBoxGetTop(b) - rBoxGetBottom(a) + rBoxGetBottom(b));
    dx = utAbs(rBoxGetLeft(a) - rBoxGetLeft(b) - rBoxGetRight(a) + rBoxGetRight(b));

    /* spacing (overlapping) */
    sx = utMax(0, utMax(rBoxGetLeft(a)-rBoxGetRight(b),rBoxGetLeft(b)-rBoxGetRight(a)));
    sy = utMax(0, utMax(rBoxGetBottom(a)-rBoxGetTop(b),rBoxGetBottom(b)-rBoxGetTop(a)));

    /* width */
    wxa = utAbs(rBoxGetTop(a)-rBoxGetBottom(a));
    wya = utAbs(rBoxGetRight(a)-rBoxGetLeft(a));

    wxb = utAbs(rBoxGetTop(b)-rBoxGetBottom(b));
    wyb = utAbs(rBoxGetRight(b)-rBoxGetLeft(b));

    /* TODO: put factors from rSpace, all defaulted to '1' to get the following
       behavior */
    return (dx*dx+dy*dy) + (sx*sx+sy*sy) + (wxa*wxa+wya*wya) + (wxb*wxb+wyb*wyb);
}

static bool rBoxWithin(
    rBox outside,
    rBox inside)
{
    assert(!rBoxEmpty(inside));

    if(rBoxEmpty(outside) || rBoxEmpty(inside)) {
        return false;
    }

    return rBoxGetTop(outside) >= rBoxGetTop(inside)
        && rBoxGetBottom(outside) <= rBoxGetBottom(inside)
        && rBoxGetLeft(outside) <= rBoxGetLeft(inside)
        && rBoxGetRight(outside) >= rBoxGetRight(inside);
}

static bool rBoxTouching(
    rBox a,
    rBox b)
{
    /* an empty (null) box is touching no others */
    if(rBoxEmpty(a) || rBoxEmpty(b)) {
        return false;
    }

    return rBoxGetTop(a) >= rBoxGetBottom(b) &&
           rBoxGetBottom(a) <= rBoxGetTop(b) &&
           rBoxGetLeft(a) <= rBoxGetRight(b) &&
           rBoxGetRight(a) >= rBoxGetLeft(b);
}


static rBox rBoxClone(
    rBox a)
{
    if(rBoxEmpty(a)) {
        return rBoxCreateEmpty();
    } else {
        return rBoxCreate(rBoxGetLeft(a), rBoxGetBottom(a), rBoxGetRight(a), rBoxGetTop(a));
    }
}

static void rBoxCopy(
    rBox src,
    rBox dest)
{
    rBoxSetTop(dest, rBoxGetTop(src));
    rBoxSetBottom(dest, rBoxGetBottom(src));
    rBoxSetLeft(dest, rBoxGetLeft(src));
    rBoxSetRight(dest, rBoxGetRight(src));
    rBoxSetEmpty(dest, rBoxEmpty(src));
}

static void rBoxBounding(
    rBox a,
    rBox b)
{
    if(rBoxEmpty(b)) {
        /* do nothing */
    } else if ( rBoxEmpty(a) ) {
        rBoxSetTop(a, rBoxGetTop(b));
        rBoxSetBottom(a, rBoxGetBottom(b));
        rBoxSetLeft(a, rBoxGetLeft(b));
        rBoxSetRight(a, rBoxGetRight(b));
        rBoxSetEmpty(a, false);
    } else {
        rBoxSetTop(a, utMax(rBoxGetTop(a), rBoxGetTop(b)));
        rBoxSetBottom(a, utMin(rBoxGetBottom(a), rBoxGetBottom(b)));
        rBoxSetLeft(a, utMin(rBoxGetLeft(a), rBoxGetLeft(b)));
        rBoxSetRight(a, utMax(rBoxGetRight(a), rBoxGetRight(b)));
    }
}

static void rBoxBoundingXY(
    rBox a,
    int32 x,
    int32 y)
{
    if(rBoxEmpty(a)) {
        rBoxSetTop(a, y);
        rBoxSetBottom(a, y);
        rBoxSetLeft(a, x);
        rBoxSetRight(a, x);
        rBoxSetEmpty(a, false);
    } else {
        rBoxSetTop(a, utMax(rBoxGetTop(a), y));
        rBoxSetBottom(a, utMin(rBoxGetBottom(a), y));
        rBoxSetLeft(a, utMin(rBoxGetLeft(a), x));
        rBoxSetRight(a, utMax(rBoxGetRight(a), x));
    }
}

static void rBoxBoundingShape(
    rBox a,
    rShape b)
{
    rBox bbox = rShapeGetBbox(b);
    if(bbox != rBoxNull) {
        rBoxBounding(a, bbox);
    }
}

static void rBoxBoundingNode(
    rBox a,
    rNode b)
{
    rBox bbox = rNodeGetBbox(b);
    if(bbox != rBoxNull) {
        rBoxBounding(a, bbox);
    }
}

static bool rBoxEq(
    rBox a,
    rBox b)
{
    return
        rBoxGetLeft(a) == rBoxGetLeft(b) &&
        rBoxGetBottom(a) == rBoxGetBottom(b) &&
        rBoxGetRight(a) == rBoxGetRight(b) &&
        rBoxGetTop(a) == rBoxGetTop(b);

}


static void rBoxFillRandom(
    rBox a)
{
    int32 x, y, w, h;
    x = utRandN(1000);
    y = utRandN(1000);
    w = utRandN(100);
    h = utRandN(100);
    rBoxSetLeft(a, x);
    rBoxSetBottom(a, y);
    rBoxSetRight(a, x+w);
    rBoxSetTop(a, y+h);
    rBoxSetEmpty(a, false);
}

/*----------------- Shape -------------------------*/

rShape rShapeCreate(
    rSpace space,
    int32 left,
    int32 bottom,
    int32 right,
    int32 top)
{
    rShape retval = rShapeAlloc();
    rSpaceAppendContainingShape(space, retval);
    rShapeSetBbox(retval, rBoxCreate(left, bottom, right, top));
    rSpaceInsertShape(space, retval);
    return retval;
}


void rShapeSetBoundingBox(
    rShape shape,
    int32 left,
    int32 bottom,
    int32 right,
    int32 top)
{
    rBox box = rShapeGetBbox(shape);
    rNode node = rShapeGetNode(shape);
    rSpace space = rShapeGetSpace(shape);

    if(rBoxGetLeft(box) != left &&
        rBoxGetBottom(box) != bottom &&
        rBoxGetRight(box) != right &&
        rBoxGetTop(box) != top) {

        rNodeRemoveShape(node, shape);
        rNodeReevaluateBboxAfterDeletion(node);
        rBoxSet(box, left, bottom, right, top);
        rSpaceInsertShape(space, shape);
    }
}

/*----------------- Group -------------------------*/

static rGroup rGroupCreate(rPartition partition, rNode first)
{
    rGroup retval = rGroupAlloc();
    rPartitionAppendGroup(partition, retval);
    rGroupAppendNode(retval, first);
    rGroupSetBbox(retval, rBoxClone(rNodeGetBbox(first)));
    return retval;
}

static bool rGroupContainsOneNode(rGroup group)
{
    int count = 0;
    rNode node;
    rForeachGroupNode(group, node) {
        count++;
        if(count > 1) {
            return false;
        }
    } rEndGroupNode;
    return count == 1;
}

static void rGroupMerge(rGroup dest, rGroup todel)
{
    rBox bb = rGroupGetBbox(dest);
    rNode node;
    rVertex vertex;

    rSafeForeachGroupNode(todel, node) {

        rBoxBounding(bb, rNodeGetBbox(node));

        rGroupRemoveNode(todel, node);
        rGroupAppendNode(dest, node);

    } rEndSafeGroupNode;

    rGroupDestroy(todel);

    rForeachGroupChild1Vertex(dest, vertex) {

        rVertexReevaluate(vertex);

    } rEndGroupChild1Vertex;

    rForeachGroupChild2Vertex(dest, vertex) {

        rVertexReevaluate(vertex);

    } rEndGroupChild2Vertex;



}


/*----------------- Vertex -------------------------*/

static rVertex rVertexCreate(rPartition partition, rGroup group1, rGroup group2)
{
    rVertex vertex = rVertexAlloc();
    rVertexSetGroup1Group(vertex, group1);
    rVertexSetGroup2Group(vertex, group2);
    rGroupAppendChild1Vertex(group1, vertex);
    rGroupAppendChild2Vertex(group2, vertex);
    rPartitionAppendVertex(partition, vertex);
    rVertexSetWeight(vertex, rBoxCloseness(rGroupGetBbox(group1),rGroupGetBbox(group2)));
    return vertex;
}

static void rVertexReevaluate(rVertex vertex)
{
    rGroup group1, group2;
    group1 = rVertexGetGroup1Group(vertex);
    group2 = rVertexGetGroup2Group(vertex);

    rVertexSetWeight(vertex, rBoxCloseness(rGroupGetBbox(group1),rGroupGetBbox(group2)));
}

/*----------------- Partition -------------------------*/

static bool rPartitionFinished(rPartition partition, rSpace space)
{
    rGroup group;
    uint32 count = 0, min = rSpaceGetBucketMinSize(space);

    rForeachPartitionGroup(partition, group) {
        count++;
        if(count > min) {
            return false;
        }
    } rEndPartitionGroup;

    return count <= min;
}

static rVertex rPartitionChooseVertex(rPartition partition) 
{
    rVertex vertex, retval = rVertexNull;
    int32 weight = INT32_MAX;

    rForeachPartitionVertex(partition, vertex) {

        if(rVertexGetWeight(vertex) < weight || retval == rVertexNull) {
            weight = rVertexGetWeight(vertex);
            retval = vertex;
        }
    } rEndPartitionVertex;
    return retval;
}

static uint32 rPartitionCountGroup(rPartition partition) {
    uint32 count = 0;
    rGroup group;

    rForeachPartitionGroup(partition, group) {
        count++;
    } rEndPartitionGroup;

    return count;
}

/*----------------- Node -------------------------*/

static rNode rNodeCreate(rNode parent)
{
    rNode retval = rNodeAlloc();
    rNodeSetBbox(retval, rBoxCreateEmpty());
    if(parent != rNodeNull) {
        rNodeAppendChildNode(parent, retval);
    }
    return retval;
}


static bool rNodeIsLeaf(rNode node)
{
    rShape shape;
    rForeachNodeShape(node,shape){
        return true;
    } rEndNodeShape;
    return false;
}



static bool rNodeEmpty(rNode node)
{
    rShape shape;
    rNode  child;
    rForeachNodeShape(node,shape){
        return false;
    } rEndNodeShape;
    rForeachNodeChildNode(node,child){
        return false;
    } rEndNodeChildNode;
    return true;
}

static void rNodeReevaluateBboxAfterDeletion(rNode node)
{
    rBox tmp = rBoxCreateEmpty();
    rNode child;
    rShape shape;

    if(node != rNodeNull) {
        rForeachNodeChildNode(node, child) {
            rBoxBoundingNode(tmp, child);
        } rEndNodeChildNode;

        rForeachNodeShape(node, shape) {
            rBoxBoundingShape(tmp, shape);
        } rEndNodeShape;

        rNodeReevaluateBboxAfterDeletion(rNodeGetParentNode(node));
    }

    rBoxDestroy(tmp);
}


static void rNodeReevaluateBboxAfterInsertion(rNode node, rBox bbox)
{
    rBox actual = rBoxNull;

    if(node != rNodeNull) {

        actual = rNodeGetBbox(node);

        assert(actual != rBoxNull);

        if(!rBoxWithin(actual, bbox)) {

            rBoxBounding(actual, bbox);
            rNodeReevaluateBboxAfterInsertion(rNodeGetParentNode(node), actual);
        }
    }
}

static void rNodeRemoveAllChildNodes(rNode node)
{
    while(rNodeGetFirstChildNode(node) != rNodeNull) {
        rNodeRemoveChildNode(node, rNodeGetFirstChildNode(node));
    }
}

static void rNodePartition(rNode node, rSpace space)
{
    rPartition partition = rPartitionAlloc();
    rNode child;
    rGroup group1, group2, group;
    rVertex vertex;
    rNode subnode;
    rBox bb;

    rSpaceAppendPartition(space, partition);
    
    /* group creation, one child node per group initially */
    rForeachNodeChildNode(node, child) {

        group1 = rGroupCreate(partition, child);

    } rEndNodeChildNode;

    /* vetex creation, one vertex per group pair, nodir */
    rForeachPartitionGroup(partition, group1) {

        for(group2 = rGroupGetNextPartitionGroup(group1);
            group2 != rGroupNull;
            group2 = rGroupGetNextPartitionGroup(group2)) {

            vertex = rVertexCreate(partition, group1, group2);

        }
    } rEndPartitionGroup;

    /* do the partitioning */
    utDo {
        vertex = rPartitionChooseVertex(partition);

    } utWhile(vertex != rVertexNull && !rPartitionFinished(partition, space)) {

        group1 = rVertexGetGroup1Group(vertex);
        group2 = rVertexGetGroup2Group(vertex);

        rGroupMerge(group1, group2);

    } utRepeat;


    /* apply the partitioning */
    /* rNodeFreeChildNodes(node); */
    rNodeRemoveAllChildNodes(node);
    rForeachPartitionGroup(partition, group) {

        /* keep it in parent */
        if(rGroupContainsOneNode(group)) {

            rForeachGroupNode(group, child) {

                rNodeAppendChildNode(node, child);

            } rEndGroupNode;

        /* create subnode */
        } else {
            subnode = rNodeCreate(node);

            bb = rNodeGetBbox(subnode);

            rForeachGroupNode(group, child) {

                rNodeAppendChildNode(subnode, child);
                rBoxBoundingNode(bb, child);

            } rEndGroupNode;
        }
    } rEndPartitionGroup;

    /* clean up the mess */
    rPartitionDestroy(partition);
}

static void rNodePrint(rNode node, char * prefix)
{
    char next_prefix[1024];
    rShape shape;
    rNode child;

    printf("%sNode: ", prefix);
    rShowNode(node);
    printf("%sNode Box: ", prefix);
    rShowBox(rNodeGetBbox(node));

    rForeachNodeShape(node, shape) {
        printf("%s Shape: ", prefix);
        rShowShape(shape);
        printf("%s  Shape Box:  ", prefix);
        rShowBox(rShapeGetBbox(shape));
    } rEndNodeShape;

    sprintf(next_prefix, "%s  ", prefix);
    rForeachNodeChildNode(node, child) {
        rNodePrint(child, next_prefix);
    } rEndNodeChildNode;
}

static void rNodeSummarize(rNode node, char * prefix)
{
    char next_prefix[strlen(prefix)+4];
    rShape shape;
    rNode child;
    int nbchildren = 0, nbshapes = 0;

    rForeachNodeChildNode(node, child) {
        nbchildren++;
    } rEndNodeChildNode;

    rForeachNodeShape(node, shape) {
        nbshapes++;
    } rEndNodeShape;

    if(nbchildren) {
        printf("%sNode: %d children %d shapes\n", prefix, nbchildren, nbshapes);
    }

    sprintf(next_prefix, "%s  ", prefix);
    rForeachNodeChildNode(node, child) {
        rNodeSummarize(child, next_prefix);
    } rEndNodeChildNode;
}

static bool rNodeNeedPartition(rNode node, rSpace space)
{
    uint32 count = 0, threshold = rSpaceGetBucketMaxSize(space);
    rNode child;

    rForeachNodeChildNode(node, child) {
        count++;
        if(count >= threshold) {
            return true;
        }
    } rEndNodeChildNode;
    return count >= threshold;
}


/*----------------- Best -------------------------*/

struct rBestS
{
    rNode node;
    int32 weight;
};

typedef struct rBestS * rBest;

static rBest rBestCreate()
{
    rBest retval = utNew(struct rBestS);
    retval->node = rNodeNull;
    retval->weight = INT32_MAX;
    return retval;
}

static void rBestDestroy(rBest best)
{
    utFree(best);
}

static void rBestChooseBest(rBest best, rNode node, rBox bbox)
{
    int32 weight = rBoxCloseness(rNodeGetBbox(node), bbox);
    if(weight <= best->weight) {
        best->weight = weight;
        best->node   = node;
    }
}

static void rBestRecurseNode(rBest best, rNode node, rBox bbox)
{
    rNode child;
    rBox  childBox;
    rForeachNodeChildNode(node, child) {
        childBox = rNodeGetBbox(child);
        if(rBoxWithin(childBox, bbox)) {
            rBestChooseBest(best, child, bbox);
            rBestRecurseNode(best, child, bbox);
        }
    } rEndNodeChildNode;
}

/*----------------- Selection -------------------------*/

static rSelection rSelectionCreate(rSpace space)
{
    rSelection selection = rSelectionAlloc();
    rSpaceAppendSelection(space, selection);
    return selection;
}

static void rSelectionAddShapesWithinBox(
    rSelection selection,
    rNode node,
    rBox box)
{
    rNode child;
    rShape shape;
    rItem item;

    rForeachNodeChildNode(node, child) {
        if(rBoxTouching(box, rNodeGetBbox(child))) {
            rSelectionAddShapesWithinBox(selection, child, box);
        }
    } rEndNodeChildNode;

    rForeachNodeShape(node, shape) {
        if(rBoxWithin(box, rShapeGetBbox(shape))) {
            item = rItemAlloc();
            rShapeAppendItem(shape, item);
            rSelectionAppendItem(selection, item);
        }
    } rEndNodeShape;
}

static void rSelectionAddShapesTouchingBox(
    rSelection selection,
    rNode node,
    rBox box)
{
    rNode child;
    rShape shape;
    rItem item;

    rForeachNodeChildNode(node, child) {
        if(rBoxTouching(box, rNodeGetBbox(child))) {
            rSelectionAddShapesTouchingBox(selection, child, box);
        }
    } rEndNodeChildNode;

    rForeachNodeShape(node, shape) {

        if(rBoxTouching(box, rShapeGetBbox(shape))) {

            item = rItemAlloc();
            rShapeAppendItem(shape, item);
            rSelectionAppendItem(selection, item);
        }
    } rEndNodeShape;
}

rSelection rSelectionFindShapesWithinBox(rSpace space, int32 left, int32 bottom, int32 right, int32 top)
{
    rSelection selection = rSelectionCreate(space);

    rBox box = rBoxCreate(left, bottom, right, top);

    rSelectionAddShapesWithinBox(selection, rSpaceGetRoot(space), box);

    rBoxDestroy(box);

    return selection;
}

rSelection rSelectionFindShapesTouchingBox(rSpace space, int32 left, int32 bottom, int32 right, int32 top)
{
    rSelection selection = rSelectionCreate(space);

    rBox box = rBoxCreate(left, bottom, right, top);

    rSelectionAddShapesTouchingBox(selection, rSpaceGetRoot(space), box);

    rBoxDestroy(box);

    return selection;
}



void rSelectionAndShapesDestroy(rSelection selection)
{
    rItem item;

    rSafeForeachSelectionItem(selection, item) {
        rShapeDestroy(rItemGetShape(item));
    } rEndSafeSelectionItem;

    rSelectionDestroy(selection);
}

unsigned rSelectionGetNumShapes(rSelection selection)
{
    unsigned retval = 0;
    rItem item;

    rSafeForeachSelectionItem(selection, item) {
        retval++;
    } rEndSafeSelectionItem;
    return retval;
}

void rSelectionApply(
    rSelection selection,
    void * that,
    void (*callback)(void * that, rSelection selection, rShape shape))
{
    rItem item;
    rForeachSelectionItem(selection, item) {
        callback(that, selection, rItemGetShape(item));
    } rEndSelectionItem;
}

void rSelectionSafeApply(
    rSelection selection,
    void * that,
    void (*callback)(void * that, rSelection selection, rShape shape))
{
    rItem item;
    rSafeForeachSelectionItem(selection, item) {
        callback(that, selection, rItemGetShape(item));
    } rEndSafeSelectionItem;
}


/*----------------- Space -------------------------*/

rSpace rSpaceCreate()
{
    rSpace retval = rSpaceAlloc();
    rNode root = rNodeCreate(rNodeNull);
    rSpaceSetRoot(retval, root);
    rSpaceSetBucketMaxSize(retval, 10);
    rSpaceSetBucketMinSize(retval, 5);
    return retval;
}

unsigned rSpaceGetNumShapes(
    rSpace space)
{
    unsigned retval = 0;
    rShape shape;

    rSafeForeachSpaceContainingShape(space, shape) {
        retval++;
    } rEndSafeSpaceContainingShape;
    return retval;
}


static rNode rSpaceFindEnclosingNode(
    rSpace space,
    rBox box)
{
    rBest best = rBestCreate();
    rNode node;

    /* root MUST be enclosing after insertion */
    node = best->node = rSpaceGetRoot(space);

    best->weight = rBoxCloseness(rNodeGetBbox(node), box);

    rBestRecurseNode(best, node, box);

    node = best->node;
    rBestDestroy(best);
    return node;
}

static void rSpaceInsertShape(
    rSpace space,
    rShape shape)
{
    rNode child;
    rNode node = rSpaceFindEnclosingNode(space, rShapeGetBbox(shape));

    /* we could be even more clever than that and use a weight threshold instead */
    if( rSpaceGetRoot(space) != node &&
        rBoxEq(rNodeGetBbox(node), rShapeGetBbox(shape))) {

        rNodeAppendShape(node, shape);
        return;
    }

    if(rNodeIsLeaf(node)) {
        node = rNodeGetParentNode(node);
    }

    child = rNodeCreate(node);

    rNodeAppendShape(child, shape);
    rNodeReevaluateBboxAfterInsertion(child, rShapeGetBbox(shape));

    if(rNodeNeedPartition(node, space)) {
        rNodePartition(node, space);
    }
}


static void signalShapeDestruction(
    rShape shape)
{
    rNode parent, node = rShapeGetNode(shape);

    if(node == rNodeNull) {
        return;
    }

    parent = rNodeGetParentNode(node);

    /* node is now useless */
    if(rNodeGetFirstShape(node) == shape &&
        rNodeGetLastShape(node) == shape) {

        rNodeDestroy(node);

        utDo {
            node = parent;

        } utWhile(node != rNodeNull && rNodeEmpty(node)) {

            rNodeDestroy(node);
            parent = rNodeGetParentNode(node);

        } utRepeat;
    }

    if(node != rNodeNull) {
        rNodeReevaluateBboxAfterDeletion(node);
    }
}

static void rSpacePrint(
    rSpace space)
{
    rShowSpace(space);
    rNodePrint(rSpaceGetRoot(space), "  ");
}

static void rSpaceSummarize(
    rSpace space)
{
    rNode node;
    rShape shape;
    int shape_count = 0;
    int max_depth = 0;
    int sum_depth = 0;
    int depth;

    rForeachSpaceContainingShape(space, shape) {
        shape_count++;
        depth = 0;

        node = rShapeGetNode(shape);

        utDo {
            node = rNodeGetParentNode(node);

        } utWhile(node != rNodeNull) {

            depth++;
        } utRepeat;

        max_depth = utMax(max_depth, depth);
        sum_depth += depth;

    } rEndSpaceContainingShape;

    printf("Space: %d shapes, max depth = %d, mean depth = %f\n",
        shape_count, max_depth, (double)sum_depth/(double)shape_count);
}

/* ----------- BIST ---------------*/

static void rtree_bist_helper(void * that, rSelection selection, rShape shape)
{
    rBox box = rBoxCreateEmpty();
    rBoxFillRandom(box);
    rShapeSetBoundingBox(shape, rBoxGetLeft(box), rBoxGetBottom(box), rBoxGetRight(box), rBoxGetTop(box));
    rBoxDestroy(box);
}

/* declared in tclfunc.h */
void rtree_bist(void)
{
    rSpace space;
    rShape shape;
    rBox box;
    rSelection selection;
    int i;

    rStart();

    space = rSpaceCreate();

    utInitSeed(42);

    box = rBoxCreate(0,0,0,0);
    for(i=0; i<100000; i++) {
        rBoxFillRandom(box);
        shape = rShapeCreate(space, rBoxGetLeft(box), rBoxGetBottom(box), rBoxGetRight(box), rBoxGetTop(box));
    }
    rBoxDestroy(box);

    printf("Initial space\n"); rSpaceSummarize(space);

    selection = rSelectionFindShapesTouchingBox(space, 500, 500, 700, 700);
    printf("selection touching %d\n", rSelectionGetNumShapes(selection));
    rSelectionDestroy(selection);

    selection = rSelectionFindShapesWithinBox(space, 500, 500, 700, 700);
    printf("selection within %d\n", rSelectionGetNumShapes(selection));
    rSelectionAndShapesDestroy(selection);
    
    printf("space after destuction\n"); rSpaceSummarize(space);

    selection = rSelectionFindShapesTouchingBox(space, 500, 500, 700, 700);
    printf("selection touching %d\n", rSelectionGetNumShapes(selection));

    rSelectionApply(selection, NULL, rtree_bist_helper);
    rSelectionDestroy(selection);

    printf("space after random move of selection\n"); rSpaceSummarize(space);

    selection = rSelectionFindShapesWithinBox(space, 500, 500, 700, 700);
    printf("selection within %d\n", rSelectionGetNumShapes(selection));
    rSelectionAndShapesDestroy(selection);
    
    printf("and another destroy\n"); rSpaceSummarize(space);

    /*rSpacePrint(space);*/

    rSpaceDestroy(space);

    rStop();
}
