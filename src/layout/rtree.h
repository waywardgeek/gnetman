
extern void rStart();
extern void rStop();

typedef struct struct_rSpace{char val;} *rSpace;
typedef struct struct_rShape{char val;} *rShape;
typedef struct struct_rSelection{char val;} *rSelection;

extern rSpace rSpaceCreate();
extern unsigned rSpaceGetNumShapes(rSpace space);

extern rShape rShapeCreate(rSpace space, uint32 left, uint32 bottom, uint32 right, uint32 top);
extern void rShapeSetBoundingBox(rShape shape, uint32 left, uint32 bottom, uint32 right, uint32 top);
extern void rShapeDestroy(Space space);

extern rSelection rSelectionFindShapesWithinBox(rSpace space, uint32 left, uint32 bottom, uint32 right, uint32 top);
extern rSelection rSelectionFindShapesTouchingBox(rSpace space, uint32 left, uint32 bottom, uint32 right, uint32 top);
extern void rSelectionDestroy(rSelection selection);
extern void rSelectionAndShapesDestroy(rSelection selection);
extern unsigned rSelectionGetNumShapes(rSelection selection);
extern void rSelectionApply(rSelection selection, void * that, void (*callback)(void * that, rSelection selection, rShape shape));
extern void rSelectionSafeApply(rSelection selection, void * that, void (*callback)(void * that, rSelection selection, rShape shape));
